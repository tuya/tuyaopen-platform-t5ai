#include <components/cherryusb/usbd_core.h>
#include <components/cherryusb/usbd_hid.h>
#include <driver/gpio.h>
#include <driver/hal/hal_gpio_types.h>
#include "gpio_driver.h"

/*!< endpoint address */
#define HID_INT_EP          0x81

#define HID_INT_EP_SIZE     4
#define HID_INT_EP_INTERVAL 10

#define USBD_VID           0x0000
#define USBD_PID           0x0000
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

/*!< config descriptor size */
#define USB_HID_CONFIG_DESC_SIZ 34//0x22
/*!< report descriptor size */
// #define HID_MOUSE_REPORT_DESC_SIZE 74
#define HID_MOUSE_REPORT_DESC_SIZE 46

#define HID_MOUSE_NUMBER_INTERFACES 0x01

/*!< global descriptor */
const uint8_t hid_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0002, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_HID_CONFIG_DESC_SIZ, HID_MOUSE_NUMBER_INTERFACES, 0x01, USB_CONFIG_REMOTE_WAKEUP, USBD_MAX_POWER),

    /************** Descriptor of Joystick Mouse interface ****************/
    /* 09 */
    0x09,                          /* bLength: Interface Descriptor size */
    USB_DESCRIPTOR_TYPE_INTERFACE, /* bDescriptorType: Interface descriptor type */
    0x00,                          /* bInterfaceNumber: Number of Interface */
    0x00,                          /* bAlternateSetting: Alternate setting */
    0x01,                          /* bNumEndpoints */
    0x03,                          /* bInterfaceClass: HID */
    0x01,                          /* bInterfaceSubClass : 1=BOOT, 0=no boot */
    0x02,                          /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
    0,                             /* iInterface: Index of string descriptor */
    /******************** Descriptor of Joystick Mouse HID ********************/
    /* 18 */
    0x09,                    /* bLength: HID Descriptor size */
    HID_DESCRIPTOR_TYPE_HID, /* bDescriptorType: HID */
    0x11,                    /* bcdHID: HID Class Spec release number */
    0x01,
    0x00,                       /* bCountryCode: Hardware target country */
    0x01,                       /* bNumDescriptors: Number of HID class descriptors to follow */
    0x22,                       /* bDescriptorType */
    HID_MOUSE_REPORT_DESC_SIZE, /* wItemLength: Total length of Report descriptor */
    0x00,
    /******************** Descriptor of Mouse endpoint ********************/
    /* 27 */
    0x07,                         /* bLength: Endpoint Descriptor size */
    USB_DESCRIPTOR_TYPE_ENDPOINT, /* bDescriptorType: */
    HID_INT_EP,                   /* bEndpointAddress: Endpoint Address (IN) */
    0x03,                         /* bmAttributes: Interrupt endpoint */
    HID_INT_EP_SIZE,              /* wMaxPacketSize: 4 Byte max */
    0x00,
    HID_INT_EP_INTERVAL, /* bInterval: Polling Interval */

    /* 34 */
    ///////////////////////////////////////
    /// string0 descriptor
    ///////////////////////////////////////
    USB_LANGID_INIT(USBD_LANGID_STRING),
    ///////////////////////////////////////
    /// string1 descriptor
    ///////////////////////////////////////
    0x14,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'B', 0x00,                  /* wcChar0 */
    'E', 0x00,                  /* wcChar1 */
    'K', 0x00,                  /* wcChar2 */
    'E', 0x00,                  /* wcChar3 */
    'N', 0x00,                  /* wcChar4 */
    '-', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ///////////////////////////////////////
    /// string2 descriptor
    ///////////////////////////////////////
    0x26,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ' ', 0x00,                  /* wcChar9 */
    'H', 0x00,                  /* wcChar10 */
    'I', 0x00,                  /* wcChar11 */
    'D', 0x00,                  /* wcChar12 */
    ' ', 0x00,                  /* wcChar13 */
    'D', 0x00,                  /* wcChar14 */
    'E', 0x00,                  /* wcChar15 */
    'M', 0x00,                  /* wcChar16 */
    'O', 0x00,                  /* wcChar17 */
    ///////////////////////////////////////
    /// string3 descriptor
    ///////////////////////////////////////
    0x16,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    '2', 0x00,                  /* wcChar0 */
    '0', 0x00,                  /* wcChar1 */
    '2', 0x00,                  /* wcChar2 */
    '5', 0x00,                  /* wcChar3 */
    '0', 0x00,                  /* wcChar4 */
    '3', 0x00,                  /* wcChar5 */
    '1', 0x00,                  /* wcChar6 */
    '1', 0x00,                  /* wcChar7 */
    '1', 0x00,                  /* wcChar8 */
    '6', 0x00,                  /* wcChar9 */
