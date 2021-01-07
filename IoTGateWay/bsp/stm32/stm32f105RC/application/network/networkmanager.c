/*
 * networkmanager.c
 *
 */
#include <rtthread.h>

#include <config/appconfig.h>
#include <network/wifi/wifi.h>
#include <device/uart/plccomm.h>

#include "application.h"
#include "networkmanager.h"

#define TCP_BUFFER_MAX	256
#define RETRY_MAX		5
#define TCP_RESPONSE_TIMEOUT_DELAY	10000

typedef struct _NetworkInfo
{
	rt_mq_t networkManagerMq;
	rt_thread_t tid;
	rt_timer_t	responseTimeoutTimer;

	NetworkInfoData networkInfoData;
	rt_uint16_t tcpSendLength; //Set Tcp data length
	rt_uint8_t retryCount; //retry count variable

	rt_uint8_t tcpSendBuffer[TCP_BUFFER_MAX];
}NetworkInfo;

NetworkInfo networkInfo;

void ResponseTimeoutTimer(void *params)
{
	rt_kprintf("Receive TimeOut\r\n");
	DeviceReboot();
}

void NetworkMangerSendMessage(MqData_t *pMqData)
{
	if(-RT_EFULL == rt_mq_send(networkInfo.networkManagerMq, (void *)&pMqData, sizeof(void*)))
	{
		rt_kprintf("NetworkManager Queue full - throw away\r\n");
	}
}

void SendData(rt_uint8_t *pData, rt_size_t dataSize)
{
	if( 0 < dataSize && (STATUS_STANDBY_SEND == networkInfo.networkInfoData.networkStatus))
	{
		MqData_t data;
		networkInfo.networkInfoData.networkStatus = STATUS_SET_SEND_DATA_LENGTH;
		data.messge = SMSG_TCP_SEND_DATA;
		data.size = dataSize;
		rt_memcpy(data.data,pData,dataSize);
		NetworkMangerSendMessage(&data);
	}
}

