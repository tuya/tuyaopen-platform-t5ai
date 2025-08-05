/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <components/cherryusb/usbh_hub.h>
#include "usb_errno.h"

#define DEV_FORMAT "/dev/hub%d"

#define DEBOUNCE_TIMEOUT       400
#define DEBOUNCE_TIME_STEP     25
#define DELAY_TIME_AFTER_RESET 20

#define EXTHUB_FIRST_INDEX 2

static uint32_t g_devinuse = 0;

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t g_hub_buf[32];

usb_slist_t hub_event_head = USB_SLIST_OBJECT_INIT(hub_event_head);
usb_slist_t hub_class_head = USB_SLIST_OBJECT_INIT(hub_class_head);

usb_osal_sem_t hub_event_wait = NULL;
usb_osal_thread_t hub_thread = NULL;
usb_osal_mutex_t hub_event_mutex = NULL;

USB_NOCACHE_RAM_SECTION struct usbh_hub *roothub = NULL;
//struct usbh_hubport roothub_parent_port;
struct usbh_hubport *roothub_parent_port = NULL;

//USB_NOCACHE_RAM_SECTION struct usbh_hub exthub[CONFIG_USBHOST_MAX_EXTHUBS];
USB_NOCACHE_RAM_SECTION struct usbh_hub *ext_onehub = NULL;

extern int usbh_hport_activate_ep0(struct usbh_hubport *hport);
extern int usbh_hport_deactivate_ep0(struct usbh_hubport *hport);
extern int usbh_enumerate(struct usbh_hubport *hport);

static const char *speed_table[] = { "error-speed", "low-speed", "full-speed", "high-speed", "wireless-speed", "super-speed", "superplus-speed" };

static beken_queue_t hub_event_queue = NULL;
#define HUB_EVENT_QITEM_COUNT 100
typedef void (*usbh_event_queue_cb_t)();

typedef struct {
    usbh_event_queue_cb_t callback;
    void *arg;
} hub_event_queue_t;

static int usbh_hub_event_send_queue(void *callback, void *arg);
extern void bk_usb_phy_register_refresh();
extern uint32_t usb_hc_enter_critical();
extern void usb_hc_exit_critical(uint32_t flags);

static int usbh_hub_devno_alloc(void)
{
    int devno;

    for (devno = EXTHUB_FIRST_INDEX; devno < 32; devno++) {
        uint32_t bitno = 1 << devno;
        if ((g_devinuse & bitno) == 0) {
            g_devinuse |= bitno;
            return devno;
        }
    }

    return -EMFILE;
}

static void usbh_hub_devno_free(uint8_t devno)
{
    if (devno >= EXTHUB_FIRST_INDEX && devno < 32) {
        g_devinuse &= ~(1 << devno);
    }
}

static int _usbh_hub_get_hub_descriptor(struct usbh_hub *hub, uint8_t *buffer)
{
    struct usb_setup_packet *setup;
    int ret;

    setup = &hub->parent->setup;

    setup->bmRequestType = USB_REQUEST_DIR_IN | USB_REQUEST_CLASS | USB_REQUEST_RECIPIENT_DEVICE;
    setup->bRequest = USB_REQUEST_GET_DESCRIPTOR;
    setup->wValue = HUB_DESCRIPTOR_TYPE_HUB << 8;
    setup->wIndex = 0;
    setup->wLength = USB_SIZEOF_HUB_DESC;

    ret = usbh_control_transfer(hub->parent->ep0, setup, g_hub_buf);
    if (ret < 0) {
        return ret;
    }
    memcpy(buffer, g_hub_buf, USB_SIZEOF_HUB_DESC);
    return ret;
}

static int _usbh_hub_get_status(struct usbh_hub *hub, uint8_t *buffer)
{
    struct usb_setup_packet *setup;
    int ret;

    setup = &hub->parent->setup;

    setup->bmRequestType = USB_REQUEST_DIR_IN | USB_REQUEST_CLASS | USB_REQUEST_RECIPIENT_DEVICE;
    setup->bRequest = HUB_REQUEST_GET_STATUS;
    setup->wValue = 0;
    setup->wIndex = 0;
    setup->wLength = 2;

    ret = usbh_control_transfer(hub->parent->ep0, setup, g_hub_buf);
    if (ret < 0) {
        return ret;
    }
    memcpy(buffer, g_hub_buf, 2);
    return ret;
}

static int _usbh_hub_get_portstatus(struct usbh_hub *hub, uint8_t port, struct hub_port_status *port_status)
{
    struct usb_setup_packet *setup;
    int ret;

    setup = &hub->parent->setup;

    setup->bmRequestType = USB_REQUEST_DIR_IN | USB_REQUEST_CLASS | USB_REQUEST_RECIPIENT_OTHER;
    setup->bRequest = HUB_REQUEST_GET_STATUS;
    setup->wValue = 0;
    setup->wIndex = port;
    setup->wLength = 4;

    ret = usbh_control_transfer(hub->parent->ep0, setup, g_hub_buf);
    if (ret < 0) {
        return ret;
    }
    memcpy(port_status, g_hub_buf, 4);
    return ret;
}

