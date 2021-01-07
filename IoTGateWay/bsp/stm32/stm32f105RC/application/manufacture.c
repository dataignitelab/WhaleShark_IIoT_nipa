/*
 * manufacture.c
 *
 */

#include <rtthread.h>

#include "manufacture.h"
#include <config/appconfig.h>
#include <common/common.h>
#include <config/configuration.h>

#define RESPONSE_STANDBY_DELAY	5000

#define MANUFACTURE_RX_BUF		32U

#define MSG_EXPIRED				0x00000001
#define MSG_CHECK_DEVICE		0x00000002
#define CHECK_COUNT				1

typedef struct _ManufactureInfo {
	rt_mq_t manuMq;
	MqData_t mqData;

	rt_thread_t tid;
	rt_timer_t	manufactureTimer;

	rt_uint8_t checkCount;

	rt_uint8_t rxBuf[MANUFACTURE_RX_BUF];
} ManufactureInfo;

static ManufactureInfo manufactureInfo;
void InitManufactureData(void);

void ManufactureSendMessage(MqData_t *pMqData)
{
	if(-RT_EFULL == rt_mq_send(manufactureInfo.manuMq, (void *)&pMqData, sizeof(void*)))
	{
		rt_kprintf("Queue full - throw away\r\n");
	}
}

void WaitResponseData(void *params)
{
	ManufactureInfo *p_handle = (ManufactureInfo *)params;

	p_handle->mqData.messge = MSG_EXPIRED;
	//copy message to p_handle->mqData.data
	ManufactureSendMessage(&p_handle->mqData);
}

rt_bool_t StartManufacture(void)
{
	rt_bool_t retVal = RT_FALSE;

	if(RT_EOK == rt_thread_startup(manufactureInfo.tid))
	{
	}
	else
	{
		rt_kprintf("StartManufacture failed.\r\n");
	}

	return retVal;
}

void ManufactureControlThread(void *params)
{
	ManufactureInfo *p_handle = (ManufactureInfo *)params;
	MqData_t *pMqData = RT_NULL;

	while (1)
	{
		if(rt_mq_recv(p_handle->manuMq, &pMqData, sizeof(void *), RT_WAITING_FOREVER) == RT_EOK)
		{
			MqData_t mqData;

			rt_memcpy(&mqData, pMqData, sizeof(MqData_t));

			switch(mqData.messge)
			{
			case SMSG_REP_DATA:
				rt_timer_stop(p_handle->manufactureTimer);
				break;
			case MSG_EXPIRED:
				break;
			case MSG_CHECK_DEVICE:
				rt_timer_start(p_handle->manufactureTimer);
				break;
			}
		}
	}
}

void InitManufactureInfo(void)
{
	manufactureInfo.manuMq = RT_NULL;

	manufactureInfo.tid = RT_NULL;
	manufactureInfo.manufactureTimer = RT_NULL;
}

rt_bool_t InitManufacture(void)
{
	rt_kprintf("Init Manufacture\r\n");
	ManufactureInfo *h_data = &manufactureInfo;
	rt_bool_t retVal = RT_FALSE;

	InitManufactureInfo();

	h_data->manufactureTimer = rt_timer_create("WaitResponseData", WaitResponseData, (void *)h_data, rt_tick_from_millisecond(RESPONSE_STANDBY_DELAY), RT_TIMER_FLAG_ONE_SHOT);
	RT_ASSERT(RT_NULL != h_data->manufactureTimer);

	h_data->manuMq = rt_mq_create("ManufauctureMq", sizeof(MqData_t), MANUFACTURE_MQ_SIZE, RT_IPC_FLAG_FIFO);
	RT_ASSERT(RT_NULL != h_data->manuMq);

	h_data->tid = rt_thread_create("ManufactureControlThread", ManufactureControlThread, (void *)h_data, MANUFACTURE_CONTROL_STACK_SIZE, RT_MAIN_THREAD_PRIORITY, 20);
	RT_ASSERT(RT_NULL != h_data->tid);

	retVal = RT_TRUE;

	return retVal;
}
