/*
 * plccomm.c
 *
 */
#include <string.h>
#include <stdlib.h>

#include <rtthread.h>

#include <common/smsgdef.h>
#include <device/uart/plccomm.h>
#include "application.h"
#include <common/common.h>
#include <config/appconfig.h>
#include <config/configuration.h>
#include <network/networkmanager.h>

#define PLC_DATA_LEN	79
#define PLC_BUFF_SIZE	128
#define PV				"PV" //PV
#define TS				"TS"
#define TK				"TK"
#define DEV_TYPE_TS		0 //TS 
#define DEV_TYPE_TK		1 //TK
#define DEV_TYPE_INVALID	2
#define PV_LEN			2
#define TB_VALUE_LEN	2	//2 bytes value len
#define FB_VALUE_LEN	4	//4 bytes value len
#define PLC_COMMUNICATION_TIMEOUT	60000

//Sensor Code
#define SensorCode_1	0x0001
#define SensorCode_2	0x0002
#define SensorCode_3	0x0003
#define SensorCode_4	0x0004
#define SensorCode_5	0x0005
#define SensorCode_6	0x0006
#define SensorCode_7	0x0007
#define SensorCode_8	0x0008
#define SensorCode_9	0x0009
#define SensorCode_10	0x000A
#define SensorCode_11	0x000B
#define SensorCode_12	0x000C
#define SensorCode_13	0x000D
#define SensorCode_14	0x000E
#define SensorCode_15	0x000F
#define SensorCode_16	0x0010
#define SensorCode_17	0x0011
#define SensorCode_18	0x0012
#define SensorCode_19	0x0013

typedef struct _PlcCommInfo{
	rt_event_t rxEvent;
	rt_timer_t plcCommTimeoutTimer;

	rt_uint8_t receiveOn; //Receive On Flag
	rt_uint8_t devInfo[4];
	rt_uint8_t devType; //0: TS, 1: TK
	rt_uint8_t valid;
	rt_uint8_t txBuf[PLC_BUFF_SIZE>>2];
	rt_uint8_t rxBuf[PLC_BUFF_SIZE];
} PlcCommInfo;

PlcCommInfo plcCommInfo;
static rt_uint8_t findStart = DISABLE;
static rt_uint16_t sensorCodeCount = 1;

void PlcCommTimeoutTimer(void *params)
{
	DeviceReboot();
}

//Set Receive On
void SetReceiveOn(rt_uint8_t receiveOn)
{
	rt_kprintf("PLC Data Receive %s\r\n", (ENABLE == receiveOn)?"On":"Off");
	if(receiveOn != plcCommInfo.receiveOn)
	{
		plcCommInfo.receiveOn = receiveOn;
	}
}

void SetSendEvent(void)
{
	if((DEV_TYPE_TS == plcCommInfo.devType) && (SensorCode_11 < sensorCodeCount))
	{
		sensorCodeCount = 1;
	}
	else if((DEV_TYPE_TK == plcCommInfo.devType) && (SensorCode_19 < sensorCodeCount))
	{
		sensorCodeCount = 1;
	}

	if(1 == sensorCodeCount)
	{
		rt_timer_stop(plcCommInfo.plcCommTimeoutTimer);
		rt_memset(plcCommInfo.rxBuf,'\0',PLC_BUFF_SIZE);
	}

	rt_event_send(plcCommInfo.rxEvent, SMSG_INC_DATA);
}

static rt_err_t plcCommRxInd(rt_device_t dev, rt_size_t size)
{
	return rt_event_send(plcCommInfo.rxEvent, SMSG_RX_DATA);
}