#ifdef CONFIG_USB_HS
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x00,
    0x00,
#endif

    0x00
};

static const uint8_t hid_mouse_bos_desc[] = {
    0x05,
    USB_DESCRIPTOR_TYPE_BINARY_OBJECT_STORE,
    0x0c & 0xff,
    (0x0c & 0xff00) >> 8,
    0x01,
    ///////////////////////////////////////
    /// USB 2.0 Extension Descriptor
    ///////////////////////////////////////
    0x07,
    USB_DESCRIPTOR_TYPE_DEVICE_CAPABILITY,
    0x02,
    0x06, 0x00, 0x00, 0x00,
};

static struct usb_bos_descriptor hid_bos_descriptor = {
    .string = (uint8_t *)hid_mouse_bos_desc,
    .string_len = 0xc,
};

static const uint8_t hid_mouse_report_desc[HID_MOUSE_REPORT_DESC_SIZE] = {
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x02, // USAGE (Mouse)
    0xA1, 0x01, // COLLECTION (Application)
    0x09, 0x01, //   USAGE (Pointer)

    0xA1, 0x00, //   COLLECTION (Physical)
    0x05, 0x09, //     USAGE_PAGE (Button)
    0x19, 0x01, //     USAGE_MINIMUM (Button 1)
    0x29, 0x03, //     USAGE_MAXIMUM (Button 3)

    0x15, 0x00, //     LOGICAL_MINIMUM (0)
    0x25, 0x01, //     LOGICAL_MAXIMUM (1)
    0x95, 0x08, //     REPORT_COUNT (3)
    0x75, 0x01, //     REPORT_SIZE (1)

    0x81, 0x02, //     INPUT (Data,Var,Abs)

    0x05, 0x01, //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30, //     USAGE (X)
    0x09, 0x31, //     USAGE (Y)

    0x09, 0x38, //     USAGE (Mouse)

    0x15, 0x81, //     LOGICAL_MINIMUM (-127)
    0x25, 0x7F, //     LOGICAL_MAXIMUM (127)
    0x75, 0x08, //     REPORT_SIZE (8)
    0x95, 0x03, //     REPORT_COUNT (2)

    0x81, 0x06, //     INPUT (Data,Var,Rel)
    0xc0, 0xc0  //   END_COLLECTION
};

/*!< mouse report struct */
struct hid_mouse {
    uint8_t buttons;
    int8_t x;
    int8_t y;
    int8_t wheel;
};

/*!< mouse report */
static struct hid_mouse mouse_cfg;

#define HID_STATE_IDLE 0
#define HID_STATE_BUSY 1
#define HID_REMOTE_WAKEUP_TEST 1
#define HID_MOUSE_GPIO_SIM_LEFT_RIGHT_KEY 1
#define HID_MOUSE_THREAD_ENABLE 1

#if HID_MOUSE_GPIO_SIM_LEFT_RIGHT_KEY
#define USB_HID_MOUSE_LEFT_GPIO_ID GPIO_12
#define USB_HID_MOUSE_RIGHT_GPIO_ID GPIO_13
#define USBD_HID_MOUSE_CHECK_KEYUP_MS 100

static beken_timer_t s_simulate_leftkeyup_tmr = {0};
static beken_timer_t s_simulate_rightkeyup_tmr = {0};