static int _usbh_hub_set_feature(struct usbh_hub *hub, uint8_t port, uint8_t feature)
{
    struct usb_setup_packet *setup;

    setup = &hub->parent->setup;

    setup->bmRequestType = USB_REQUEST_DIR_OUT | USB_REQUEST_CLASS | USB_REQUEST_RECIPIENT_OTHER;
    setup->bRequest = HUB_REQUEST_SET_FEATURE;
    setup->wValue = feature;
    setup->wIndex = port;
    setup->wLength = 0;

    return usbh_control_transfer(hub->parent->ep0, setup, NULL);
}

static int _usbh_hub_clear_feature(struct usbh_hub *hub, uint8_t port, uint8_t feature)
{
    struct usb_setup_packet *setup;

    setup = &hub->parent->setup;

    setup->bmRequestType = USB_REQUEST_DIR_OUT | USB_REQUEST_CLASS | USB_REQUEST_RECIPIENT_OTHER;
    setup->bRequest = HUB_REQUEST_CLEAR_FEATURE;
    setup->wValue = feature;
    setup->wIndex = port;
    setup->wLength = 0;

    return usbh_control_transfer(hub->parent->ep0, setup, NULL);
}

static int parse_hub_descriptor(struct usb_hub_descriptor *desc, uint16_t length)
{
    if (desc->bLength != USB_SIZEOF_HUB_DESC) {
        USB_LOG_ERR("invalid device bLength 0x%02x\r\n", desc->bLength);
        return -1;
    } else if (desc->bDescriptorType != HUB_DESCRIPTOR_TYPE_HUB) {
        USB_LOG_ERR("unexpected descriptor 0x%02x\r\n", desc->bDescriptorType);
        return -2;
    } else {
        USB_LOG_VBS("Hub Descriptor:\r\n");
        USB_LOG_VBS("bLength: 0x%02x             \r\n", desc->bLength);
        USB_LOG_VBS("bDescriptorType: 0x%02x     \r\n", desc->bDescriptorType);
        USB_LOG_VBS("bNbrPorts: 0x%02x           \r\n", desc->bNbrPorts);
        USB_LOG_VBS("wHubCharacteristics: 0x%04x \r\n", desc->wHubCharacteristics);
        USB_LOG_VBS("bPwrOn2PwrGood: 0x%02x      \r\n", desc->bPwrOn2PwrGood);
        USB_LOG_VBS("bHubContrCurrent: 0x%02x    \r\n", desc->bHubContrCurrent);
        USB_LOG_VBS("DeviceRemovable: 0x%02x     \r\n", desc->DeviceRemovable);
        USB_LOG_VBS("PortPwrCtrlMask: 0x%02x     \r\n", desc->PortPwrCtrlMask);
    }
    return 0;
}

static int usbh_hub_get_portstatus(struct usbh_hub *hub, uint8_t port, struct hub_port_status *port_status)
{
    struct usb_setup_packet roothub_setup;
    struct usb_setup_packet *setup;

    if (hub->is_roothub) {
        setup = &roothub_setup;
        setup->bmRequestType = USB_REQUEST_DIR_IN | USB_REQUEST_CLASS | USB_REQUEST_RECIPIENT_OTHER;
        setup->bRequest = HUB_REQUEST_GET_STATUS;
        setup->wValue = 0;
        setup->wIndex = port;
        setup->wLength = 4;
        return usbh_roothub_control(&roothub_setup, (uint8_t *)port_status);
    } else {
        return _usbh_hub_get_portstatus(hub, port, port_status);
    }
}

static int usbh_hub_set_feature(struct usbh_hub *hub, uint8_t port, uint8_t feature)
{
    struct usb_setup_packet roothub_setup;
    struct usb_setup_packet *setup;

    if (hub->is_roothub) {
        setup = &roothub_setup;
        setup->bmRequestType = USB_REQUEST_DIR_OUT | USB_REQUEST_CLASS | USB_REQUEST_RECIPIENT_OTHER;
        setup->bRequest = HUB_REQUEST_SET_FEATURE;
        setup->wValue = feature;
        setup->wIndex = port;
        setup->wLength = 0;
        return usbh_roothub_control(setup, NULL);
    } else {
        return _usbh_hub_set_feature(hub, port, feature);
    }
}