//Make Packet following Protocol
rt_size_t MakeSensorValue(rt_uint16_t sensorCode, rt_uint8_t *pData, rt_size_t position)
{
	rt_uint32_t time = 0;
	rt_uint16_t SensorCode = 0;
	rt_uint16_t innerPress = 0;
	rt_uint16_t outerPress = 0;
	rt_uint8_t length = sizeof(time);
	rt_uint8_t 	flaotingPoint = 0;
	rt_uint8_t 	len = sizeof(sensorCode);
	rt_uint8_t *pos =  (plcCommInfo.txBuf+position);

	*pos++ = STX;

	//Time
	time = HTONL(time);
	rt_memcpy(pos,&time,length);
	pos += length;

	length = sizeof(plcCommInfo.devInfo);
	rt_memcpy(pos,plcCommInfo.devInfo,length);
	pos += length;

	if(DEV_TYPE_TS == plcCommInfo.devType)//TS
	{
		innerPress = SensorCode_7;
		outerPress = SensorCode_8;
	}
	else//TK
	{
		innerPress = SensorCode_13;
		outerPress = SensorCode_14;
	}

	//Sensor code, PV
	SensorCode = HTONS(sensorCode);
	rt_memcpy(pos, &SensorCode, len);
	pos += len;
	rt_memcpy(pos, PV, PV_LEN);
	pos += PV_LEN;

	rt_uint32_t u32Val = 0;
	length = sizeof(u32Val);

	//Sensor Value
	rt_memcpy(&u32Val,pData,length);
	u32Val = HTONL(u32Val);
	rt_memcpy(pos,&u32Val,length);
	pos += length;

	if((innerPress == sensorCode) || (outerPress == sensorCode))
	{
		flaotingPoint = 3;
		*pos++ = flaotingPoint;

	}
	else
	{
		flaotingPoint = 1;
		*pos++ = flaotingPoint;
	}

	*pos++ = ETX;

	return (pos-plcCommInfo.txBuf-position);
}

//Check Sensorcode and make packet
static rt_size_t MakeSensorPayloadData(rt_uint8_t *pData, rt_uint16_t length)
{
	rt_size_t size = 0;
	rt_uint8_t *begin = pData;
	rt_uint8_t *pos = plcCommInfo.txBuf;
	rt_uint8_t *p_base = pos;

	begin++;//Skip STX of PLC Data
	//sensor 1
	if( SensorCode_1 ==  sensorCodeCount) //Voltage1 RS
	{
		size += MakeSensorValue(SensorCode_1, begin, size);
	}
	begin += FB_VALUE_LEN;

	//sensor 2
	if( SensorCode_2 ==  sensorCodeCount) //Voltage1 ST
	{
		size += MakeSensorValue(SensorCode_2, begin, size);
	}
	begin += FB_VALUE_LEN;

	//sensor 3
	if( SensorCode_3 ==  sensorCodeCount) //Voltage1 RT
	{
		size += MakeSensorValue(SensorCode_3, begin, size);
	}
	begin += FB_VALUE_LEN;

	//sensor 4
	if( SensorCode_4 ==  sensorCodeCount) //Current1 R
	{
		size += MakeSensorValue(SensorCode_4, begin, size);
	}
	begin += FB_VALUE_LEN;

	//sensor 5
	if( SensorCode_5 ==  sensorCodeCount) //Current1 T
	{
		size += MakeSensorValue(SensorCode_5, begin, size);
	}
	begin += FB_VALUE_LEN;

	//sensor 6
	if( SensorCode_6 ==  sensorCodeCount) //Current1 T
	{
		size += MakeSensorValue(SensorCode_6, begin, size);
	}
	begin += FB_VALUE_LEN;

	//if TS
	if(DEV_TYPE_TS == plcCommInfo.devType)
	{
		begin += FB_VALUE_LEN;//rs
		begin += FB_VALUE_LEN;//st
		begin += FB_VALUE_LEN;//rt
		begin += FB_VALUE_LEN;//r
		begin += FB_VALUE_LEN;//s
		begin += FB_VALUE_LEN;//t

		//sensor 7 //inner press
		if( SensorCode_7 ==  sensorCodeCount)
		{
			size += MakeSensorValue(SensorCode_7, begin, size);
		}
		begin += FB_VALUE_LEN;

		//sensor 8 //outer press
		if( SensorCode_8 ==  sensorCodeCount)
		{
			size += MakeSensorValue(SensorCode_8, begin, size);
		}
		begin += FB_VALUE_LEN;

		//sensor 9
		if( SensorCode_9 ==  sensorCodeCount) //temperature PV
		{
			size += MakeSensorValue(SensorCode_9, begin, size);
		}
		begin += FB_VALUE_LEN;

		//sensor 10
		if( SensorCode_10 ==  sensorCodeCount) //temperature SV
		{
			size += MakeSensorValue(SensorCode_10, begin, size);
		}
		begin += FB_VALUE_LEN;
		begin += FB_VALUE_LEN;
		begin += FB_VALUE_LEN;

		//sensor 11
		if( SensorCode_11 ==  sensorCodeCount) //over temp
		{
			size += MakeSensorValue(SensorCode_11, begin, size);
		}
	}
	else// TK
	{
		//sensor 7
		size += MakeSensorValue(SensorCode_7, begin, size);
		begin += FB_VALUE_LEN;

		//sensor 8
		size += MakeSensorValue(SensorCode_8, begin, size);
		begin += FB_VALUE_LEN;

		//sensor 9
		size += MakeSensorValue(SensorCode_9, begin, size);
		begin += FB_VALUE_LEN;

		//sensor 10
		size += MakeSensorValue(SensorCode_10, begin, size);
		begin += FB_VALUE_LEN;

		//sensor 11
		size += MakeSensorValue(SensorCode_11, begin, size);
		begin += FB_VALUE_LEN;

		//sensor 12
		size += MakeSensorValue(SensorCode_12, begin, size);
		begin += FB_VALUE_LEN;

		//sensor 13 //inner press
		size += MakeSensorValue(SensorCode_13, begin, size);
		begin += FB_VALUE_LEN;

		//sensor 14 //outer press
		size += MakeSensorValue(SensorCode_14, begin, size);
		begin += FB_VALUE_LEN;

		//sensor 15
		size += MakeSensorValue(SensorCode_15, begin, size);
		begin += FB_VALUE_LEN;

		//sensor 16
		size += MakeSensorValue(SensorCode_16, begin, size);
		begin += FB_VALUE_LEN;

		//sensor 17
		size += MakeSensorValue(SensorCode_17, begin, size);
		begin += FB_VALUE_LEN;

		//sensor 18
		size += MakeSensorValue(SensorCode_18, begin, size);
		begin += FB_VALUE_LEN;

		//sensor 19
		size += MakeSensorValue(SensorCode_19, begin, size);
	}

	pos = p_base+size;

	return (pos-plcCommInfo.txBuf);
}