void NetworkManagerThread(void *params)
{
	NetworkInfo *p_handle = (NetworkInfo *)params;
	MqData_t *pMqData = RT_NULL;
	rt_uint8_t beReboot = DISABLE;
	rt_uint8_t ledEvent = DISABLE;

	SendWifiCommand(CMD_RESTART);

	while(1)
	{
		if(rt_mq_recv(p_handle->networkManagerMq, &pMqData, sizeof(void *), RT_WAITING_FOREVER) == RT_EOK)
		{
			MqData_t mqData;
			rt_memcpy(&mqData, pMqData, sizeof(mqData));
			rt_memcpy(&p_handle->networkInfoData, mqData.data, sizeof(p_handle->networkInfoData));

			switch(mqData.messge)
			{
			case SMSG_WIFI_OK:
				p_handle->retryCount = 0;
				switch(p_handle->networkInfoData.networkStatus)
				{
				case STATUS_WIFI_RESTART:
					SendWifiCommand(CMD_ECHO_OFF);
					break;
				case STATUS_ECHO_OFF:
					SendWifiCommand(CMD_SET_POWER);
					break;
				case STATUS_SET_RFPOWER:
					SendWifiCommand(CMD_DISABLE_SLEEP_MODE);
					break;
				case STATUS_DISABLE_SLEEP_MODE:
					SendWifiCommand(CMD_QUERY_MODE);
					break;
				case STATUS_GET_WIFI_MODE:
					switch(p_handle->networkInfoData.data[0])
					{
					case StationMode:
						SendWifiCommand(CMD_QUERY_DHCP);
						break;
					case NullMode:
					case SoftApMode:
					case SoftAp_StationMode:
						SendWifiCommand(CMD_SET_MODE);
						break;
					default:
						rt_kprintf("Invalid Mode \r\n");
						break;
					}
					break;
				case STATUS_SET_WIFI_MODE:
					rt_kprintf("Success Setting Wi-Fi Mode\r\n");
					SendWifiCommand(CMD_QUERY_DHCP);
					break;
				case STATUS_CONNECT_WIFI:
					rt_kprintf("AP Connection Success\r\n");
					ledEvent = ENABLE;
					SendWifiCommand(CMD_QUERY_AP_INFO);
					break;
				case STATUS_GET_DHCP_INFO:
					if( RT_TRUE == p_handle->networkInfoData.data[DHCP_COMPARE_RESULT]) //Result
					{
						SendWifiCommand(CMD_CONNECT_AP);
					}
					else
					{
						SendWifiCommand(CMD_SET_DHCP); //Set DHCP
					}
					break;
				case STATUS_SET_DHCP:
					rt_kprintf("Set DHCP Success\r\n");
					SendWifiCommand(CMD_CONNECT_AP);
					break;
				case STATUS_GET_AP_INFO:
					if( RT_TRUE == p_handle->networkInfoData.data[DHCP_MODE_INFO])
					{
						SendWifiCommand(CMD_TCP_CONNECT);//if dhcp enable
					}
					else
					{
						SendWifiCommand(CMD_QUERY_LOCAL_INFO);//if dhcp disable
					}
					break;
				case STATUS_SET_LOCAL_INFO:
					SendWifiCommand(CMD_QUERY_LOCAL_INFO);//if dhcp disable
					break;
				case STATUS_GET_LOCAL_INFO:
					if( CHANGE_LOCAL_INFO != p_handle->networkInfoData.data[LOCAL_IP_INDEX] && CHANGE_LOCAL_INFO != p_handle->networkInfoData.data[GATEWAY_IP_INDEX] &&  CHANGE_LOCAL_INFO != p_handle->networkInfoData.data[NETMASK_IP_INDEX])
					{
						SendWifiCommand(CMD_TCP_CONNECT); //TCP connect
					}
					else
					{
						SendWifiCommand(CMD_SET_LOCAL_INFO); // Set Local Information
					}
					break;
				case STATUS_TCP_CONNECT:
					rt_kprintf("TCP Connection Success\r\n");
					ledEvent = ENABLE;
					if( DISABLE == GetTcpStatus())
					{
						SetTcpStatus(ENABLE);
						p_handle->networkInfoData.networkStatus = STATUS_STANDBY_SEND;
					}
					break;
				case STATUS_READY_TO_SEND_DATA:
					TcpSendData(p_handle->tcpSendBuffer,p_handle->tcpSendLength );
					break;
				case STATUS_SEND_DATA:
					p_handle->networkInfoData.networkStatus = STATUS_WAIT_ACK;
					rt_timer_start(p_handle->responseTimeoutTimer);
					break;
				case STATUS_TCP_DISCONNECT:
					rt_kprintf("TCP Disconnection Success \r\n");
					SendWifiCommand(CMD_TCP_CONNECT); //TCP connect
					break;
				case STATUS_RECEIVE_DATA:
					SetTcpStatus(ENABLE);
					rt_timer_stop(p_handle->responseTimeoutTimer);
					p_handle->networkInfoData.networkStatus = STATUS_STANDBY_SEND;
					SetSendEvent();
					break;
				default:
					break;
				}
				break;
			case SMSG_WIFI_ERROR:
				rt_kprintf("[%d]Wi-Fi Response: ERROR\r\n",p_handle->networkInfoData.networkStatus);
				ledEvent = ENABLE;
				switch(p_handle->networkInfoData.networkStatus)
				{
				case STATUS_SEND_DATA:
					if( RETRY_MAX > p_handle->retryCount++)
					{
						rt_thread_delay(2000);
						SetTcpStatus(DISABLE);
						TcpSendData(p_handle->tcpSendBuffer,p_handle->tcpSendLength );
					}
					else
					{
						beReboot = ENABLE;
					}
					break;
				case STATUS_READY_TO_SEND_DATA:
					if( RETRY_MAX > p_handle->retryCount++)
					{
						TcpSendData(p_handle->tcpSendBuffer,p_handle->tcpSendLength );
					}
					else
					{
						beReboot = ENABLE;
					}
					break;
				case STATUS_TCP_CONNECT:
					if( RETRY_MAX > p_handle->retryCount++)
					{
						SendWifiCommand(CMD_TCP_CONNECT); //TCP connect
					}
					else
					{
						beReboot = ENABLE;
					}
					break;
				case STATUS_TCP_DISCONNECT:
				case STATUS_REFUSE_DATA:
					beReboot = ENABLE;
					break;
				default:
					if(ENABLE == GetTcpStatus())
					{
						SendWifiCommand(CMD_TCP_DISCONNECT);
					}
					else
					{
						beReboot = ENABLE;
					}
					break;
				}
				break;
			case SMSG_WIFI_FAIL:
				rt_kprintf("[%d]Wi-Fi Response: FAIL\r\n",p_handle->networkInfoData.networkStatus);
				ledEvent = ENABLE;
				switch(p_handle->networkInfoData.networkStatus)
				{
				case STATUS_CONNECT_WIFI:
					if( RETRY_MAX > p_handle->retryCount++)
					{
						SendWifiCommand(CMD_CONNECT_AP);
					}
					else
					{
						beReboot = ENABLE;
					}
					break;
				case STATUS_SEND_DATA:
					beReboot = ENABLE;
					break;
				default:
					break;
				}
				break;
			case SMSG_WIFI_TIMEOUT:
				rt_kprintf("[%d]Wi-Fi Response Timeout\r\n",p_handle->networkInfoData.networkStatus);
				switch(p_handle->networkInfoData.networkStatus)
				{
				case STATUS_CONNECT_WIFI:
					if( RETRY_MAX > p_handle->retryCount++)
					{
						SendWifiCommand(CMD_CONNECT_AP);
					}
					else
					{
						beReboot = ENABLE;
					}
					break;
				default:
					if(ENABLE == GetTcpStatus())
					{
						SendWifiCommand(CMD_TCP_DISCONNECT);
					}
					else
					{
						beReboot = ENABLE;
					}
					break;
				}
				break;
			case SMSG_TCP_SEND_DATA:
				rt_memset(&p_handle->tcpSendBuffer,'\0',TCP_BUFFER_MAX); //init buffer before transmit
				rt_memcpy(&p_handle->tcpSendBuffer, mqData.data, mqData.size);
				p_handle->tcpSendLength = mqData.size;
				TcpSetDataLength(p_handle->tcpSendLength);
				break;
			default:
				break;
			}

			if(ENABLE == beReboot)
			{
				DeviceReboot();
			}
			else if(ENABLE == ledEvent)
			{
				ledEvent = DISABLE;
				ApplicationSendMessage(&mqData);
			}
		}
	}
}