static int usbh_hub_clear_feature(struct usbh_hub *hub, uint8_t port, uint8_t feature)
{
    struct usb_setup_packet roothub_setup;
    struct usb_setup_packet *setup;

    if (hub->is_roothub) {
        setup = &roothub_setup;
        setup->bmRequestType = USB_REQUEST_DIR_OUT | USB_REQUEST_CLASS | USB_REQUEST_RECIPIENT_OTHER;
        setup->bRequest = HUB_REQUEST_CLEAR_FEATURE;
        setup->wValue = feature;
        setup->wIndex = port;
        setup->wLength = 0;
        return usbh_roothub_control(setup, NULL);
    } else {
        return _usbh_hub_clear_feature(hub, port, feature);
    }
}

static void usbh_hub_thread_wakeup(struct usbh_hub *hub)
{
    usbh_hub_event_send_queue(NULL, hub);
}

uint8_t connect_int_buffer[1];
static void hub_int_complete_callback(void *arg, int nbytes)
{
    struct usbh_hub *hub = (struct usbh_hub *)arg;

    USB_LOG_VBS("hub_int_complete_callback: nbytes:%x int_buffer:0x%x data:0x%x\r\n", nbytes, hub->int_buffer, hub->int_buffer[0]);
    if(connect_int_buffer[0] > 0) {
        hub->int_buffer[0] = (hub->int_buffer[0] | connect_int_buffer[0]);
    }
    hub->intin_urb.transfer_buffer = &hub->int_buffer[0];

    if (nbytes > 0) {
        usbh_hub_thread_wakeup(hub);
    }
    connect_int_buffer[0] = 0;
}

static int usbh_hub_connect(struct usbh_hubport *hport, uint8_t intf)
{
    struct usb_endpoint_descriptor *ep_desc;
    struct hub_port_status port_status;
    int ret;
    int index;
    struct usbh_hubport *child;

    index = usbh_hub_devno_alloc();
    if (index > (CONFIG_USBHOST_MAX_EXTHUBS + EXTHUB_FIRST_INDEX - 1)) {
        USB_LOG_ERR("No memory to alloc hub class\r\n");
        usbh_hub_devno_free(index);
        return -ENOMEM;
    }

    //struct usbh_hub *hub = &exthub[index - EXTHUB_FIRST_INDEX];
    struct usbh_hub *hub = ext_onehub;

    memset(hub, 0, sizeof(struct usbh_hub));
    hub->hub_addr = hport->dev_addr;
    hub->parent = hport;
    hub->index = index;
    USB_LOG_VBS("%s hport->dev_addr:%x intf:%d\r\n",__func__, hport->dev_addr, intf);

    hport->config.intf[intf].priv = hub;

    ret = _usbh_hub_get_hub_descriptor(hub, (uint8_t *)&hub->hub_desc);
    if (ret < 0) {
        return ret;
    }

    parse_hub_descriptor(&hub->hub_desc, USB_SIZEOF_HUB_DESC);

    for (uint8_t port = 0; port < hub->hub_desc.bNbrPorts; port++) {
        hub->child[port].port = port + 1;
        hub->child[port].parent = hub;
    }

    ep_desc = &hport->config.intf[intf].altsetting[0].ep[0].ep_desc;
    if (ep_desc->bEndpointAddress & 0x80) {
        usbh_hport_activate_epx(&hub->intin, hport, ep_desc);
    } else {
        return -1;
    }

    for (uint8_t port = 0; port < hub->hub_desc.bNbrPorts; port++) {
        ret = usbh_hub_set_feature(hub, port + 1, HUB_PORT_FEATURE_POWER);
        if (ret < 0) {
            return ret;
        }
    }

    for (uint8_t port = 0; port < hub->hub_desc.bNbrPorts; port++) {
        ret = usbh_hub_get_portstatus(hub, port + 1, &port_status);
        USB_LOG_VBS("port %u, status:0x%02x, change:0x%02x\r\n", port + 1, port_status.wPortStatus, port_status.wPortChange);
        //hub->int_buffer[0] |= (port_status.wPortChange << (port + 1));
        connect_int_buffer[0] |= (port_status.wPortChange << (port + 1));
        child = &hub->child[port];
        memset(child, 0, sizeof(struct usbh_hubport));
        if (ret < 0) {
            return ret;
        }
    }

    hub->connected = true;
    snprintf(hport->config.intf[intf].devname, CONFIG_USBHOST_DEV_NAMELEN, DEV_FORMAT, hub->index);
    usbh_hub_register(hub);
    USB_LOG_DBG("Register HUB Class:%s\r\n", hport->config.intf[intf].devname);

    usbh_int_urb_fill(&hub->intin_urb, hub->intin, hub->int_buffer, 1, 0, hub_int_complete_callback, hub);

    usbh_submit_urb(&hub->intin_urb);
    return 0;
}