static struct hid_mouse mouse_left_cfg;
static struct hid_mouse mouse_right_cfg;
void hid_mouse_L_R_key_init();
void hid_mouse_L_R_key_deinit();
#endif

#if HID_MOUSE_THREAD_ENABLE
static beken_thread_t  s_hid_mouse_thread_hdl = NULL;
static beken_queue_t s_hid_mouse_event_queue = NULL;
#define HID_MOUSE_EVENT_QITEM_COUNT 100
#define HID_MOUSE_THREAD_SIZE 1024
#define HID_MOUSE_THREAD_PRIO 4

typedef struct {
    uint32_t hid_mouse_input_button;
    void *cfg;
} hid_mouse_event_queue_t;

#endif
/*!< hid state ! Data can be sent only when state is idle  */
static volatile uint8_t hid_state = HID_STATE_IDLE;

/* function ------------------------------------------------------------------*/
static void usbd_hid_int_callback(uint8_t ep, uint32_t nbytes)
{
    //USB_LOG_DBG("%s ep:0x%x nbyres:%d\n", __func__, ep, nbytes);

    hid_state = HID_STATE_IDLE;
}

/*!< endpoint call back */
static struct usbd_endpoint hid_in_ep = {
    .ep_cb = usbd_hid_int_callback,
    .ep_addr = HID_INT_EP
};

static struct usbd_interface intf0;
static uint8_t s_usbd_hid_mouse_is_init = false;

void hid_mouse_test(void);

#if HID_MOUSE_THREAD_ENABLE
static void hid_mouse_left_press(struct hid_mouse *cfg)
{
    if(cfg) {
        mouse_cfg.buttons = cfg->buttons;
        mouse_cfg.x = cfg->x;
        mouse_cfg.y = cfg->y;
        mouse_cfg.wheel = cfg->wheel;
    } else {
        mouse_cfg.buttons = HID_MOUSE_INPUT_BUTTON_LEFT;
        /* cfg param by sensor*/
        mouse_cfg.x = 0;
        mouse_cfg.y = 0;
        mouse_cfg.wheel = 0;
    }

    if(mouse_cfg.buttons != 0){
#if HID_MOUSE_GPIO_SIM_LEFT_RIGHT_KEY
        BK_LOG_ON_ERR(bk_gpio_enable_interrupt(USB_HID_MOUSE_LEFT_GPIO_ID));
        if(rtos_is_timer_running(&s_simulate_leftkeyup_tmr)) {
            rtos_reload_timer(&s_simulate_leftkeyup_tmr);
        } else {
            rtos_start_timer(&s_simulate_leftkeyup_tmr);
        }
#endif
    } else {
#if HID_MOUSE_GPIO_SIM_LEFT_RIGHT_KEY
        BK_LOG_ON_ERR(bk_gpio_enable_interrupt(USB_HID_MOUSE_LEFT_GPIO_ID));
        rtos_stop_timer(&s_simulate_leftkeyup_tmr);
#endif
    }
}

static void hid_mouse_right_press(struct hid_mouse *cfg)
{
    if(cfg) {
        mouse_cfg.buttons = cfg->buttons;
        mouse_cfg.x = cfg->x;
        mouse_cfg.y = cfg->y;
        mouse_cfg.wheel = cfg->wheel;
    } else {
        mouse_cfg.buttons = HID_MOUSE_INPUT_BUTTON_RIGHT;
        /* cfg param by sensor*/
        mouse_cfg.x = 0;
        mouse_cfg.y = 0;
        mouse_cfg.wheel = 0;
    }

    if(mouse_cfg.buttons != 0){
#if HID_MOUSE_GPIO_SIM_LEFT_RIGHT_KEY
        BK_LOG_ON_ERR(bk_gpio_enable_interrupt(USB_HID_MOUSE_RIGHT_GPIO_ID));
        if(rtos_is_timer_running(&s_simulate_rightkeyup_tmr)) {
            rtos_reload_timer(&s_simulate_rightkeyup_tmr);
        } else {
            rtos_start_timer(&s_simulate_rightkeyup_tmr);
        }
#endif
    } else {
#if HID_MOUSE_GPIO_SIM_LEFT_RIGHT_KEY
        BK_LOG_ON_ERR(bk_gpio_enable_interrupt(USB_HID_MOUSE_RIGHT_GPIO_ID));
        rtos_stop_timer(&s_simulate_rightkeyup_tmr);
#endif
    }
}