rt_uint8_t CalSum(rt_uint8_t *pData, rt_uint8_t len)
{
	rt_uint8_t result = 0;
	rt_uint8_t buf[PLC_DATA_LEN] = {0,};
	rt_uint8_t count = 0;

	rt_memcpy(buf,pData,len);

	for(count = 0 ; count < len ; count++)
	{
		result += buf[count];
	}

	return result;
}

static rt_size_t ParserReceiveData(rt_uint8_t *p_base, rt_size_t rx_size)
{
	rt_size_t consumed = 0;

	if(RT_NULL != p_base && 0 < rx_size)
	{
		rt_uint8_t *begin=p_base, *end=begin, *last=(p_base+rx_size);

		do {
			if( (STX == *end) && (findStart == DISABLE))
			{
				findStart = ENABLE;
				begin = end;
			}
			else if((findStart == ENABLE) && (PLC_DATA_LEN == (rt_size_t)(end-begin)))
			{
				findStart = DISABLE;
				rt_uint8_t sum = *(begin+PLC_DATA_LEN-1);
				if( sum == CalSum(begin,PLC_DATA_LEN-1))
				{
					plcCommInfo.valid = ENABLE;
#if 0//For monitoring PLC Data
					for(rt_uint8_t i = 0; i < PLC_DATA_LEN; i++ )
					{
						rt_kprintf("%.2x ", *(begin+i));
					}
					rt_kprintf("\r\n");
#endif
				}
			}
		} while(++end < last);
		consumed = (rt_size_t)(begin-p_base);
	}

	return consumed;
}