static int usbh_hub_disconnect(struct usbh_hubport *hport, uint8_t intf)
{
    struct usbh_hubport *child;
    int ret = 0;

    struct usbh_hub *hub = (struct usbh_hub *)hport->config.intf[intf].priv;

    if (hub) {
        usbh_hub_devno_free(hub->index);

        if (hub->intin) {
            usbh_pipe_free(hub->intin);
        }

        for (uint8_t port = 0; port < hub->hub_desc.bNbrPorts; port++) {
            child = &hub->child[port];
            usbh_hport_deactivate_ep0(child);
            for (uint8_t i = 0; i < child->config.config_desc.bNumInterfaces; i++) {
                if (child->config.intf[i].class_driver && child->config.intf[i].class_driver->disconnect) {
                    ret = CLASS_DISCONNECT(child, i);
                }
            }

            child->config.config_desc.bNumInterfaces = 0;
            child->parent = NULL;
        }

        usbh_hub_unregister(hub);
        memset(hub, 0, sizeof(struct usbh_hub));

        if (hport->config.intf[intf].devname[0] != '\0')
            USB_LOG_DBG("Unregister HUB Class:%s\r\n", hport->config.intf[intf].devname);
    }
    return ret;
}

static void usbh_roothub_free_port1_hub(void)
{
    struct usbh_hubport *child;
    struct usbh_hub *hub;
    hub = roothub;

    child = &hub->child[0];
    if(child->config.config_desc.bNumInterfaces == 0) return;
    child->connected = false;
    usbh_hport_deactivate_ep0(child);
    for (uint8_t i = 0; i < child->config.config_desc.bNumInterfaces; i++) {
        if (child->config.intf[i].class_driver && child->config.intf[i].class_driver->disconnect) {
            CLASS_DISCONNECT(child, i);
        }
        if (child->raw_config_desc) {
            usb_free(child->raw_config_desc);
            child->raw_config_desc = NULL;
        }
    }

    usbh_device_unmount_done_callback(child);
    child->config.config_desc.bNumInterfaces = 0;

}

static void usbh_roothub_register(void)
{
    USB_LOG_VBS("[+]%s\r\n", __func__);

    uint32_t psram_malloc_fail_flag = 0;

    do
    {
        if(!roothub){
            roothub = psram_malloc(sizeof(struct usbh_hub));
            if(!roothub) {
                USB_LOG_DBG("%s roothub Malloc Fail\r\n", __func__);
                psram_malloc_fail_flag = 1;
                break;
            } else {
                memset(roothub, 0, sizeof(struct usbh_hub));
            }
        }

        if(!ext_onehub){
            ext_onehub = psram_malloc(sizeof(struct usbh_hub));
            if(!ext_onehub) {
                USB_LOG_DBG("%s ext_onehub Malloc Fail\r\n", __func__);
                psram_malloc_fail_flag = 1;
                break;
            } else {
                memset(ext_onehub, 0, sizeof(struct usbh_hub));
            }
        }

        if(!roothub_parent_port){
            roothub_parent_port = psram_malloc(sizeof(struct usbh_hub));
            if(!roothub_parent_port) {
                USB_LOG_DBG("%s roothub Malloc Fail\r\n", __func__);
                psram_malloc_fail_flag = 1;
                break;
            } else {
            	memset(roothub_parent_port, 0, sizeof(struct usbh_hub));
            }
        }
    }while(0);

    if(psram_malloc_fail_flag) {
        if(roothub){
            psram_free(roothub);
            roothub = NULL;
        }

        if(ext_onehub){
            psram_free(ext_onehub);
            ext_onehub = NULL;
        }

        if(roothub_parent_port){
            psram_free(roothub_parent_port);
            roothub_parent_port = NULL;
        }
     return;
    }

    //memset(&roothub, 0, sizeof(struct usbh_hub));
    //memset(&roothub_parent_port, 0, sizeof(struct usbh_hubport));
    roothub_parent_port->port = 1;
    roothub_parent_port->dev_addr = 1;
    roothub->connected = true;
    roothub->index = 1;
    roothub->is_roothub = true;
    roothub->parent = roothub_parent_port;
    roothub->hub_addr = roothub_parent_port->dev_addr;
    roothub->hub_desc.bNbrPorts = CONFIG_USBHOST_MAX_RHPORTS;
    memset(&(roothub->child[0]), 0, sizeof(struct usbh_hubport));
    usbh_hub_register(roothub);
    USB_LOG_VBS("[-]%s\r\n", __func__);
}

static void usbh_roothub_unregister(void)
{
    usbh_hub_unregister(roothub);
    if(roothub){
        psram_free(roothub);
        roothub = NULL;
    }

    if(ext_onehub){
        psram_free(ext_onehub);
        ext_onehub = NULL;
    }

    if(roothub_parent_port){
        psram_free(roothub_parent_port);
        roothub_parent_port = NULL;
    }

}

