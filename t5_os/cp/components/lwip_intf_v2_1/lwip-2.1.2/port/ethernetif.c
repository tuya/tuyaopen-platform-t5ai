// Copyright 2020-2023 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
  ******************************************************************************
  * File Name          : ethernetif.c
  * Description        : This file provides code for the configuration
  *                      of the ethernetif.c MiddleWare.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "common/bk_include.h"

#if CONFIG_ETH
#include <string.h>
#include "lwip/opt.h"
#include "lwip/timeouts.h"
#include "lwip/tcpip.h"
#include "lwip/ethip6.h"
#include "netif/ethernet.h"
#include "netif/etharp.h"
#include "ethernetif.h"
#include "lan8742.h"
#include "eth_mac.h"
#include "cache.h"
#include <driver/hal/hal_gpio_types.h>
#include "sys_ll.h"
#include "driver/int_types.h"
#include "driver/int.h"
#include "gpio_driver.h"
#include "lwip/netifapi.h"
#include "net.h"
#include "sys_hal.h"
#include "miiphy.h"
#include "os/mem.h"

#define ETH_MULTI_PHY_SUPPORT  1

/* Private define ------------------------------------------------------------*/
/* Stack size of the interface thread */
#define INTERFACE_THREAD_STACK_SIZE ( 1024 )
/* Network interface name */
#define IFNAME0 's'
#define IFNAME1 't'

/* ETH Setting  */
#define ETH_DMA_TRANSMIT_TIMEOUT               ( 20U )
#define ETH_TX_BUFFER_MAX                      ((ETH_TX_DESC_CNT) * 2U)
#define ETH_RX_BUFFER_SIZE                     ( 1536 )
#define ETH_TX_DESCS_PER_TX                    8

/* MAX RX threshold per process */
#define ETH_RX_FRAME_PREP_THD                  16

/* Private variables ---------------------------------------------------------*/
/*
@Note: This interface is implemented to operate in zero-copy mode only:
        - Rx buffers are allocated statically and passed directly to the LwIP stack
          they will return back to ETH DMA after been processed by the stack.
        - Tx Buffers will be allocated from LwIP stack memory heap,
          then passed to ETH HAL driver.

@Notes:
  1.a. ETH DMA Rx descriptors must be contiguous, the default count is 4,
       to customize it please redefine ETH_RX_DESC_CNT in ETH GUI (Rx Descriptor Length)
       so that updated value will be generated in stm32xxxx_hal_conf.h
  1.b. ETH DMA Tx descriptors must be contiguous, the default count is 4,
       to customize it please redefine ETH_TX_DESC_CNT in ETH GUI (Tx Descriptor Length)
       so that updated value will be generated in stm32xxxx_hal_conf.h

  2.a. Rx Buffers number must be between ETH_RX_DESC_CNT and 2*ETH_RX_DESC_CNT
  2.b. Rx Buffers must have the same size: ETH_RX_BUFFER_SIZE, this value must
       passed to ETH DMA in the init field (heth.Init.RxBuffLen)
  2.c  The RX Ruffers addresses and sizes must be properly defined to be aligned
       to L1-CACHE line size (32 bytes).
*/

/* Data Type Definitions */
typedef enum
{
  RX_ALLOC_OK       = 0x00,
  RX_ALLOC_ERROR    = 0x01
} RxAllocStatusTypeDef;

typedef struct
{
  struct pbuf_custom pbuf_custom;
  uint8_t buff[(ETH_RX_BUFFER_SIZE + 31) & ~31] __ALIGNED(32);
} RxBuff_t;


enum {
  BMSG_NULL_TYPE = 0,
  BMSG_RX_TYPE,
  BMSG_TX_TYPE,
  BMSG_TX_COMPLETE,
  BMSG_PS_KEEP_END,
};

typedef struct bus_message {
  uint32_t type;
  uint32_t arg;
} BUS_MSG_T;

#define ETH_PS_PREVENT_KEEP_TIMER  0x01
#define ETH_PS_PREVENT_LINK_CHECK  0x02
#define ETH_PS_PREVENT_DHCP        0x04

// private data
struct eth_mac_priv {
  uint8_t mac[6];

  struct mii_dev *mii;
  struct phy_device *phy;

  // Ethernet event Queue
  beken_queue_t eventq;

#ifdef CONFIG_ETH_PM_CB_SUPPORT
  // Ethernet TX counter of Tx path
  uint32_t tx_count;
  uint32_t rx_count;

  // HW enters PS ?
  bool hw_in_doze;

  // current phy speed and duplex
  uint32_t speed;
  uint32_t duplex;

  // bitwise flags that prevent Eth enters PS
  uint32_t ps_prevent;

  // linger timer that prevents Eth enters PS
  beken2_timer_t timer;

  // skip link check count after exit PS
  int skip_link_check_count;
#endif
};


/* Memory Pool Declaration */
#define ETH_RX_BUFFER_CNT             24U
LWIP_MEMPOOL_DECLARE(RX_POOL, ETH_RX_BUFFER_CNT, sizeof(RxBuff_t), "Zero-copy RX PBUF pool");

/* Variable Definitions */
static uint8_t RxAllocStatus;


static ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
static ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT];   /* Ethernet Tx DMA Descriptors */
extern u8_t memp_memory_RX_POOL_base[];
extern int32_t xTaskGetTickCount( void );
volatile int32_t bmsg_eth_rx_count = 0;
volatile int32_t bmsg_eth_tx_compl_count = 0;

/* Global Ethernet handle */
static ETH_HandleTypeDef heth;

/* Private function prototypes -----------------------------------------------*/
static int32_t ETH_PHY_IO_Init(void);
static int32_t ETH_PHY_IO_DeInit (void);
static int32_t ETH_PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal);
static int32_t ETH_PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal);
static int32_t ETH_PHY_IO_GetTick(void);
static void ethernetif_tx_cplt(void *argument);
static void ethernetif_core_thread(void *argument);
static void bmsg_eth_sender(int type);
static void bmsg_eth_null_sender();
static void bmsg_eth_rx_sender(void *arg);
static void bmsg_eth_tx_compl_sender(void *arg);
static int bmsg_eth_tx_sender(struct pbuf *p);
static int __HAL_ETH_Exit_LP();

