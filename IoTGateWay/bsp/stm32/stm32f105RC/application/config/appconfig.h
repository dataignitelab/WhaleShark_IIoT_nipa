/*
 * File      : appconfig.h
 */

/**
 * @addtogroup STM32
 */
/*@{*/
#ifndef __APPCONFIG_H__
#define __APPCONFIG_H__

#include <board.h>

/* software version */
#define SVER_MAJOR							1
#define SVER_MINOR							0
#define SVER_BUILD							1

#define MODEL_NAME							"SA-100"
#define PRODUCT_NAME						"Spiretech"

/* Uart name */
#define UART2_DEV_NAME						"uart2"
#define UART3_DEV_NAME						"uart3"
#define UART5_DEV_NAME						"uart5"

#define UART2_PORT                   		UART2_DEV_NAME
#define UART3_PORT                   		UART3_DEV_NAME
#define UART5_PORT                   		UART5_DEV_NAME

/*rt-thread stack size*/
#define APPLICATION_STACK_SIZE				1024U
#define USBCONSOLE_STACK_SIZE				1024U
#define PLCCOMMM_STACK_SIZE				    1024U
#define WIFI_RX_STACK_SZIE					1024U
#define	NETWORKMANAGER_STACK_SIZE			1024U
#define MANUFACTURE_CONTROL_STACK_SIZE		1024U

/**/
#define CR							        '\r' // 0x0D
#define LF							        '\n' // 0x0A
#define SP									0x20 // space
#define STX									0x02
#define ETX									0x03

/* TLV ID */

#define DEFAULT_SSID						"STI_IOT"
#define DEFAULT_PASSWORD					"!sti_iot!"
#define DEFAULT_IP							"0.0.0.0"
#define DEFAULT_MAC							"00:00:00:00:00:00"
#define DEFAULT_DEVICE						"TS0000" //Default Device Info
#define DEFAULT_DOMAIN						"onsite-monitor.xip.kr"

#endif /* __APPCONFIG_H__ */

/*@}*/