static void hid_mouse_middle_press(struct hid_mouse *cfg)
{
    if(cfg) {
        mouse_cfg.buttons = cfg->buttons;
        mouse_cfg.x = cfg->x;
        mouse_cfg.y = cfg->y;
        mouse_cfg.wheel = cfg->wheel;
    } else {
        mouse_cfg.buttons = HID_MOUSE_INPUT_BUTTON_MIDDLE;
        /* cfg param by sensor*/
        mouse_cfg.x = 0;
        mouse_cfg.y = 0;
        mouse_cfg.wheel = 0;
    }
}

static void usbd_hid_mouse_events(uint32_t button, void *cfg)
{
    switch (button) {
        case HID_MOUSE_INPUT_BUTTON_LEFT: {
            hid_mouse_left_press((struct hid_mouse *)cfg);
        } break;

        case HID_MOUSE_INPUT_BUTTON_RIGHT: {
            hid_mouse_right_press((struct hid_mouse *)cfg);
        } break;

        case HID_MOUSE_INPUT_BUTTON_MIDDLE: {
            hid_mouse_middle_press((struct hid_mouse *)cfg);
        } break;

        default:
            break;
    }

    hid_state = HID_STATE_BUSY;
    int ret = usbd_ep_start_write(HID_INT_EP, (uint8_t *)&mouse_cfg, 4);
    if (ret < 0) {
        return;
    }
    while (hid_state == HID_STATE_BUSY) {
        rtos_delay_milliseconds(20);
    }
}

static int usbd_hid_mouse_send_queue(uint32_t button, void *cfg)
{
    bk_err_t ret;
    hid_mouse_event_queue_t msg;

    msg.hid_mouse_input_button = button;
    msg.cfg = cfg;

    if (s_hid_mouse_event_queue) {
        ret = rtos_push_to_queue(&s_hid_mouse_event_queue, &msg, 0);
        if (kNoErr != ret) {
            rtos_reset_queue(&s_hid_mouse_event_queue);
            return BK_FAIL;
        }
        return ret;
    }
    return BK_OK;
}

static void usbd_hid_mouse_thread(void *argument)
{
#if HID_MOUSE_GPIO_SIM_LEFT_RIGHT_KEY
    hid_mouse_L_R_key_init();
#endif

    while (1) {
        if(s_hid_mouse_event_queue != NULL) {
            hid_mouse_event_queue_t msg;
            int ret = 0;
            ret = rtos_pop_from_queue(&s_hid_mouse_event_queue, &msg, BEKEN_WAIT_FOREVER);
            if(kNoErr == ret) {
                usbd_hid_mouse_events(msg.hid_mouse_input_button, (void *)msg.cfg);
            }
        }
    }
}

void hid_mouse_thread_init()
{
    uint32_t os_create_fail_flag = 0;
    do {
        rtos_init_queue(&s_hid_mouse_event_queue, "usbd_mouse_queue", sizeof(hid_mouse_event_queue_t), HID_MOUSE_EVENT_QITEM_COUNT);
        if (s_hid_mouse_event_queue == NULL) {
            USB_LOG_ERR("%s create queue fail\r\n", __func__);
            os_create_fail_flag = 1;
        }

        rtos_create_thread(&s_hid_mouse_thread_hdl, HID_MOUSE_THREAD_PRIO, "usbd_mouse", (beken_thread_function_t)usbd_hid_mouse_thread, HID_MOUSE_THREAD_SIZE, NULL);
        if (s_hid_mouse_thread_hdl == NULL) {
            USB_LOG_ERR("%s create thread fail\r\n", __func__);
            os_create_fail_flag = 1;
        }
    } while(0);

    if(os_create_fail_flag) {
        if(s_hid_mouse_event_queue) {
            rtos_deinit_queue(&s_hid_mouse_event_queue);
            s_hid_mouse_event_queue = NULL;
        }
        if (s_hid_mouse_thread_hdl) {
            rtos_delete_thread(&s_hid_mouse_thread_hdl);
            s_hid_mouse_thread_hdl = NULL;
        }
    }
}