rt_bool_t StartNetworkManager(void)
{
	rt_thread_delay(10000);//To wait init Wi-Fi Module
	return rt_thread_startup(networkInfo.tid);
}

void InitNetworkManagerInformation(void)
{
	networkInfo.tcpSendLength = 0;
	networkInfo.retryCount = 0;
	networkInfo.networkManagerMq = RT_NULL;
	networkInfo.tid = RT_NULL;
}

void DeInitNetworkManager(void)
{

}

rt_bool_t InitNetworkManager(void)
{
	NetworkInfo *h_data = &networkInfo;
	rt_bool_t retVal = RT_FALSE;

	InitNetworkManagerInformation();

	h_data->networkManagerMq = rt_mq_create("networkManagerMq", sizeof(MqData_t), NETWORKMANAGER_MQ_SIZE, RT_IPC_FLAG_FIFO);
	RT_ASSERT(RT_NULL != h_data->networkManagerMq);

	h_data->tid = rt_thread_create("networkmanager", NetworkManagerThread, (void *)h_data, NETWORKMANAGER_STACK_SIZE, RT_MAIN_THREAD_PRIORITY, 20);
	RT_ASSERT(RT_NULL != h_data->tid);

	h_data->responseTimeoutTimer = rt_timer_create("responseTimeoutTimer", ResponseTimeoutTimer, (void *)h_data, rt_tick_from_millisecond(TCP_RESPONSE_TIMEOUT_DELAY), RT_TIMER_FLAG_ONE_SHOT);
	RT_ASSERT(RT_NULL != h_data->responseTimeoutTimer);

	return (retVal=InitWifi());
}
