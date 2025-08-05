/* USER CODE BEGIN Header */
/**
 ******************************************************************************
  * File Name          : ethernetif.h
  * Description        : This file provides initialization code for LWIP
  *                      middleWare.
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
/* USER CODE END Header */

#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__

#include "common/bk_include.h"

#ifdef CONFIG_ETH
#include "lwip/err.h"
#include "lwip/netif.h"

/* Exported functions ------------------------------------------------------- */
err_t ethernetif_init(struct netif *netif);
void ethernet_link_thread(void *argument);

#endif /* CONFIG_ETH */

#endif /* __ETHERNETIF_H__ */