#if !ETH_MULTI_PHY_SUPPORT
static lan8742_Object_t LAN8742;
static lan8742_IOCtx_t  LAN8742_IOCtx = {ETH_PHY_IO_Init,
                                  ETH_PHY_IO_DeInit,
                                  ETH_PHY_IO_WriteReg,
                                  ETH_PHY_IO_ReadReg,
                                  ETH_PHY_IO_GetTick};
#endif

/* Private functions ---------------------------------------------------------*/
static void pbuf_free_custom(struct pbuf *p);
static void ethernetif_input(void* argument);

/**
  * @brief  Ethernet Rx Transfer completed callback
  * @param  handlerEth: ETH handler
  * @retval None
  */
void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *handlerEth)
{
  bmsg_eth_rx_sender(0);
}

/**
  * @brief  Ethernet Tx Transfer completed callback
  * @param  handlerEth: ETH handler
  * @retval None
  */
void HAL_ETH_TxCpltCallback(ETH_HandleTypeDef *handlerEth)
{
  bmsg_eth_tx_compl_sender(0);
}

/**
  * @brief  Ethernet DMA transfer error callback
  * @param  handlerEth: ETH handler
  * @retval None
  */
void HAL_ETH_ErrorCallback(ETH_HandleTypeDef *handlerEth)
{
  if((HAL_ETH_GetDMAError(handlerEth) & ETH_DMACSR_RBU) == ETH_DMACSR_RBU)
  {
    bmsg_eth_rx_sender(0);
  }
}

void ETH_IRQHandler(void)
{
  //LWIP_LOGD("%s\n", __func__);
  HAL_ETH_IRQHandler(&heth);
}

#ifdef CONFIG_ETH_PM_CB_SUPPORT
// Eth keep timer expired, clear PS_KEEP prevent bit
static void power_save_keep_timer_handler(void *dummy1, void *dummy2)
{
  bmsg_eth_sender(BMSG_PS_KEEP_END);
}

// Eth keep timer
static void eth_start_keep_timer(uint32_t secs, uint32_t usecs)
{
  bk_err_t err;
  uint32_t clk_time;
  struct eth_mac_priv *priv = heth.priv;

  clk_time = (secs * 1000 + usecs / 1000);

  if (rtos_is_oneshot_timer_init(&priv->timer)) {
    rtos_oneshot_reload_timer(&priv->timer);
  } else {
    err = rtos_init_oneshot_timer(&priv->timer, clk_time,
          power_save_keep_timer_handler, NULL, NULL);
    BK_ASSERT(err == kNoErr);

    err = rtos_start_oneshot_timer(&priv->timer);
    BK_ASSERT(err == kNoErr);
  }
}
#endif // CONFIG_ETH_PM_CB_SUPPORT

int eqos_mdio_read(struct mii_dev *bus, int addr, int devad, int reg)
{
  ETH_HandleTypeDef *heth = bus->priv;
  uint32_t val;
  if (HAL_ETH_ReadPHYRegister(heth, addr, reg, &val) == HAL_OK)
    return val;
  return -1;
}

int eqos_mdio_write(struct mii_dev *bus, int addr, int devad, int reg, u16 val)
{
  ETH_HandleTypeDef *heth = bus->priv;

  if (HAL_ETH_WritePHYRegister(heth, addr, reg, val) == HAL_OK)
    return 0;
  return -1;
}

int eqos_mdio_reset(struct mii_dev *bus)
{
  ETH_HandleTypeDef *heth = bus->priv;
  /* We assume that MDIO GPIO configuration is already done
   in the ETH_MspInit() else it should be done here
  */

  /* Configure the MDIO Clock */
  HAL_ETH_SetMDIOClockRange(heth);

  return 0;
}

/*******************************************************************************
                       LL Driver Interface ( LwIP stack --> ETH)
*******************************************************************************/
/**
 * @brief In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static bk_err_t low_level_init(struct netif *netif)
{
  HAL_StatusTypeDef hal_eth_init_status = HAL_OK;
  bk_err_t ret = BK_OK;
  struct eth_mac_priv *priv;

  LWIP_LOGD("ETH %s start XXX\n", __func__);

  // Allocate eth_mac priv data
  priv = heth.priv = os_zalloc(sizeof(struct eth_mac_priv));
  if (!heth.priv) {
    LWIP_LOGE("%s OOM\n", __func__);
    return BK_FAIL;
  }

  /* Start ETH HAL Init */
  heth.Instance = ETH;
  bk_get_mac(priv->mac, MAC_TYPE_ETH);
  heth.Init.MACAddr = priv->mac;
  heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
  heth.Init.TxDesc = DMATxDscrTab;
  heth.Init.RxDesc = DMARxDscrTab;
  heth.Init.RxBuffLen = 1536;

  hal_eth_init_status = HAL_ETH_Init(&heth);

  /* Initialize the RX POOL */
  LWIP_MEMPOOL_INIT(RX_POOL);

#if LWIP_ARP || LWIP_ETHERNET

  /* set MAC hardware address length */
  netif->hwaddr_len = ETH_HWADDR_LEN;

  /* set MAC hardware address */
  netif->hwaddr[0] =  heth.Init.MACAddr[0];
  netif->hwaddr[1] =  heth.Init.MACAddr[1];
  netif->hwaddr[2] =  heth.Init.MACAddr[2];
  netif->hwaddr[3] =  heth.Init.MACAddr[3];
  netif->hwaddr[4] =  heth.Init.MACAddr[4];
  netif->hwaddr[5] =  heth.Init.MACAddr[5];

  /* maximum transfer unit */
  netif->mtu = ETH_MAX_PAYLOAD;

  /* Accept broadcast address and ARP traffic */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  #if LWIP_ARP
  netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
  #else
  netif->flags |= NETIF_FLAG_BROADCAST;
  #endif /* LWIP_ARP */