void hid_mouse_thread_deinit()
{
#if HID_MOUSE_GPIO_SIM_LEFT_RIGHT_KEY
    hid_mouse_L_R_key_deinit();
#endif

    if (s_hid_mouse_thread_hdl) {
        rtos_delete_thread(&s_hid_mouse_thread_hdl);
        s_hid_mouse_thread_hdl = NULL;
    }

    if(s_hid_mouse_event_queue) {
        rtos_deinit_queue(&s_hid_mouse_event_queue);
        s_hid_mouse_event_queue = NULL;
    }
}
#endif

#if HID_REMOTE_WAKEUP_TEST
#define USB_HID_MOUSE_WAKEUP_GPIO_ID GPIO_5
static void gpio_int_isr(gpio_id_t id)
{
    bk_gpio_clear_interrupt(id);
    usbd_remote_wakeup_from_L2_state();

    hid_mouse_test();
}
void hid_mouse_remote_wakeup_init()
{

    gpio_config_t cfg;
    gpio_int_type_t int_type = 0;
    
    cfg.io_mode = GPIO_INPUT_ENABLE;
    cfg.pull_mode = GPIO_PULL_UP_EN;
    cfg.func_mode = GPIO_SECOND_FUNC_DISABLE;
    
    int_type = GPIO_INT_TYPE_FALLING_EDGE;
    gpio_dev_unprotect_unmap(USB_HID_MOUSE_WAKEUP_GPIO_ID);
    bk_gpio_set_config(USB_HID_MOUSE_WAKEUP_GPIO_ID, &cfg);
    bk_gpio_register_isr(USB_HID_MOUSE_WAKEUP_GPIO_ID, gpio_int_isr);
    BK_LOG_ON_ERR(bk_gpio_set_interrupt_type(USB_HID_MOUSE_WAKEUP_GPIO_ID, int_type));
    BK_LOG_ON_ERR(bk_gpio_enable_interrupt(USB_HID_MOUSE_WAKEUP_GPIO_ID));
}
void hid_mouse_remote_wakeup_deinit()
{
    gpio_dev_unprotect_unmap(USB_HID_MOUSE_WAKEUP_GPIO_ID);
}
#endif

#if HID_MOUSE_GPIO_SIM_LEFT_RIGHT_KEY
static void hid_mouse_simulate_leftkeyup_timer_isr()
{
    mouse_left_cfg.buttons = 0;
    mouse_left_cfg.x = 0;
    mouse_left_cfg.y = 0;
    mouse_left_cfg.wheel = 0;

#if HID_MOUSE_THREAD_ENABLE
    usbd_hid_mouse_send_queue(HID_MOUSE_INPUT_BUTTON_LEFT, (void *)&mouse_left_cfg);
#endif
}

static void hid_mouse_simulate_leftkeyup_init()
{
    bk_err_t err;

    if(s_simulate_leftkeyup_tmr.handle != NULL)
    {
        err = rtos_deinit_timer(&s_simulate_leftkeyup_tmr);
        BK_ASSERT(kNoErr == err);
        s_simulate_leftkeyup_tmr.handle = NULL;
    }

    err = rtos_init_timer(&s_simulate_leftkeyup_tmr, USBD_HID_MOUSE_CHECK_KEYUP_MS,hid_mouse_simulate_leftkeyup_timer_isr,(void *)0);
    BK_ASSERT(kNoErr == err);

}