void usbh_hub_event_lock_mutex()
{
    if(hub_event_mutex)
        usb_osal_mutex_take(&hub_event_mutex);
}

void usbh_hub_event_unlock_mutex()
{
    if(hub_event_mutex)
        usb_osal_mutex_give(&hub_event_mutex);
}

static void usbh_roothub_enumerate_status_handle(bool status)
{
    static uint32_t roothub_enum_fail_count = 0;

    if(status){
        roothub_enum_fail_count = 0;
    } else {
        if(roothub_enum_fail_count > 0) {
            USB_LOG_ERR("roothub_enum_fail_count:%d enumerate fail\r\n", roothub_enum_fail_count);
            uint32_t pop_queue_all_event_count = 0;
            hub_event_queue_t msg;
            
            uint32_t flags = usb_hc_enter_critical();
            if (hub_event_queue != NULL) {
                while((kNoErr == rtos_pop_from_queue(&hub_event_queue, &msg, 0)))
                {
                    pop_queue_all_event_count++;
                    if(pop_queue_all_event_count >= HUB_EVENT_QITEM_COUNT) {
                        break;
                    }
                }
            }
            usb_hc_exit_critical(flags);
            bk_usb_phy_register_refresh();
            roothub_enum_fail_count = 0;
        } else {
            roothub_enum_fail_count++;
            usbh_musb_trigger_disconnect_by_sw();
        }
    }

}

static void usbh_hub_events_connect_handle(struct usbh_hub *hub, struct usbh_hubport *child, uint8_t port)
{
    child->parent = hub;
    child->connected = true;
    child->port = port + 1;

    USB_LOG_DBG("New %s device on Hub %u, Port %u connected\r\n", speed_table[child->speed], hub->index, port + 1);
    
    if (usbh_enumerate(child) < 0) {
    	child->connected = false;
    	USB_LOG_ERR("Port %u enumerate fail\r\n", port + 1);
        if(hub->is_roothub) {
            usbh_roothub_enumerate_status_handle(false);
        } else {
            usbh_hub_event_lock_mutex();
            usbh_hub_pipe_reconfigure(hub->parent->ep0, hub->hub_addr, 0x40, USB_SPEED_HIGH);
            int ret = usbh_hub_set_feature(hub, port + 1, HUB_PORT_FEATURE_RESET);
            if (ret < 0) {
                USB_LOG_ERR("%s Failed to reset port %u,errorcode:%d\r\n", __func__, port, ret);
            }
            usbh_hub_event_unlock_mutex();
        }
    } else {
        if(hub->is_roothub) {
            usbh_roothub_enumerate_status_handle(true);
        }
    }
}

static void usbh_hub_events_disconnect_handle(struct usbh_hub *hub, struct usbh_hubport *child, uint8_t port)
{
    if(child == NULL) {
        USB_LOG_DBG("%s child is null\r\n", __func__);
        return;
    }

    child->connected = false;
    usbh_hport_deactivate_ep0(child);
    for (uint8_t i = 0; i < child->config.config_desc.bNumInterfaces; i++) {
        if (child->config.intf[i].class_driver && child->config.intf[i].class_driver->disconnect) {
            CLASS_DISCONNECT(child, i);
        }
    }
    if (child->raw_config_desc) {
         usb_free(child->raw_config_desc);
         child->raw_config_desc = NULL;
    }
    
    USB_LOG_DBG("Device on Hub %u, Port %u disconnected\r\n", hub->index, port + 1);
    usbh_device_unmount_done_callback(child);
    child->config.config_desc.bNumInterfaces = 0;

    if(hub->is_roothub) {
    	bk_usb_phy_register_refresh();
    }
}

static int usbh_hub_event_send_queue(void *callback, void *arg)
{
    bk_err_t ret;
    hub_event_queue_t msg;

    msg.callback = callback;
    msg.arg = arg;

    uint32_t flags = usb_hc_enter_critical();
    if (hub_event_queue != NULL) {
        ret = rtos_push_to_queue(&hub_event_queue, &msg, 0);
        if (kNoErr != ret) {
            USB_LOG_DBG("%s fail ret:%d\r\n", __func__, ret);
            rtos_reset_queue(&hub_event_queue);
        }
        usb_hc_exit_critical(flags);
        return ret;
    }
    usb_hc_exit_critical(flags);
    return BK_OK;
}

void usbh_roothub_thread_send_queue(uint8_t port, void *callback)
{
    roothub->int_buffer[0] |= (1 << port);
    usbh_hub_event_send_queue(callback, roothub);
}

