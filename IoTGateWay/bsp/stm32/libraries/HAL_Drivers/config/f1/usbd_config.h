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

#define USBD_IRQ_TYPE        OTG_FS_IRQn
#define USBD_IRQ_HANDLER     OTG_FS_IRQHandler
#define USBD_INSTANCE        USB_OTG_FS 
#define USBD_PCD_SPEED       PCD_SPEED_FULL
#define USBD_PCD_PHY_MODULE  PCD_PHY_EMBEDDED

#endif