#if ETH_MULTI_PHY_SUPPORT
  if (!priv->mii) {
    priv->mii = mdio_alloc();
    if (!priv->mii) {
      BK_LOGD(NULL,"mdio_alloc() failed");
      ret = -1;
      goto fail;
    }
    priv->mii->read = eqos_mdio_read;
    priv->mii->write = eqos_mdio_write;
    priv->mii->reset = eqos_mdio_reset;
    priv->mii->priv = &heth;
    strncpy(priv->mii->name, netif->name, 2);

    ret = mdio_register(priv->mii);
    if (ret < 0) {
      BK_LOGD(NULL,"mdio_register() failed: %d", ret);
      goto fail;
    }
  }

  if (!priv->phy) {
    int addr = -1;

    priv->phy = phy_connect(priv->mii, addr, netif, PHY_INTERFACE_MODE_RMII);  // for BK7236, RMII, for BK7286, RGMII
    if (!priv->phy) {
      BK_LOGD(NULL,"phy_connect() failed\n");
      ret = -1;
      goto fail;
    }
    // phy_set_supported();
    phy_config(priv->phy);  // genphy_config
    phy_startup(priv->phy); // genphy_startup
  }
#else // ETH_MULTI_PHY_SUPPORT
  /* Set PHY IO functions */
  LAN8742_RegisterBusIO(&LAN8742, &LAN8742_IOCtx);

  /* Initialize the LAN8742 ETH PHY */
  LAN8742_Init(&LAN8742);

  if (LAN8742_GetAutoNego(&LAN8742))
  {
    LWIP_LOGD("AutoNego enabled\n");
  }
  else
  {
    LWIP_LOGD("AutoNego disabled\n");
  }
#endif // ETH_MULTI_PHY_SUPPORT

  // Init event Queue
  rtos_init_queue(&priv->eventq, "core_queue", sizeof(BUS_MSG_T), 64);

  /* create ethernet main thread */
  ret = rtos_create_thread(NULL, 1 /* BEKEN_APPLICATION_PRIORITY */, /* TBD: ETH priority */
      "EthIf", ethernetif_core_thread, 4096, netif);
  if (ret != BK_OK) {
    LWIP_LOGE("%s create EthIf thread failed: %d\n", __func__, ret);
    goto fail;
  }

  if (hal_eth_init_status != HAL_OK)
  {
    LWIP_LOGD("%s: Init ETH failed\n", __func__);

    ret = BK_FAIL;
    goto fail;
  }

  LWIP_LOGD("ETH %s end, ret %d\n", __func__, ret);

  return ret;

fail:
  // FIXME: cleanup
  if (priv) {
    if (priv->mii) {
      mdio_unregister(priv->mii);
      mdio_free(priv->mii);
      priv->mii = NULL;
    }
    os_free(priv);
  }

  return ret;
#endif /* LWIP_ARP || LWIP_ETHERNET */
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become available since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
  uint32_t i = 0U;
  struct pbuf *q = NULL;
  err_t errval = ERR_OK;
  ETH_BufferTypeDef *Txbuffer;
  ETH_TxPacketConfig *txconfig;

  txconfig = os_malloc(sizeof(*txconfig) + ETH_TX_DESCS_PER_TX * sizeof(ETH_BufferTypeDef));
  if (!txconfig) {
    return ERR_MEM;
  }
  Txbuffer = (ETH_BufferTypeDef *)(txconfig + 1);

  txconfig->Length = p->tot_len;
  txconfig->TxBuffer = Txbuffer;
  txconfig->pData = p;
  txconfig->Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
  txconfig->ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
  txconfig->CRCPadCtrl = ETH_CRC_PAD_INSERT;

  // Fill txl buffer control
  memset(Txbuffer, 0, ETH_TX_DESCS_PER_TX * sizeof(ETH_BufferTypeDef));

  for (q = p; q != NULL; q = q->next)
  {
    if (i >= ETH_TX_DESCS_PER_TX) {
      LWIP_LOGE("%s: txdesc overflow\n", __func__);
      os_free(txconfig);
      return ERR_IF;
    }

    Txbuffer[i].buffer = q->payload;
    Txbuffer[i].len = q->len;

    if (i > 0)
    {
      Txbuffer[i-1].next = &Txbuffer[i];
    }

    if (q->next == NULL)
    {
      Txbuffer[i].next = NULL;
    }

    i++;
  }

  // Put txconfig pointer in pbuf
  errval = pbuf_header(p, sizeof(txconfig));
  if (errval) {
    LWIP_LOGE("no headroom for pbuf\n");
    os_free(txconfig);
    return errval;
  }

  os_memcpy(p->payload, &txconfig, sizeof(txconfig));

  // Send to eth core thread
  return bmsg_eth_tx_sender(p);
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
   */
static struct pbuf * low_level_input(struct netif *netif)
{
  struct pbuf *p = NULL;

  if(RxAllocStatus == RX_ALLOC_OK)
  {
    HAL_ETH_ReadData(&heth, (void **)&p);
  }

  return p;
}

// Kick eth core thread run
static void bmsg_eth_null_sender()
{
  bk_err_t ret;
  BUS_MSG_T msg;
  struct eth_mac_priv *priv = heth.priv;

  msg.type = BMSG_NULL_TYPE;
  msg.arg = 0;

  if (!rtos_is_queue_empty(&priv->eventq))
    return;

  ret = rtos_push_to_queue(&priv->eventq, &msg, BEKEN_NO_WAIT);
  if (kNoErr != ret)
    LWIP_LOGE("%s failed, ret=%d\r\n", __func__, ret);
}

// send bus msg to eth core thread
static void bmsg_eth_sender(int type)
{
  bk_err_t ret;
  BUS_MSG_T msg;
  struct eth_mac_priv *priv = heth.priv;

  msg.type = type;
  msg.arg = 0;

  ret = rtos_push_to_queue(&priv->eventq, &msg, BEKEN_NO_WAIT);
  if (kNoErr != ret)
    LWIP_LOGE("%s failed, ret=%d\r\n", __func__, ret);
}