static void usbh_hub_events(struct usbh_hub *hub)
{
    struct usbh_hubport *child;
    struct hub_port_status port_status;
    uint8_t portchange_index;
    uint16_t portstatus;
    uint16_t portchange;
    uint8_t speed;
    int ret;
    bool child_connected = false;

    if (!hub->connected) {
        return;
    }

    uint16_t mask;
    uint16_t feat;
    uint16_t clear_feature_err_count;
  
    if(hub->hub_desc.bNbrPorts > CONFIG_USBHOST_MAX_EHPORTS)
    {
        USB_LOG_ERR("%s Exceeding the Maximum value bNbrPorts: %d\r\n", __func__, hub->hub_desc.bNbrPorts);
        usbh_musb_trigger_disconnect_by_sw();
        return;
    }

    for (uint8_t port = 0; port < hub->hub_desc.bNbrPorts; port++) {
        portchange_index = hub->int_buffer[0];

        USB_LOG_VBS("Port %u change:0x%02x\r\n", port + 1, portchange_index);

        if (!(portchange_index & (1 << (port + 1)))) {
            continue;
        }

        child = &hub->child[port];
        child_connected = child->connected;

        portchange_index &= ~(1 << (port + 1));
        USB_LOG_VBS("Port %d change hub_addr:%d\r\n", port + 1, hub->hub_addr);
        usbh_hub_event_lock_mutex();
        if (!hub->is_roothub) {
             usbh_hub_pipe_reconfigure(hub->parent->ep0, hub->hub_addr, 0x40, USB_SPEED_HIGH);
        }
        /* Read hub port status */
        ret = usbh_hub_get_portstatus(hub, port + 1, &port_status);
        usbh_hub_event_unlock_mutex();
        if (ret < 0) {
            USB_LOG_ERR("Failed to read port %u status, errorcode: %d\r\n", port + 1, ret);
            continue;
        }

        portstatus = port_status.wPortStatus;
        portchange = port_status.wPortChange;

        USB_LOG_VBS("port %u, status:0x%02x, change:0x%02x\r\n", port + 1, portstatus, portchange);

        /* First, clear all change bits */
        mask = 1;
        feat = HUB_PORT_FEATURE_C_CONNECTION;
        clear_feature_err_count = 0;
        while (portchange) {
            if(clear_feature_err_count > 5) {
                usbh_musb_trigger_disconnect_by_sw();
                return;
            }
            if (portchange & mask) {
                usbh_hub_event_lock_mutex();
                if (!hub->is_roothub) {
                    usbh_hub_pipe_reconfigure(hub->parent->ep0, hub->hub_addr, 0x40, USB_SPEED_HIGH);
                }
                ret = usbh_hub_clear_feature(hub, port + 1, feat);
                usbh_hub_event_unlock_mutex();
                if (ret < 0) {
                    clear_feature_err_count++;
                    USB_LOG_ERR("Failed to clear port %u, change mask:%04x, errorcode:%d\r\n", port + 1, mask, ret);
                    continue;
                }
                portchange &= (~mask);
            }
            mask <<= 1;
            feat++;
        }

        portchange = port_status.wPortChange;

        /* Second, if port changes, debounces first */
        if ((portchange & HUB_PORT_STATUS_C_CONNECTION) || (portchange & HUB_PORT_STATUS_C_ENABLE)) {
            uint16_t connection = 0;
            uint16_t debouncestable = 0;
            for (uint32_t debouncetime = 0; debouncetime < DEBOUNCE_TIMEOUT; debouncetime += DEBOUNCE_TIME_STEP) {
                usb_osal_msleep(DEBOUNCE_TIME_STEP);
                usbh_hub_event_lock_mutex();
                if (!hub->is_roothub) {
                    usbh_hub_pipe_reconfigure(hub->parent->ep0, hub->hub_addr, 0x40, USB_SPEED_HIGH);
                }
                /* Read hub port status */
                ret = usbh_hub_get_portstatus(hub, port + 1, &port_status);
                usbh_hub_event_unlock_mutex();
                if (ret < 0) {
                    USB_LOG_ERR("Failed to read port %u status, errorcode: %d\r\n", port + 1, ret);
                    continue;
                }

                portstatus = port_status.wPortStatus;
                portchange = port_status.wPortChange;

                USB_LOG_VBS("Port %u, status:0x%02x, change:0x%02x\r\n", port + 1, portstatus, portchange);
                if ((portstatus & HUB_PORT_STATUS_CONNECTION) == connection) {
                    if (connection) {
                        if (++debouncestable == 4) {
                            break;
                        }
                    }
                } else {
                    debouncestable = 0;
                }

                connection = portstatus & HUB_PORT_STATUS_CONNECTION;

                if (portchange & HUB_PORT_STATUS_C_CONNECTION) {
                    usbh_hub_event_lock_mutex();
                    if (!hub->is_roothub) {
                        usbh_hub_pipe_reconfigure(hub->parent->ep0, hub->hub_addr, 0x40, USB_SPEED_HIGH);
                    }
                    usbh_hub_clear_feature(hub, port + 1, HUB_PORT_FEATURE_C_CONNECTION);
                    usbh_hub_event_unlock_mutex();
                }
            }

            /* Last, check connect status */
            if (portstatus & HUB_PORT_STATUS_CONNECTION) {

                if(child_connected) {
                    USB_LOG_ERR("port %u Please check the voltage and wires of the device\r\n", port + 1);
                    usbh_hub_events_disconnect_handle(hub, child, port);
                }

                usbh_hub_event_lock_mutex();
                if (!hub->is_roothub) {
                    usbh_hub_pipe_reconfigure(hub->parent->ep0, hub->hub_addr, 0x40, USB_SPEED_HIGH);
                }
                ret = usbh_hub_set_feature(hub, port + 1, HUB_PORT_FEATURE_RESET);
                usbh_hub_event_unlock_mutex();
                if (ret < 0) {
                    USB_LOG_ERR("Failed to reset port %u,errorcode:%d\r\n", port, ret);
                    continue;
                }

                usb_osal_msleep(DELAY_TIME_AFTER_RESET);
                /* Read hub port status */
                usbh_hub_event_lock_mutex();
                if (!hub->is_roothub) {
                    usbh_hub_pipe_reconfigure(hub->parent->ep0, hub->hub_addr, 0x40, USB_SPEED_HIGH);
                }
                ret = usbh_hub_get_portstatus(hub, port + 1, &port_status);
                usbh_hub_event_unlock_mutex();
                if (ret < 0) {
                    USB_LOG_ERR("Failed to read port %u status, errorcode: %d\r\n", port + 1, ret);
                    continue;
                }

                portstatus = port_status.wPortStatus;
                portchange = port_status.wPortChange;
                if (!(portstatus & HUB_PORT_STATUS_RESET) && (portstatus & HUB_PORT_STATUS_ENABLE)) {
                    if (portchange & HUB_PORT_STATUS_C_RESET) {
                        usbh_hub_event_lock_mutex();
                        if (!hub->is_roothub) {
                            usbh_hub_pipe_reconfigure(hub->parent->ep0, hub->hub_addr, 0x40, USB_SPEED_HIGH);
                        }
                        ret = usbh_hub_clear_feature(hub, port + 1, HUB_PORT_FEATURE_C_RESET);
                        usbh_hub_event_unlock_mutex();
                        if (ret < 0) {
                            USB_LOG_ERR("Failed to clear port %u reset change, errorcode: %d\r\n", port, ret);
                        }
                    }

                    if (portstatus & HUB_PORT_STATUS_HIGH_SPEED) {
                        speed = USB_SPEED_HIGH;
                    } else if (portstatus & HUB_PORT_STATUS_LOW_SPEED) {
                        speed = USB_SPEED_LOW;
                    } else {
                        speed = USB_SPEED_FULL;
                    }

                    memset(child, 0, sizeof(struct usbh_hubport));
                    child->speed = speed;
                    usbh_hub_events_connect_handle(hub, child, port);
                } else {
                    USB_LOG_ERR("Failed to enable port %u\r\n", port + 1);
                    continue;
                }
            } 
            else {
                usbh_hub_events_disconnect_handle(hub, child, port);
            }
        } else if(portchange & HUB_PORT_STATUS_C_RESET) {

            if (portstatus & HUB_PORT_STATUS_HIGH_SPEED) {
                speed = USB_SPEED_HIGH;
            } else if (portstatus & HUB_PORT_STATUS_LOW_SPEED) {
                speed = USB_SPEED_LOW;
            } else {
                speed = USB_SPEED_FULL;
            }
            memset(child, 0, sizeof(struct usbh_hubport));
            child->speed = speed;
            usbh_hub_events_connect_handle(hub, child, port);
        } else {
          USB_LOG_ERR("Failed to enable port %u, status:0x%02x, change:0x%02x\r\n", port + 1, portstatus, portchange);
        }
    }

    /* Start next hub int transfer */
    USB_LOG_VBS("%s Start next hub int transfer roothub:%u, connected:%u\r\n", __func__, hub->is_roothub, hub->connected);
    if (!hub->is_roothub && hub->connected) {
        hub->intin_urb.transfer_buffer_length = 1;
        usbh_submit_urb(&hub->intin_urb);
    }
}

