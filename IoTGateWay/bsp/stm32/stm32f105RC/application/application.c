/*
 * File      : application.c.
*/

#include <board.h>
#include <drivers/rtc.h>

#include "application.h"
#include <config/appconfig.h>
#include <device/uart/plccomm.h>
#include <device/uart/usbconsole.h>
#include <network/networkmanager.h>
#include <device/led/led.h>

#define REBOOT_TIMEOUT	3000

typedef struct _APPINFO
{
	rt_mq_t appMq;

	rt_timer_t	rebootTimer;

	NetworkInfoData networkInfoData;
}AppInfo;

static AppInfo appInfo;

void DeInitApplication(void)
{
	rt_kprintf("DeInitialize Application.\r\n");

	if(RT_EOK != rt_mq_delete(appInfo.appMq))
	{
		rt_kprintf("Deleting Application Message Queue Failed.\r\n");
	}
}

void ApplicationSendMessage(MqData_t *pMqData)
{
	if(-RT_EFULL == rt_mq_send(appInfo.appMq, (void *)&pMqData, sizeof(void*)))
	{
		rt_kprintf("Application Queue full - throw away\r\n");
	}
}

void RebootTimer(void *params)
{
	DeInitUsbconsole();
	NVIC_SystemReset();
}

void DeviceReboot(void)
{
	DeInitNetworkManager();
	DeInitApplication();
	rt_timer_stop(appInfo.rebootTimer);
	rt_kprintf("Restart After 3 seconds\r\n");
	rt_timer_start(appInfo.rebootTimer);
}

rt_bool_t StartApplication(void)
{
	return StartNetworkManager();
}

static void app_main_thread(void *params)
{
	AppInfo *p_handle = (AppInfo *)params;
	MqData_t *pMqData = RT_NULL;
	uint8_t channel2Led = DISABLE;

	LedBlinkTurnOn(WIFI_LED);

	while(1)
	{
		if(rt_mq_recv(p_handle->appMq, &pMqData, sizeof(void *), RT_WAITING_FOREVER) == RT_EOK)
		{
			MqData_t mqData;

			rt_memcpy(&mqData, pMqData, sizeof(mqData));
			rt_memcpy(&p_handle->networkInfoData, mqData.data, sizeof(p_handle->networkInfoData));

			switch(mqData.messge)
			{
			case SMSG_WIFI_OK:
				switch(p_handle->networkInfoData.networkStatus)
				{
				case STATUS_CONNECT_WIFI:
					LedTurnOn(WIFI_LED);
					LedBlinkTurnOn(CHANNEL1_LED);//TCP
					break;
				case STATUS_TCP_CONNECT:
					LedTurnOn(CHANNEL1_LED);
					LedBlinkTurnOn(CHANNEL2_LED);//Sending Data
					channel2Led = ENABLE;
					break;
				default:
					break;
				}
				break;
			case SMSG_SEND_FAIL:
					channel2Led = DISABLE;
					LedTurnOff(CHANNEL2_LED);
					break;
			case SMSG_SEND_OK:
				if(DISABLE == channel2Led)
				{
					LedBlinkTurnOn(CHANNEL2_LED);//Sending Data
					channel2Led = ENABLE;
				}
				break;
			}
		}
	}
}

void InitApplicationInfo(void)
{
	appInfo.appMq = RT_NULL;
	appInfo.rebootTimer = RT_NULL;
}

rt_bool_t InitApplication(void)
{
    AppInfo *h_data = &appInfo;
	rt_bool_t retVal = RT_FALSE;
	rt_thread_t tid;

	InitApplicationInfo();
	InitLed();

	h_data->appMq = rt_mq_create("app_mq", sizeof(MqData_t), APPLICATION_MQ_SIZE, RT_IPC_FLAG_FIFO);
	RT_ASSERT(RT_NULL != h_data->appMq);

    tid = rt_thread_create("application", app_main_thread, (void *)h_data, APPLICATION_STACK_SIZE, RT_MAIN_THREAD_PRIORITY, 20);
    RT_ASSERT(RT_NULL != tid);

    h_data->rebootTimer = rt_timer_create("rebootTimer", RebootTimer, RT_NULL, rt_tick_from_millisecond(REBOOT_TIMEOUT), RT_TIMER_FLAG_ONE_SHOT);
	RT_ASSERT(RT_NULL != h_data->rebootTimer);

	/* thread start  */
	rt_thread_startup(tid);

	if(RT_FALSE == (retVal=InitPlcComm()) || RT_FALSE == (retVal=InitNetworkManager()))
	{
		rt_kprintf("Init Applicaiotn failed.\r\n");
	}

	return retVal;
}