// send bus msg to eth core thread
static void bmsg_eth_rx_sender(void *arg)
{
  bk_err_t ret;
  BUS_MSG_T msg;
  struct eth_mac_priv *priv;

  GLOBAL_INT_DECLARATION();

  msg.type = BMSG_RX_TYPE;
  msg.arg = (uint32_t)arg;

  GLOBAL_INT_DISABLE();
  if (bmsg_eth_rx_count >= 2) {
    GLOBAL_INT_RESTORE();
    return;
  }

  bmsg_eth_rx_count += 1;
  GLOBAL_INT_RESTORE();

  priv = heth.priv;
  ret = rtos_push_to_queue(&priv->eventq, &msg, BEKEN_NO_WAIT);
  if (kNoErr != ret)
    LWIP_LOGE("%s failed, ret=%d\r\n", __func__, ret);
}

// send bus msg to eth core thread
static void bmsg_eth_tx_compl_sender(void *arg)
{
  bk_err_t ret;
  BUS_MSG_T msg;
  struct eth_mac_priv *priv;
  GLOBAL_INT_DECLARATION();

  msg.type = BMSG_TX_COMPLETE;
  msg.arg = (uint32_t)arg;

  GLOBAL_INT_DISABLE();
  if (bmsg_eth_tx_compl_count >= 2) {
    GLOBAL_INT_RESTORE();
    return;
  }

  bmsg_eth_tx_compl_count += 1;
  GLOBAL_INT_RESTORE();

  priv = heth.priv;
  ret = rtos_push_to_queue(&priv->eventq, &msg, BEKEN_NO_WAIT);
  if (kNoErr != ret)
    LWIP_LOGE("%s failed, ret=%d\r\n", __func__, ret);
}

// bus msg handler
static void bmsg_eth_rx_handler(BUS_MSG_T *msg, struct netif *netif)
{
  int processed = 0;
  struct pbuf *p = NULL;
  GLOBAL_INT_DECLARATION();

  GLOBAL_INT_DISABLE();
  if (bmsg_eth_rx_count > 0)
    bmsg_eth_rx_count -= 1;
  GLOBAL_INT_RESTORE();

  do {
    p = low_level_input(netif);
    if (p != NULL) {
      if (netif->input(p, netif) != ERR_OK)
        pbuf_free(p);

      // ....
      if (++processed > ETH_RX_FRAME_PREP_THD) {
        bmsg_eth_rx_sender(0);
        break;
      }
    } else {
      // BK_LOGD(NULL,"%s: no rx pbuf, RxAllocStatus %d\n", __func__, RxAllocStatus);
    }
  } while (p != NULL);
}

// bus msg handler
static int bmsg_eth_tx_sender(struct pbuf *p)
{
  bk_err_t ret;
  BUS_MSG_T msg;
  struct eth_mac_priv *priv;

  msg.type = BMSG_TX_TYPE;
  msg.arg = (uint32_t)p;

  pbuf_ref(p);
  priv = heth.priv;
  ret = rtos_push_to_queue(&priv->eventq, &msg, 1 * SECONDS);
  if (kNoErr != ret) {
    LWIP_LOGE("bmsg_tx_sender failed, ret=%d\r\n", ret);
    pbuf_free(p);
  }

  return ret;
}

// bus msg handler
static void bmsg_eth_tx_handler(BUS_MSG_T *msg)
{
  struct pbuf *p = (struct pbuf *)msg->arg;
  ETH_TxPacketConfig *txconfig;
  struct eth_mac_priv *priv __maybe_unused = heth.priv;

  if (!p) {
    LWIP_LOGE("no pbuf for tx\n");
    return;
  }

  // Get txconfig from pbuf
  os_memcpy(&txconfig, p->payload, sizeof(txconfig));
  // Remove txconfig from payload
  pbuf_remove_header(p, sizeof(txconfig));

  if (!txconfig) {
    LWIP_LOGE("pbuf has no txconfig\n");
    pbuf_free(p);
    return;
  }

  if (HAL_ETH_Transmit_IT(&heth, txconfig))  // FIXME: may failed, revise me
  {
    LWIP_LOGV("HAL_ETH_Transmit_IT failed\n");
    pbuf_free(p);
  }
  else
  {
#ifdef CONFIG_ETH_PM_CB_SUPPORT
     // Increase tx counter for LV
     priv->tx_count++;
#endif
  }

  // Free txconfig previous allocated in low_level_output
  os_free(txconfig);
}

#ifdef CONFIG_ETH_PM_CB_SUPPORT
static void eth_set_ps_prevent(struct eth_mac_priv *priv, uint32_t prevent_bits)
{
  GLOBAL_INT_DECLARATION();

  GLOBAL_INT_DISABLE();
  priv->ps_prevent |= prevent_bits;
  GLOBAL_INT_RESTORE();
}

static void eth_clear_ps_prevent(struct eth_mac_priv *priv, uint32_t prevent_bits)
{
  GLOBAL_INT_DECLARATION();

  GLOBAL_INT_DISABLE();
  priv->ps_prevent &= ~prevent_bits;
  GLOBAL_INT_RESTORE();
}

static void eth_sleep_check()
{
  // check tx, rx, irq pending, EthEvents
  struct eth_mac_priv *priv = heth.priv;

  // ps prevent
  if (priv->ps_prevent) {
    return;
  }

  // tx packet not completed
  if (priv->tx_count) {
    return;
  }

  // Still has events hot processed
  if (!rtos_is_queue_empty(&priv->eventq)) {
    return;
  }

  // Eth can put into doze mode
  bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_ENET, 0x1, 0x0);
}

static void eth_wakeup_check(BUS_MSG_T *msg)
{
  struct eth_mac_priv *priv = heth.priv;

  // Eth PS keep timer end
  if (msg->type == BMSG_PS_KEEP_END) {
    return;
  }

  if (priv->hw_in_doze) {

    // If eth in doze mode, Exit from Doze
    __HAL_ETH_Exit_LP();

    priv->hw_in_doze = false;
  } else {
    // Current all msg will wakeup ethernet
    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_ENET, 0x0, 0x0);
  }

  // restart ethernet linger timer
  eth_start_keep_timer(1, 0);
  eth_set_ps_prevent(priv, ETH_PS_PREVENT_KEEP_TIMER);
}
#endif // CONFIG_ETH_PM_CB_SUPPORT