static void usbh_hub_thread(void *argument)
{
    usb_hc_init();
    while (1) {
        if(hub_event_queue != NULL) {
            hub_event_queue_t msg;
            int ret = 0;
            ret = rtos_pop_from_queue(&hub_event_queue, &msg, BEKEN_WAIT_FOREVER);
            if(kNoErr == ret) {
                if(msg.callback != NULL) {
                    msg.callback();
                }
                usbh_hub_events((struct usbh_hub *)msg.arg);
            }
        }
    }
}

void usbh_hub_register(struct usbh_hub *hub)
{
    USB_LOG_VBS("[+]%s\r\n", __func__);
    usb_slist_add_tail(&hub_class_head, &hub->list);
    USB_LOG_VBS("[-]%s\r\n", __func__);
}

void usbh_hub_unregister(struct usbh_hub *hub)
{
    usb_slist_remove(&hub_class_head, &hub->list);
}

const struct usbh_class_driver s_hub_driver = {
    .driver_name = "hub",
    .connect = usbh_hub_connect,
    .disconnect = usbh_hub_disconnect
};

const struct usbh_class_info s_hub_info = {
    .match_flags = USB_CLASS_MATCH_INTF_CLASS,
    .class = USB_DEVICE_CLASS_HUB,
    .subclass = 0,
    .protocol = 0,
    .vid = 0x00,
    .pid = 0x00,
    .class_driver = &s_hub_driver
};

