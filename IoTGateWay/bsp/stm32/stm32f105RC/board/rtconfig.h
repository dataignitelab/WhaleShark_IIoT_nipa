#ifndef RT_CONFIG_H__
#define RT_CONFIG_H__

/* Automatically generated file; DO NOT EDIT. */
/* RT-Thread Configuration */

/* RT-Thread Kernel */

#define RT_NAME_MAX 8
#define RT_ALIGN_SIZE 4
#define RT_THREAD_PRIORITY_32
#define RT_THREAD_PRIORITY_MAX 32
#define RT_TICK_PER_SECOND 1000
#define RT_USING_OVERFLOW_CHECK
#define RT_USING_HOOK
#define RT_USING_IDLE_HOOK
#define RT_IDLE_HOOK_LIST_SIZE 4
#define IDLE_THREAD_STACK_SIZE 256

/* Inter-Thread communication */

#define RT_USING_SEMAPHORE
#define RT_USING_MUTEX
#define RT_USING_EVENT
#define RT_USING_MESSAGEQUEUE

/* Memory Management */

#define RT_USING_MEMPOOL
#define RT_USING_SMALL_MEM
#define RT_USING_HEAP

/* Kernel Device Object */

#define RT_USING_DEVICE
#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE 128
#define RT_CONSOLE_DEVICE_NAME "vcom"
#define RT_VER_NUM 0x40000
#define ARCH_ARM
#define ARCH_ARM_CORTEX_M
#define ARCH_ARM_CORTEX_M3

/* RT-Thread Components */

/* #define RT_USING_COMPONENTS_INIT */
#define RT_USING_USER_MAIN
#define RT_MAIN_THREAD_STACK_SIZE 4096
#define RT_MAIN_THREAD_PRIORITY 10

/* C++ features */

/* Device Drivers */

#define RT_USING_SERIAL
#define RT_SERIAL_RB_BUFSZ 1024 //Uart Buffer 512 -> 1024
#define RT_USING_PIN
//#define RT_USING_RTC

/* Using USB */

#define RT_USING_USB_DEVICE
#define RT_USBD_THREAD_STACK_SZ 4096
#define USB_VENDOR_ID 0x0FFE
#define USB_PRODUCT_ID 0x0001
#define _RT_USB_DEVICE_CDC
#define RT_USB_DEVICE_CDC
#define RT_VCOM_TASK_STK_SIZE 1024
#define RT_VCOM_SERNO "32021919830108"
#define RT_VCOM_SER_LEN 14
#define RT_VCOM_TX_TIMEOUT 1000

#define SOC_FAMILY_STM32
#define SOC_SERIES_STM32F1

/* Hardware Drivers Config */

#define SOC_STM32F105RC

/* Onboard Peripheral Drivers */

/* On-chip Peripheral Drivers */

#define BSP_USING_GPIO
#define BSP_USING_UART
#define BSP_USING_UART2
#define BSP_USING_UART3
#define BSP_USING_UART5
#define BSP_USING_USBD
#define BSP_USING_ON_CHIP_FLASH


/* Board extended module Drivers */


#endif