static void PlcCommRxThread(void *params)
{
    PlcCommInfo *p_handle = (PlcCommInfo *)params;
	rt_uint8_t *pBuf = p_handle->rxBuf;	
	rt_device_t uport = RT_NULL;
	rt_uint32_t events;
	rt_uint8_t uRemain = 0;
	rt_size_t consumed = 0;
	rt_uint8_t indFlag = DISABLE;

	uport = rt_device_find(UART2_DEV_NAME);
	RT_ASSERT(uport != RT_NULL);

	struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

	config.baud_rate = BAUD_RATE_19200;
	rt_device_control(uport, RT_DEVICE_CTRL_CONFIG, (void *)&config);

    rt_device_open(uport, RT_DEVICE_OFLAG_RDONLY |RT_DEVICE_FLAG_INT_RX);

	rt_device_set_rx_indicate(uport, plcCommRxInd);
	while(1)
	{	
		if(RT_EOK == (rt_event_recv(p_handle->rxEvent, (SMSG_RX_DATA|SMSG_INC_DATA), (RT_EVENT_FLAG_OR|RT_EVENT_FLAG_CLEAR), RT_WAITING_FOREVER, &events)) && (ENABLE == p_handle->receiveOn))
		{
			rt_uint8_t ulBytes;

			if((SMSG_RX_DATA & events) && (1 == sensorCodeCount))
			{
				if(PLC_BUFF_SIZE <= uRemain)
				{
					uRemain = 0;
				}

				do {
					if((ulBytes=rt_device_read(uport, 0, (pBuf+uRemain), (PLC_BUFF_SIZE-uRemain))) > 0)
					{
						consumed = ParserReceiveData(pBuf, (uRemain += ulBytes));

						if(consumed > 0 && ((uRemain -= consumed) > 0))
						{
							memmove(pBuf, pBuf+consumed, uRemain);
						}
					}
				} while(0 < ulBytes);

				if(ENABLE == p_handle->valid)
				{
					rt_timer_start(p_handle->plcCommTimeoutTimer);
					p_handle->valid = DISABLE;
					indFlag = ENABLE;
				}
			}
			else if((SMSG_INC_DATA & events) && (1 < sensorCodeCount))
			{
				indFlag = ENABLE;
			}

			if(ENABLE == indFlag)
			{
				rt_uint8_t len = 0;
				if( 0 < (len = MakeSensorPayloadData(p_handle->rxBuf,PLC_DATA_LEN)))
				{
					SendData(p_handle->txBuf,len);
					rt_memset(p_handle->txBuf,'\0',PLC_BUFF_SIZE);
				}

				sensorCodeCount++;

				indFlag = DISABLE;
			}
		}
	}
}

void InitPlcCommInfo(rt_uint8_t *pData)
{
	rt_uint16_t devNum = 0;
	plcCommInfo.receiveOn = DISABLE;
	plcCommInfo.valid = DISABLE;

	rt_memcpy(plcCommInfo.devInfo,pData,sizeof(devNum));
	devNum = strtoul((char *)pData+2,0,10);
	devNum = HTONS(devNum);
	rt_memcpy(plcCommInfo.devInfo+2,&devNum,sizeof(devNum));

	if(0 == rt_strncmp((char *)plcCommInfo.devInfo,TS,rt_strlen(TS)) )
	{
		plcCommInfo.devType = DEV_TYPE_TS; //TS
	}
	else if(0 == rt_strncmp((char *)plcCommInfo.devInfo,TK,rt_strlen(TK)))
	{
		plcCommInfo.devType = DEV_TYPE_TK; //TK
	}
	else
	{
		plcCommInfo.devType = DEV_TYPE_INVALID;
		rt_kprintf("Invalid Device\r\n");
	}
}	

rt_bool_t InitPlcComm(void)
{
	rt_kprintf("Initialize PlcComm.\r\n");
    PlcCommInfo *h_data = &plcCommInfo;
	rt_thread_t tidRx;

    InitPlcCommInfo(GetDeviceInfo());

    h_data->rxEvent = rt_event_create("plccommRx", RT_IPC_FLAG_FIFO);
    RT_ASSERT(h_data->rxEvent != RT_NULL);

    h_data->plcCommTimeoutTimer = rt_timer_create("plcCommTimeoutTimer", PlcCommTimeoutTimer, RT_NULL, rt_tick_from_millisecond(PLC_COMMUNICATION_TIMEOUT), RT_TIMER_FLAG_ONE_SHOT);;
    RT_ASSERT(h_data->plcCommTimeoutTimer != RT_NULL);

    tidRx = rt_thread_create("plcCommRxThread", PlcCommRxThread, (void *)h_data, PLCCOMMM_STACK_SIZE, RT_MAIN_THREAD_PRIORITY, 20);
    RT_ASSERT(tidRx != RT_NULL);

    /* thread start  */
    return (RT_EOK == rt_thread_startup(tidRx));
}
