/*
 * File      : application.c.
*/

#include <board.h>
#include <drivers/rtc.h>

#include "application.h"
#include <config/appconfig.h>
#include <device/uart/plccomm.h>
#include <network/networkmanager.h>
#include <device/led/led.h>

typedef struct _APPINFO
{
	rt_mq_t appMq;

	NetworkInfoData networkInfoData;
}AppInfo;

static AppInfo appInfo;

void ApplicationSendMessage(MqData_t *pMqData)
{
	if(-RT_EFULL == rt_mq_send(appInfo.appMq, (void *)&pMqData, sizeof(void*)))
	{
		rt_kprintf("Application Queue full - throw away\r\n");
	}
}

void DeviceReboot(void)
{
	rt_kprintf("Restart IoT Gateway and Wi-Fi Module After 1 second\r\n");
	rt_thread_delay(1000);
	NVIC_SystemReset();
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
				case STATUS_SEND_DATA:
					if(DISABLE == channel2Led)
					{
						LedBlinkTurnOn(CHANNEL2_LED);//Sending Data
						channel2Led = ENABLE;
					}
					break;
				default:
					break;
				}
				break;
			case SMSG_WIFI_FAIL:
			case SMSG_WIFI_ERROR:
				switch(p_handle->networkInfoData.networkStatus)
				{
				case STATUS_SEND_DATA:
					channel2Led = DISABLE;
					LedTurnOff(CHANNEL2_LED);
					break;
				default:
					break;
				}
				break;
			}
		}
	}
}

void InitApplicationInfo(void)
{
	appInfo.appMq = RT_NULL;
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

	/* thread start  */
	rt_err_t err = rt_thread_startup(tid);
	RT_ASSERT(RT_EOK == err);

	if(RT_FALSE == (retVal=InitPlcComm()) || RT_FALSE == (retVal=InitNetworkManager()))
	{
		rt_kprintf("Init Applicaiotn failed.\r\n");
	}

	return retVal;
}