// core_thread_init / core_thread_main
static void ethernetif_core_thread(void *argument)
{
  struct netif *netif = (struct netif *)argument;
  bk_err_t ret;
  BUS_MSG_T msg;
  struct eth_mac_priv *priv = heth.priv;
  GLOBAL_INT_DECLARATION();

  for (;;) {
    ret = rtos_pop_from_queue(&priv->eventq, &msg, BEKEN_WAIT_FOREVER);

    // check ethernet wakeup
    if (ret != kNoErr) {
      BK_LOGD(NULL,"pop queue failed\n");
      continue;
    }

#ifdef CONFIG_ETH_PM_CB_SUPPORT
    eth_wakeup_check(&msg);
#endif

    //BK_LOGD(NULL,"enter: msg.type %d\n", msg.type);
    switch (msg.type) {
    case BMSG_RX_TYPE:
      bmsg_eth_rx_handler(&msg, netif);
      break;

    case BMSG_TX_TYPE:
      bmsg_eth_tx_handler(&msg);
      break;

    case BMSG_TX_COMPLETE:
      GLOBAL_INT_DISABLE();
      if (bmsg_eth_tx_compl_count > 0)
        bmsg_eth_tx_compl_count -= 1;
      GLOBAL_INT_RESTORE();

      HAL_ETH_ReleaseTxPacket(&heth);
      break;

#ifdef CONFIG_ETH_PM_CB_SUPPORT
    case BMSG_PS_KEEP_END:
      eth_clear_ps_prevent(priv, ETH_PS_PREVENT_KEEP_TIMER);
      break;
#endif
    }

#ifdef CONFIG_ETH_PM_CB_SUPPORT
    // Check whether ETH may sleep
    eth_sleep_check();
#endif
  }
}

#if !LWIP_ARP
/**
 * This function has to be completed by user in case of ARP OFF.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if ...
 */
static err_t low_level_output_arp_off(struct netif *netif, struct pbuf *q, const ip4_addr_t *ipaddr)
{
  err_t errval;
  errval = ERR_OK;

  return errval;

}
#endif /* LWIP_ARP */

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  // MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */

#if LWIP_IPV4
#if LWIP_ARP || LWIP_ETHERNET
#if LWIP_ARP
  netif->output = etharp_output;
#else
  /* The user should write its own code in low_level_output_arp_off function */
  netif->output = low_level_output_arp_off;
#endif /* LWIP_ARP */
#endif /* LWIP_ARP || LWIP_ETHERNET */
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */

  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  return low_level_init(netif);
}

/**
  * @brief  Custom Rx pbuf free callback
  * @param  pbuf: pbuf to be freed
  * @retval None
  */
static void pbuf_free_custom(struct pbuf *p)
{
  struct pbuf_custom* custom_pbuf = (struct pbuf_custom*)p;
  LWIP_MEMPOOL_FREE(RX_POOL, custom_pbuf);

  /* If the Rx Buffer Pool was exhausted, signal the ethernetif_input task to
   * call HAL_ETH_GetRxDataBuffer to rebuild the Rx descriptors. */

  if (RxAllocStatus == RX_ALLOC_ERROR)
  {
    RxAllocStatus = RX_ALLOC_OK;
    bmsg_eth_rx_sender(0);
  }
}

#ifdef CONFIG_ETH_PM_CB_SUPPORT
// refer: HAL_ETH_Init
static int HAL_ETH_Enter_LP(uint64_t sleep_time, void *args)
{
  struct eth_mac_priv *priv = heth.priv;

  // already in doze
  if (priv->hw_in_doze)
    return 0;

  // Disable ETH IRQ
  sys_hal_enable_eth_int(0);

  // Stop ETH
  HAL_ETH_Stop_IT(&heth);

  sys_ll_set_reserver_reg0xd_enet_cken(0);

  SET_BIT(ETH_RESET_CTRL, 0);

  // disable AHB Power Domain
  bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_ENET, PM_POWER_MODULE_STATE_OFF);

  LWIP_LOGV("ETH Enter LP done\n");

  // Eth enters low voltage
  priv->hw_in_doze = true;

  return 0;
}

static int HAL_ETH_Exit_LP(uint64_t sleep_time, void *args)
{
  LWIP_LOGV("%s\n", __func__);

  struct eth_mac_priv *priv = heth.priv;

  priv->skip_link_check_count = 10;

  // defer to task
  bmsg_eth_null_sender();

  return 0;
}


// Exits from PS
static int __HAL_ETH_Exit_LP()
{
  ETH_MACConfigTypeDef MACConf = {0};
  struct eth_mac_priv *priv = heth.priv;

  // Current all msg will wakeup ethernet
  bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_ENET, 0x0, 0x0);

  // ETH soft reset
  SET_BIT(ETH_RESET_CTRL, 0x1);

  // ETH bypass clockgate
  WRITE_REG(ETH_RESET_CTRL, 0x3);

  // ETH CLK enable
  sys_ll_set_reserver_reg0xd_enet_cken(1);

  // Enable ETH IRQ
  sys_hal_enable_eth_int(1);

  // eth_restore_registers(&heth);
  HAL_ETH_ReInit(&heth);

  /* Get MAC Config MAC */
  HAL_ETH_GetMACConfig(&heth, &MACConf);
  MACConf.DuplexMode = priv->duplex;
  MACConf.Speed = priv->speed;
  HAL_ETH_SetMACConfig(&heth, &MACConf);
  HAL_ETH_Start_IT(&heth);
  priv->hw_in_doze = false;

  LWIP_LOGD("ETH exits LP\n");

  return 0;
}
#endif // CONFIG_ETH_PM_CB_SUPPORT