int usbh_hub_initialize(void)
{
    USB_LOG_VBS("[+]%s\r\n", __func__);

    usbh_roothub_register();

    uint32_t os_create_fail_flag = 0;

    do {
        rtos_init_queue(&hub_event_queue, "hub_event_queue", sizeof(hub_event_queue_t), HUB_EVENT_QITEM_COUNT);
        if (hub_event_queue == NULL) {
            USB_LOG_ERR("%s create queue fail\r\n", __func__);
            os_create_fail_flag = 1;
        }

        hub_event_wait = usb_osal_sem_create(1);
        if (hub_event_wait == NULL) {
            USB_LOG_ERR("%s create sem fail\r\n", __func__);
            os_create_fail_flag = 1;
        }

        hub_event_mutex = usb_osal_mutex_create();
        if (hub_event_mutex == NULL) {
            USB_LOG_ERR("%s create mutex fail\r\n", __func__);
            os_create_fail_flag = 1;
        }

        hub_thread = usb_osal_thread_create("usbh_hub", CONFIG_USBHOST_PSC_STACKSIZE, CONFIG_USBHOST_PSC_PRIO, usbh_hub_thread, NULL);
        if (hub_thread == NULL) {
            USB_LOG_ERR("%s create thread fail\r\n", __func__);
            os_create_fail_flag = 1;
        }
    }while(0);

    if(os_create_fail_flag) {
        if(hub_event_queue) {
            rtos_deinit_queue(&hub_event_queue);
            hub_event_queue = NULL;
        }

        if (hub_event_wait) {
            usb_osal_sem_delete(&hub_event_wait);
            hub_event_wait = NULL;
        }

        if (hub_event_mutex) {
            usb_osal_mutex_delete(&hub_event_mutex);
            hub_event_mutex = NULL;
        }

        if (hub_thread) {
            usb_osal_thread_delete(&hub_thread);
            hub_thread = NULL;
        }
    }

    return 0;
}

int usbh_hub_deinitialize(void)
{
    USB_LOG_VBS("[+]%s\r\n", __func__);

    usbh_hub_event_lock_mutex();

    usb_hc_deinit();

    uint32_t flags = usb_hc_enter_critical();
    usbh_roothub_free_port1_hub();
    if(hub_event_queue) {
        rtos_deinit_queue(&hub_event_queue);
        hub_event_queue = NULL;
    }

    if (hub_event_wait) {
        usb_osal_sem_delete(&hub_event_wait);
        hub_event_wait = NULL;
    }

    if (hub_thread) {
        usb_osal_thread_delete(&hub_thread);
        hub_thread = NULL;
    }
    usb_hc_exit_critical(flags);
    usbh_hub_event_unlock_mutex();
    if (hub_event_mutex) {
        usb_osal_mutex_delete(&hub_event_mutex);
        hub_event_mutex = NULL;
    }
    //usb_hc_deinit();
    usbh_roothub_unregister();

    USB_LOG_VBS("[-]%s\r\n", __func__);
    return 0;
}

const struct usbh_class_driver hub_driver = {
    .driver_name = "hub",
    .connect = usbh_hub_connect,
    .disconnect = usbh_hub_disconnect
};

CLASS_INFO_DEFINE const struct usbh_class_info hub_info = {
    .match_flags = USB_CLASS_MATCH_INTF_CLASS,
    .class = USB_DEVICE_CLASS_HUB,
    .subclass = 0,
    .protocol = 0,
    .vid = 0x00,
    .pid = 0x00,
    .class_driver = &hub_driver
};

void usbh_hub_class_register()
{
	usbh_register_class_driver(0, (void *)&hub_info);
}

