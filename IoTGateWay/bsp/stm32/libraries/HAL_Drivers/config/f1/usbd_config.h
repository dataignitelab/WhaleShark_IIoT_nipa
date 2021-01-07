/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-10     ZYH          first version
 * 2019-07-29     Chinese66    change from f4 to f1
 */
#ifndef __USBD_CONFIG_H__
#define __USBD_CONFIG_H__

#if 0 /* Kyle 20200121 */
#define USBD_IRQ_TYPE        USB_LP_CAN1_RX0_IRQn
#define USBD_IRQ_HANDLER     USB_LP_CAN1_RX0_IRQHandler
#define USBD_INSTANCE        USB
#else
#define USBD_IRQ_TYPE        OTG_FS_IRQn
#define USBD_IRQ_HANDLER     OTG_FS_IRQHandler
#define USBD_INSTANCE        USB_OTG_FS 
#endif
#define USBD_PCD_SPEED       PCD_SPEED_FULL
#define USBD_PCD_PHY_MODULE  PCD_PHY_EMBEDDED

#ifndef BSP_USB_CONNECT_PIN
#define BSP_USB_CONNECT_PIN  -1
#endif

#ifndef BSP_USB_PULL_UP_STATUS
#define BSP_USB_PULL_UP_STATUS  1
#endif
#endif