void HAL_ETH_MspInit(ETH_HandleTypeDef* ethHandle)
{
  //LWIP_LOGD("HW DeviceID: 0x%x\n", REG_READ((ETH_BASE + 0x800*4)));
  //LWIP_LOGD("HW VersionID: 0x%x\n", REG_READ((ETH_BASE + 0x801*4)));

  // GPIO PinMUX: group0(27, 29-39), or group2(46-55)
  // FIXME: BK7236, use dts instead of hard coding.
#ifdef CONFIG_ETH_PIN_GROUP0
  gpio_dev_unmap(GPIO_27);  // PHY INT
  gpio_dev_unmap(GPIO_29);  // MDC
  gpio_dev_unmap(GPIO_32);  // MDIO
  gpio_dev_unmap(GPIO_33);  // RXD[0]
  gpio_dev_unmap(GPIO_34);  // RXD[1]
  gpio_dev_unmap(GPIO_35);  // RXDV
  gpio_dev_unmap(GPIO_36);  // TXD[0]
  gpio_dev_unmap(GPIO_37);  // TXD[1]
  gpio_dev_unmap(GPIO_38);  // TXEN
  gpio_dev_unmap(GPIO_39);  // REF_CLK

  gpio_dev_map(GPIO_27, GPIO_DEV_ENET_PHY_INT);
  gpio_dev_map(GPIO_29, GPIO_DEV_ENET_MDC);
  gpio_dev_map(GPIO_32, GPIO_DEV_ENET_MDIO);
  gpio_dev_map(GPIO_33, GPIO_DEV_ENET_RXD0);
  gpio_dev_map(GPIO_34, GPIO_DEV_ENET_RXD1);
  gpio_dev_map(GPIO_35, GPIO_DEV_ENET_RXDV);
  gpio_dev_map(GPIO_36, GPIO_DEV_ENET_TXD0);
  gpio_dev_map(GPIO_37, GPIO_DEV_ENET_TXD1);
  gpio_dev_map(GPIO_38, GPIO_DEV_ENET_TXEN);
  gpio_dev_map(GPIO_39, GPIO_DEV_ENET_REF_CLK);
#elif defined(CONFIG_ETH_PIN_GROUP1)
  // group2(46-55)
  gpio_dev_unmap(GPIO_46);  // PHY INT
  gpio_dev_unmap(GPIO_47);  // MDC
  gpio_dev_unmap(GPIO_48);  // MDIO
  gpio_dev_unmap(GPIO_49);  // RXD[0]
  gpio_dev_unmap(GPIO_50);  // RXD[1]
  gpio_dev_unmap(GPIO_51);  // RXDV
  gpio_dev_unmap(GPIO_52);  // TXD[0]
  gpio_dev_unmap(GPIO_53);  // TXD[1]
  gpio_dev_unmap(GPIO_54);  // TXEN
  gpio_dev_unmap(GPIO_55);  // REF_CLK

  gpio_dev_map(GPIO_46, GPIO_DEV_ENET_PHY_INT);
  gpio_dev_map(GPIO_47, GPIO_DEV_ENET_MDC);
  gpio_dev_map(GPIO_48, GPIO_DEV_ENET_MDIO);
  gpio_dev_map(GPIO_49, GPIO_DEV_ENET_RXD0);
  gpio_dev_map(GPIO_50, GPIO_DEV_ENET_RXD1);
  gpio_dev_map(GPIO_51, GPIO_DEV_ENET_RXDV);
  gpio_dev_map(GPIO_52, GPIO_DEV_ENET_TXD0);
  gpio_dev_map(GPIO_53, GPIO_DEV_ENET_TXD1);
  gpio_dev_map(GPIO_54, GPIO_DEV_ENET_TXEN);
  gpio_dev_map(GPIO_55, GPIO_DEV_ENET_REF_CLK);
#endif

#ifdef CONFIG_ETH_PM_CB_SUPPORT
  // Power On AHBP
  bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_ENET, PM_POWER_MODULE_STATE_ON);

  // Don't allow ETH enters PS
  bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_ENET, 0x0, 0x0);
#endif

  LWIP_LOGD("ETH SoftRest\n");
  // ETH soft reset
  SET_BIT(ETH_RESET_CTRL, 0x1);

  // ETH bypass clockgate
  WRITE_REG(ETH_RESET_CTRL, 0x3);

  // ETH CLK enable
  sys_ll_set_reserver_reg0xd_enet_cken(1);

  // Register ETH interrupt ISR
  bk_int_isr_register(INT_SRC_ETH, ETH_IRQHandler, NULL);

  // Enable ETH IRQ
  sys_hal_enable_eth_int(1);

#ifdef CONFIG_ETH_PM_CB_SUPPORT
  // Register enters/exit low voltage callback
  pm_cb_conf_t enter_config;
  enter_config.cb = (pm_cb)HAL_ETH_Enter_LP;
  enter_config.args = NULL;

  pm_cb_conf_t exit_config;
  exit_config.cb = (pm_cb)HAL_ETH_Exit_LP;
  exit_config.args = NULL;

  bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_ENET, &enter_config, &exit_config);
#endif
}

void HAL_ETH_MspDeInit(ETH_HandleTypeDef* ethHandle)
{
  if(ethHandle->Instance==ETH)
  {
#ifdef ETH_NOT_IMPLEMENTED
    /* Disable Peripheral clock */
    __HAL_RCC_ETH1MAC_CLK_DISABLE();
    __HAL_RCC_ETH1TX_CLK_DISABLE();
    __HAL_RCC_ETH1RX_CLK_DISABLE();

    /**ETH GPIO Configuration
    PC1     ------> ETH_MDC
    PA1     ------> ETH_REF_CLK
    PA2     ------> ETH_MDIO
    PA7     ------> ETH_CRS_DV
    PC4     ------> ETH_RXD0
    PC5     ------> ETH_RXD1
    PB13     ------> ETH_TXD1
    PG11     ------> ETH_TX_EN
    PG13     ------> ETH_TXD0
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5);

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_7);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13);

    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_11|GPIO_PIN_13);

    /* Peripheral interrupt Deinit*/
    HAL_NVIC_DisableIRQ(ETH_IRQn);
#endif
  }

#ifdef CONFIG_ETH_PM_CB_SUPPORT
  // allow eth sleep, and allow AHBP power off
  bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_ENET, 0x1, 0x0);
  bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_ENET, PM_POWER_MODULE_STATE_OFF);