static void hid_mouse_simulate_leftkeyup_deinit()
{
    bk_err_t err;

    if(rtos_is_timer_running(&s_simulate_leftkeyup_tmr)) {
        rtos_stop_timer(&s_simulate_leftkeyup_tmr);
     }

    if(s_simulate_leftkeyup_tmr.handle != NULL)
    {
        if(rtos_is_timer_running(&s_simulate_leftkeyup_tmr)) {
            rtos_stop_timer(&s_simulate_leftkeyup_tmr);
        }
        err = rtos_deinit_timer(&s_simulate_leftkeyup_tmr);
        BK_ASSERT(kNoErr == err);
        s_simulate_leftkeyup_tmr.handle = NULL;
    }
}

static void hid_mouse_simulate_rightkeyup_timer_isr()
{
    mouse_right_cfg.buttons = 0;
    mouse_right_cfg.x = 0;
    mouse_right_cfg.y = 0;
    mouse_right_cfg.wheel = 0;

#if HID_MOUSE_THREAD_ENABLE
    usbd_hid_mouse_send_queue(HID_MOUSE_INPUT_BUTTON_RIGHT, (void *)&mouse_right_cfg);
#endif

}

static void hid_mouse_simulate_rightkeyup_init()
{
    bk_err_t err;

    if(s_simulate_rightkeyup_tmr.handle != NULL)
    {
        err = rtos_deinit_timer(&s_simulate_rightkeyup_tmr);
        BK_ASSERT(kNoErr == err);
        s_simulate_rightkeyup_tmr.handle = NULL;
    }

    err = rtos_init_timer(&s_simulate_rightkeyup_tmr, USBD_HID_MOUSE_CHECK_KEYUP_MS,hid_mouse_simulate_rightkeyup_timer_isr,(void *)0);
    BK_ASSERT(kNoErr == err);

}

static void hid_mouse_simulate_rightkeyup_deinit()
{
    bk_err_t err;

    if(s_simulate_rightkeyup_tmr.handle != NULL)
    {
        if(rtos_is_timer_running(&s_simulate_rightkeyup_tmr)) {
            rtos_stop_timer(&s_simulate_rightkeyup_tmr);
        }
        err = rtos_deinit_timer(&s_simulate_rightkeyup_tmr);
        BK_ASSERT(kNoErr == err);
        s_simulate_rightkeyup_tmr.handle = NULL;
    }
}


static void hid_mouse_left_gpio_int_isr(gpio_id_t id)
{
    bk_gpio_clear_interrupt(id);
#if HID_MOUSE_THREAD_ENABLE
    BK_LOG_ON_ERR(bk_gpio_disable_interrupt(USB_HID_MOUSE_LEFT_GPIO_ID));
    if(s_usbd_hid_mouse_is_init) {
        usbd_hid_mouse_send_queue(HID_MOUSE_INPUT_BUTTON_LEFT, NULL);
    }
#endif
}

static void hid_mouse_right_gpio_int_isr(gpio_id_t id)
{
    bk_gpio_clear_interrupt(id);
#if HID_MOUSE_THREAD_ENABLE
    BK_LOG_ON_ERR(bk_gpio_disable_interrupt(USB_HID_MOUSE_RIGHT_GPIO_ID));
    if(s_usbd_hid_mouse_is_init) {
        usbd_hid_mouse_send_queue(HID_MOUSE_INPUT_BUTTON_RIGHT, NULL);
    }
#endif
}