#endif
}

#if !ETH_MULTI_PHY_SUPPORT
/*******************************************************************************
                       PHI IO Functions
*******************************************************************************/
/**
  * @brief  Initializes the MDIO interface GPIO and clocks.
  * @param  None
  * @retval 0 if OK, -1 if ERROR
  */
static int32_t ETH_PHY_IO_Init(void)
{
  /* We assume that MDIO GPIO configuration is already done
     in the ETH_MspInit() else it should be done here
  */

  /* Configure the MDIO Clock */
  HAL_ETH_SetMDIOClockRange(&heth);

  return 0;
}

/**
  * @brief  De-Initializes the MDIO interface .
  * @param  None
  * @retval 0 if OK, -1 if ERROR
  */
static int32_t ETH_PHY_IO_DeInit (void)
{
  return 0;
}

/**
  * @brief  Read a PHY register through the MDIO interface.
  * @param  DevAddr: PHY port address
  * @param  RegAddr: PHY register address
  * @param  pRegVal: pointer to hold the register value
  * @retval 0 if OK -1 if Error
  */
static int32_t ETH_PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal)
{
  if(HAL_ETH_ReadPHYRegister(&heth, DevAddr, RegAddr, pRegVal) != HAL_OK)
  {
    return -1;
  }

  return 0;
}

/**
  * @brief  Write a value to a PHY register through the MDIO interface.
  * @param  DevAddr: PHY port address
  * @param  RegAddr: PHY register address
  * @param  RegVal: Value to be written
  * @retval 0 if OK -1 if Error
  */
static int32_t ETH_PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal)
{
  if(HAL_ETH_WritePHYRegister(&heth, DevAddr, RegAddr, RegVal) != HAL_OK)
  {
    return -1;
  }

  return 0;
}

/**
  * @brief  Get the time in millisecons used for internal PHY driver process.
  * @retval Time value
  */
static int32_t ETH_PHY_IO_GetTick(void)
{
  return xTaskGetTickCount();
}
#endif

#if ETH_MULTI_PHY_SUPPORT
/**
  * @brief  Check the ETH link state then update ETH driver and netif link accordingly.
  * @param  argument: netif
  * @retval None
  */
void ethernet_link_thread(void *argument)
{
  ETH_MACConfigTypeDef MACConf = {0};
  //int32_t PHYLinkState = 0;
  uint32_t linkchanged = 0U, speed = 0U, duplex = 0U;

  struct netif *netif = (struct netif *) argument;
  struct eth_mac_priv *priv __maybe_unused = heth.priv;
  struct phy_device *phydev = priv->phy;

  /* ETH_CODE: workaround to call LOCK_TCPIP_CORE when accessing netif link functions*/
  // LOCK_TCPIP_CORE();

  /* ETH link init */

  for(;;)
  {
#ifdef CONFIG_ETH_PM_CB_SUPPORT
    // Don't allow eth enters PS until link up
    eth_set_ps_prevent(priv, ETH_PS_PREVENT_LINK_CHECK);
#endif

    genphy_read_status(phydev);

#ifdef CONFIG_ETH_PM_CB_SUPPORT
    // If link down and skip count remains then try later. Still don't allow eth enter PS
    if (!phydev->link && priv->skip_link_check_count) {
      priv->skip_link_check_count--;
      rtos_delay_milliseconds(200);
      continue;
    }
#endif

    if (netif_is_link_up(netif) && !phydev->link) {
    LWIP_LOGD("ETH link down\n");
    HAL_ETH_Stop_IT(&heth);

    // stop dynamic address or static ip address
    eth_ip_down();  // XXX: netifapi_XXX
    } else if (!netif_is_link_up(netif) && phydev->link) {
      linkchanged = 1;

      switch (phydev->duplex) {
      case DUPLEX_FULL:
          duplex = ETH_FULLDUPLEX_MODE;
          break;
        case DUPLEX_HALF:
      duplex = ETH_HALFDUPLEX_MODE;
      break;
    default:
        linkchanged = 0;
      }

      switch (phydev->speed) {
      case SPEED_1000:
        break;
      case SPEED_100:
        speed = ETH_SPEED_100M;
        break;
      case SPEED_10:
        speed = ETH_SPEED_10M;
        break;
    default:
        linkchanged = 0;
      }

      if(linkchanged)
      {
        LWIP_LOGD("ETH link up, speed %dM, %s-duplex\n", phydev->speed, phydev->duplex ? "Full" : "Half");

        /* Get MAC Config MAC */
        HAL_ETH_GetMACConfig(&heth, &MACConf);
        MACConf.DuplexMode = duplex;
        MACConf.Speed = speed;
#ifdef CONFIG_ETH_PM_CB_SUPPORT
        priv->duplex = duplex;
        priv->speed = speed;
#endif
        HAL_ETH_SetMACConfig(&heth, &MACConf);
        HAL_ETH_Start_IT(&heth);
        netifapi_netif_set_link_up(netif);

        eth_ip_start();   // netifapi_XXX
      }
    }

#ifdef CONFIG_ETH_PM_CB_SUPPORT
    // allow eth enters PS
    eth_clear_ps_prevent(priv, ETH_PS_PREVENT_LINK_CHECK);
#endif

    /* ETH_CODE: workaround to call LOCK_TCPIP_CORE when accessing netif link functions*/
    // UNLOCK_TCPIP_CORE();
    rtos_delay_milliseconds(200);
    // LOCK_TCPIP_CORE();
  }
}
#else
/**
  * @brief  Check the ETH link state then update ETH driver and netif link accordingly.
  * @param  argument: netif
  * @retval None
  */
void ethernet_link_thread(void *argument)
{
  ETH_MACConfigTypeDef MACConf = {0};
  int32_t PHYLinkState = 0;
  uint32_t linkchanged = 0U, speed = 0U, duplex = 0U;

  struct netif *netif = (struct netif *) argument;
  struct eth_mac_priv *priv __maybe_unused = heth.priv;

  /* ETH_CODE: workaround to call LOCK_TCPIP_CORE when accessing netif link functions*/
  // LOCK_TCPIP_CORE();

  /* ETH link init */

  for(;;)
  {
#ifdef CONFIG_ETH_PM_CB_SUPPORT
    // Don't allow eth enters PS until link up
    eth_set_ps_prevent(priv, ETH_PS_PREVENT_LINK_CHECK);
#endif

    PHYLinkState = LAN8742_GetLinkState(&LAN8742);

#ifdef CONFIG_ETH_PM_CB_SUPPORT
    // If link down and skip count remains then try later. Still don't allow eth enter PS
    if ((PHYLinkState <= LAN8742_STATUS_LINK_DOWN) && priv->skip_link_check_count) {
      priv->skip_link_check_count--;
      rtos_delay_milliseconds(200);
      continue;
    }
#endif

    if(netif_is_link_up(netif) && (PHYLinkState <= LAN8742_STATUS_LINK_DOWN))
    {
      LWIP_LOGD("ETH link down\n");
      HAL_ETH_Stop_IT(&heth);

      // stop dynamic address or static ip address
      eth_ip_down();  // XXX: netifapi_XXX
    }
    else if(!netif_is_link_up(netif) && (PHYLinkState > LAN8742_STATUS_LINK_DOWN))
    {
      switch (PHYLinkState)
      {
      case LAN8742_STATUS_100MBITS_FULLDUPLEX:
        duplex = ETH_FULLDUPLEX_MODE;
        speed = ETH_SPEED_100M;
        linkchanged = 1;
        break;
      case LAN8742_STATUS_100MBITS_HALFDUPLEX:
        duplex = ETH_HALFDUPLEX_MODE;
        speed = ETH_SPEED_100M;
        linkchanged = 1;
        break;
      case LAN8742_STATUS_10MBITS_FULLDUPLEX:
        duplex = ETH_FULLDUPLEX_MODE;
        speed = ETH_SPEED_10M;
        linkchanged = 1;
        break;
      case LAN8742_STATUS_10MBITS_HALFDUPLEX:
        duplex = ETH_HALFDUPLEX_MODE;
        speed = ETH_SPEED_10M;
        linkchanged = 1;
        break;
      default:
        break;
      }

      if(linkchanged)
      {
        LWIP_LOGD("ETH link up, speed %d, duplex %d\n", speed, duplex);

        /* Get MAC Config MAC */
        HAL_ETH_GetMACConfig(&heth, &MACConf);
#ifdef CONFIG_ETH_PM_CB_SUPPORT
        priv->duplex = MACConf.DuplexMode = duplex;
        priv->speed = MACConf.Speed = speed;
#endif
        HAL_ETH_SetMACConfig(&heth, &MACConf);
        HAL_ETH_Start_IT(&heth);
        netifapi_netif_set_link_up(netif);

        eth_ip_start();   // netifapi_XXX
      }
    }

#ifdef CONFIG_ETH_PM_CB_SUPPORT
    // allow eth enters PS
    eth_clear_ps_prevent(priv, ETH_PS_PREVENT_LINK_CHECK);
#endif

    /* ETH_CODE: workaround to call LOCK_TCPIP_CORE when accessing netif link functions*/
    // UNLOCK_TCPIP_CORE();
    rtos_delay_milliseconds(200);
    // LOCK_TCPIP_CORE();
  }
}
#endif //ETH_MULTI_PHY_SUPPORT

void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
  struct pbuf_custom *p = LWIP_MEMPOOL_ALLOC(RX_POOL);
  if (p)
  {
    /* Get the buff from the struct pbuf address. */
    *buff = (uint8_t *)p + offsetof(RxBuff_t, buff);
    p->custom_free_function = pbuf_free_custom;
    /* Initialize the struct pbuf.
    * This must be performed whenever a buffer's allocated because it may be
    * changed by lwIP or the app, e.g., pbuf_free decrements ref. */
    pbuf_alloced_custom(PBUF_RAW, 0, PBUF_REF, p, *buff, ETH_RX_BUFFER_SIZE);
  }
  else
  {
    RxAllocStatus = RX_ALLOC_ERROR;
    *buff = NULL;
  }
}

void HAL_ETH_Free_Rxbuf(uint8_t *buff)
{
  struct pbuf *p = (struct pbuf *)(buff - offsetof(RxBuff_t, buff));
  pbuf_free(p);
}

void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd, uint8_t *buff, uint16_t Length)
{
  struct pbuf **ppStart = (struct pbuf **)pStart;
  struct pbuf **ppEnd = (struct pbuf **)pEnd;
  struct pbuf *p = NULL;

  /* Get the struct pbuf from the buff address. */
  p = (struct pbuf *)(buff - offsetof(RxBuff_t, buff));
  p->next = NULL;
  p->tot_len = 0;
  p->len = Length;

  /* Chain the buffer. */
  if (!*ppStart)
  {
    /* The first buffer of the packet. */
    *ppStart = p;
  }
  else
  {
    /* Chain the buffer to the end of the packet. */
    (*ppEnd)->next = p;
  }
  *ppEnd  = p;

  /* Update the total length of all the buffers of the chain. Each pbuf in the chain should have its tot_len
   * set to its own length, plus the length of all the following pbufs in the chain. */
  for (p = *ppStart; p != NULL; p = p->next)
  {
    p->tot_len += Length;
  }
}

void HAL_ETH_TxFreeCallback(uint32_t * buff)
{
  struct eth_mac_priv *priv __maybe_unused = heth.priv;

  pbuf_free((struct pbuf *)buff);

#ifdef CONFIG_ETH_PM_CB_SUPPORT
  // decrease tx counter of TX path
  BK_ASSERT(priv->tx_count);
  priv->tx_count--;
#endif
}

void ETH_DumpAllRegisters(void)
{
  HAL_ETH_DumpAllRegisters(&heth);
}

char *netif_name(struct netif *netif)
{
  return netif->name;
}

#endif /* CONFIG_ETH */