void hid_mouse_L_R_key_init()
{
    /*left key*/
    gpio_config_t left_cfg;
    gpio_int_type_t left_int_type = 0;

    left_cfg.io_mode = GPIO_INPUT_ENABLE;
    left_cfg.pull_mode = GPIO_PULL_DOWN_EN;
    left_cfg.func_mode = GPIO_SECOND_FUNC_DISABLE;

    left_int_type = GPIO_INT_TYPE_HIGH_LEVEL;
    gpio_dev_unprotect_unmap(USB_HID_MOUSE_LEFT_GPIO_ID);
    bk_gpio_clear_interrupt(USB_HID_MOUSE_LEFT_GPIO_ID);
    bk_gpio_set_config(USB_HID_MOUSE_LEFT_GPIO_ID, &left_cfg);
    bk_gpio_register_isr(USB_HID_MOUSE_LEFT_GPIO_ID, hid_mouse_left_gpio_int_isr);
    BK_LOG_ON_ERR(bk_gpio_set_interrupt_type(USB_HID_MOUSE_LEFT_GPIO_ID, left_int_type));

    BK_LOG_ON_ERR(bk_gpio_enable_interrupt(USB_HID_MOUSE_LEFT_GPIO_ID));

    hid_mouse_simulate_leftkeyup_init();

    /*right key*/
    gpio_config_t right_cfg;
    gpio_int_type_t right_int_type = 0;

    right_cfg.io_mode = GPIO_INPUT_ENABLE;
    right_cfg.pull_mode = GPIO_PULL_DOWN_EN;
    right_cfg.func_mode = GPIO_SECOND_FUNC_DISABLE;

    right_int_type = GPIO_INT_TYPE_HIGH_LEVEL;
    gpio_dev_unprotect_unmap(USB_HID_MOUSE_RIGHT_GPIO_ID);
    bk_gpio_clear_interrupt(USB_HID_MOUSE_RIGHT_GPIO_ID);
    bk_gpio_set_config(USB_HID_MOUSE_RIGHT_GPIO_ID, &right_cfg);
    bk_gpio_register_isr(USB_HID_MOUSE_RIGHT_GPIO_ID, hid_mouse_right_gpio_int_isr);
    BK_LOG_ON_ERR(bk_gpio_set_interrupt_type(USB_HID_MOUSE_RIGHT_GPIO_ID, right_int_type));
    BK_LOG_ON_ERR(bk_gpio_enable_interrupt(USB_HID_MOUSE_RIGHT_GPIO_ID));

    hid_mouse_simulate_rightkeyup_init();
}
void hid_mouse_L_R_key_deinit()
{
    hid_mouse_simulate_leftkeyup_deinit();
    BK_LOG_ON_ERR(bk_gpio_disable_interrupt(USB_HID_MOUSE_LEFT_GPIO_ID));
    bk_gpio_register_isr(USB_HID_MOUSE_LEFT_GPIO_ID, NULL);

    hid_mouse_simulate_rightkeyup_deinit();
    BK_LOG_ON_ERR(bk_gpio_disable_interrupt(USB_HID_MOUSE_RIGHT_GPIO_ID));
    bk_gpio_register_isr(USB_HID_MOUSE_RIGHT_GPIO_ID, NULL);
}
#endif

void bk_usbd_hid_mouse_init(void)
{
    if (s_usbd_hid_mouse_is_init) {
        return;
    }

    usbd_desc_register(hid_descriptor);
    usbd_bos_desc_register(&hid_bos_descriptor);
    usbd_add_interface(usbd_hid_init_intf(&intf0, hid_mouse_report_desc, HID_MOUSE_REPORT_DESC_SIZE));
    usbd_add_endpoint(&hid_in_ep);

    usbd_initialize();

    /*!< init mouse report data */
    mouse_cfg.buttons = 0;
    mouse_cfg.wheel = 0;
    mouse_cfg.x = 0;
    mouse_cfg.y = 0;

#if HID_MOUSE_THREAD_ENABLE
    hid_mouse_thread_init();
#endif

#if HID_REMOTE_WAKEUP_TEST
    hid_mouse_remote_wakeup_init();
#endif
	
    s_usbd_hid_mouse_is_init = true;
}

void bk_usbd_hid_mouse_deinit(void)
{
    if(s_usbd_hid_mouse_is_init)
    {
        usbd_deinitialize();
#if HID_REMOTE_WAKEUP_TEST
        hid_mouse_remote_wakeup_deinit();
#endif

#if HID_MOUSE_THREAD_ENABLE
        hid_mouse_thread_deinit();
#endif
        s_usbd_hid_mouse_is_init = 0;
    }
    else
    {
        return ;
    }
}

/**
  * @brief            hid mouse test
  * @pre              none
  * @param[in]        none
  * @retval           none
  */
void hid_mouse_test(void)
{
    /*!< click mouse */
    mouse_cfg.buttons = 1;
    mouse_cfg.x = 0;
    mouse_cfg.y = 0;
    mouse_cfg.wheel = 0;

    int ret = usbd_ep_start_write(HID_INT_EP, (uint8_t *)&mouse_cfg, 4);
    if (ret < 0) {
        return;
    }
    hid_state = HID_STATE_BUSY;
    while (hid_state == HID_STATE_BUSY) {
    }
}

#define CURSOR_STEP  10U//2U
#define CURSOR_WIDTH 20U

void draw_circle(struct hid_mouse *cfg, uint8_t c_button)
{
    static int32_t move_cnt = 0;
    static uint8_t step_x_y = 0;
    static int8_t x = 0, y = 0;
    uint8_t buttons = 0;
    int8_t ops = 0;
    move_cnt++;
    if (move_cnt > CURSOR_WIDTH) {
        step_x_y++;
        step_x_y = step_x_y % 4;
        move_cnt = 0;
    }
    switch (step_x_y) {
        case 0: {
            y = 0;
            x = CURSOR_STEP;
            buttons = c_button;
            ops = 1;
        } break;

        case 1: {
            x = 0;
            y = CURSOR_STEP;
            buttons = 0;
            ops = -1;
        } break;

        case 2: {
            y = 0;
            x = (int8_t)(-CURSOR_STEP);
            buttons = 0;
            ops = 1;
        } break;

        case 3: {
            x = 0;
            y = (int8_t)(-CURSOR_STEP);
            buttons = c_button;
            ops = -1;
        } break;
    }

    if(c_button == HID_MOUSE_INPUT_BUTTON_MIDDLE) {
        cfg->buttons = buttons;
        cfg->x = 0;
        cfg->y = 0;
        cfg->wheel = ops;
    } else {
        cfg->buttons = buttons;
        cfg->x = x;
        cfg->y = y;
        cfg->wheel = 0;
    }
}

/* https://cps-check.com/cn/polling-rate-check */
void bk_usbd_hid_mouse_cps_check_test(uint8_t c_button)
{
    uint8_t button = 0;

    switch (c_button) {
        case HID_MOUSE_INPUT_BUTTON_LEFT: {
            USB_LOG_DBG("USB TEST HID_MOUSE_INPUT_BUTTON_LEFT\r\n");
            button = HID_MOUSE_INPUT_BUTTON_LEFT;
        } break;

        case HID_MOUSE_INPUT_BUTTON_RIGHT: {
            USB_LOG_DBG("USB TEST HID_MOUSE_INPUT_BUTTON_RIGHT\r\n");
            button = HID_MOUSE_INPUT_BUTTON_RIGHT;

        } break;

        case HID_MOUSE_INPUT_BUTTON_MIDDLE: {
            USB_LOG_DBG("USB TEST HID_MOUSE_INPUT_BUTTON_MIDDLE\r\n");
            button = HID_MOUSE_INPUT_BUTTON_MIDDLE;
        } break;

        default:
            break;
    }

    int counter = 0;
    while (counter < 1000) {
        draw_circle(&mouse_cfg, button);
        hid_state = HID_STATE_BUSY;
        int ret = usbd_ep_start_write(HID_INT_EP, (uint8_t *)&mouse_cfg, 4);
        if (ret < 0) {
            return;
        }
        while (hid_state == HID_STATE_BUSY) {
            rtos_delay_milliseconds(20);
        }

        counter++;
    }
    mouse_cfg.buttons = 0;
    mouse_cfg.x = 0;
    mouse_cfg.y = 0;
    mouse_cfg.wheel = 0;

    int ret = usbd_ep_start_write(HID_INT_EP, (uint8_t *)&mouse_cfg, 4);
    if (ret < 0) {
        return;
    }
}

