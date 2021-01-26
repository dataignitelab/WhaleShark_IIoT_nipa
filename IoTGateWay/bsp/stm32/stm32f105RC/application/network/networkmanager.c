/*
 * networkmanager.c
 *
 */
#include <rtthread.h>

#include <config/appconfig.h>
#include <config/configuration.h>
#include <network/wifi/wifi.h>
#include <device/uart/plccomm.h>

#include "application.h"
#include "networkmanager.h"

#define TCP_BUFFER_MAX	64
#define RETRY_MAX		5
#define TCP_RESPONSE_TIMEOUT_DELAY	10000
#define START_IOT_GATEWAY_DELAY		10000

typedef struct _NetworkInfo
{
	rt_mq_t networkManagerMq;
	rt_timer_t	responseTimeoutTimer;
	rt_timer_t	startIoTGateWayTimer;

	NetworkInfoData networkInfoData;
	rt_uint16_t sendLength; //Set Tcp data length
	rt_uint8_t retryCount; //retry count variable

	rt_uint8_t sendBuffer[TCP_BUFFER_MAX];
}NetworkInfo;

NetworkInfo networkInfo;
static rt_thread_t tid;

void ResponseTimeoutTimer(void *params)
{
	rt_kprintf("Receive TimeOut\r\n");
	SendWifiCommand(CMD_TCP_DISCONNECT);
}

void StartIoTGateWayTimer(void *params)
{
	SendWifiCommand(CMD_ECHO_OFF);
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
	if( 0 < dataSize)
	{
		MqData_t data;
		data.messge = SMSG_SEND_DATA;
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
	rt_uint8_t cmd = CMD_MAX;

	rt_timer_start(p_handle->startIoTGateWayTimer);

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
					rt_kprintf("Success restarting WiFi Module\r\n");
					beReboot = ENABLE;
					break;
				case STATUS_ECHO_OFF:
					rt_kprintf("Success setting echo off\r\n");
					cmd = CMD_QUERY_CONNECTION_STATUS;
					break;
				case STATUS_SET_RFPOWER:
					cmd = CMD_DISABLE_SLEEP_MODE;
					break;
				case STATUS_DISABLE_SLEEP_MODE:
					cmd = CMD_QUERY_MODE;
					break;
				case STATUS_GET_WIFI_MODE:
					switch(p_handle->networkInfoData.data[0])
					{
					case StationMode:
						cmd = CMD_QUERY_DHCP;
						break;
					case NullMode:
					case SoftApMode:
					case SoftAp_StationMode:
						cmd = CMD_SET_MODE;
						break;
					default:
						rt_kprintf("Invalid Mode \r\n");
						break;
					}
					break;
				case STATUS_SET_WIFI_MODE:
					rt_kprintf("Success Setting Wi-Fi Mode\r\n");
					cmd = CMD_QUERY_DHCP;
					break;
				case STATUS_CONNECT_WIFI:
					rt_kprintf("AP Connection Success\r\n");
					ledEvent = ENABLE;
					if(ENABLE == GetMqttOn())
					{
						cmd = CMD_MQTT_QUERY_CONNECT_STATE;
					}
					else
					{
						cmd = CMD_DELETE_IP_PORT_WITH_DATA;
					}
					break;
				case STATUS_GET_DHCP_INFO:
					if( RT_TRUE == p_handle->networkInfoData.data[DHCP_COMPARE_RESULT]) //Result
					{
						cmd = CMD_CONNECT_AP;
					}
					else
					{
						cmd = CMD_SET_DHCP;//Set DHCP
					}
					break;
				case STATUS_SET_DHCP:
					rt_kprintf("Set DHCP Success\r\n");
					cmd = CMD_CONNECT_AP;
					break;
				case STATUS_GET_AP_INFO:
					if( RT_TRUE == p_handle->networkInfoData.data[DHCP_MODE_INFO])
					{
						cmd = CMD_TCP_CONNECT;//if dhcp enable
					}
					else
					{
						cmd = CMD_QUERY_LOCAL_INFO;//if dhcp disable
					}
					break;
				case STATUS_SET_LOCAL_INFO:
					cmd = CMD_QUERY_LOCAL_INFO;//if dhcp disable
					break;
				case STATUS_GET_LOCAL_INFO:
					if( CHANGE_LOCAL_INFO != p_handle->networkInfoData.data[LOCAL_IP_INDEX] && CHANGE_LOCAL_INFO != p_handle->networkInfoData.data[GATEWAY_IP_INDEX] &&  CHANGE_LOCAL_INFO != p_handle->networkInfoData.data[NETMASK_IP_INDEX])
					{
						cmd = CMD_TCP_CONNECT;//TCP connect
					}
					else
					{
						cmd = CMD_SET_LOCAL_INFO;// Set Local Information
					}
					break;
				case STATUS_TCP_CONNECT:
					rt_kprintf("TCP Connection Success\r\n");
					ledEvent = ENABLE;
					cmd = CMD_SET_TRANSMISSION_MODE;
					break;
				case STATUS_SET_TRANSMISSION_MODE:
					SetReceiveOn(ENABLE);
					break;
				case STATUS_TCP_DISCONNECT:
					rt_kprintf("TCP Disconnection Success \r\n");
					cmd = CMD_RESTART;
					break;
				case STATUS_DELETE_IP_PORT_WITH_DATA:
					cmd = CMD_QUERY_AP_INFO;
					break;
				case STATUS_MQTT_USER_CFG:
				case STATUS_MQTT_CONNECT_CFG:
					cmd = CMD_MQTT_CONNECT;
					break;
				case STATUS_MQTT_CONNECT:
					rt_kprintf("Success Connecting MQTT \r\n");
					cmd = CMD_MQTT_SUB;
					break;
				case STATUS_MQTT_SUB:
					rt_kprintf("Success Setting Topic \r\n");
					break;
				case STATUS_MQTT_GET_CONNECT_STATE:
					//rt_kprintf("MQTT State Value: %c \r\n",p_handle->networkInfoData.data[MQTT_STATE_INDEX]);
					switch(p_handle->networkInfoData.data[MQTT_STATE_INDEX])
					{
					case MQTT_UNINITIALIZED:
					case MQTT_ALREADY_SET_USER_CFG:
					case MQTT_ALREADY_SET_CONNECTION_CFG:
						cmd = CMD_MQTT_USER_CFG;
						break;
					case MQTT_DISCONNECTED:
					case MQTT_CONNECTION_ESTABLISHED:
						cmd = CMD_MQTT_DISCONNECT;
						break;
					default:
						break;
					}
					break;
				case STATUS_MQTT_DISCONNECT:
					rt_kprintf("Success Disconnecting MQTT\r\n");
					cmd = CMD_MQTT_USER_CFG;
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
				case STATUS_TCP_CONNECT:
					cmd = CMD_TCP_CONNECT;//TCP connect
					break;
				case STATUS_SET_DHCP:
					cmd = CMD_SET_DHCP;
					break;
				case STATUS_MQTT_CONNECT:
					break;
				case STATUS_MQTT_USER_CFG:
					break;
				case STATUS_SET_SEND_DATA_LENGTH:
					break;
				case STATUS_MQTT_SUB:
					break;
				default:
					cmd = CMD_TCP_DISCONNECT;
					break;
				}
				break;
			case SMSG_WIFI_FAIL:
				rt_kprintf("[%d]Wi-Fi Response: FAIL\r\n",p_handle->networkInfoData.networkStatus);
				ledEvent = ENABLE;
				switch(p_handle->networkInfoData.networkStatus)
				{
				case STATUS_CONNECT_WIFI:
					cmd = CMD_CONNECT_AP;
					break;
				default:
					cmd = CMD_TCP_DISCONNECT;
					break;
				}
				break;
			case SMSG_WIFI_TIMEOUT:
				rt_kprintf("[%d]Wi-Fi Response Timeout\r\n",p_handle->networkInfoData.networkStatus);
				switch(p_handle->networkInfoData.networkStatus)
				{
				case STATUS_ECHO_OFF:
					cmd = CMD_ECHO_OFF;
					break;
				case STATUS_QUERY_CONNECTION_STATUS:
					cmd = CMD_QUERY_CONNECTION_STATUS;
					break;
				case STATUS_CONNECT_WIFI:
					cmd = CMD_CONNECT_AP;
					break;
				case STATUS_WIFI_RESTART:
					cmd = CMD_RESTART;
					break;
				case STATUS_SET_SEND_DATA_LENGTH:
					NetworkSetDataLength(p_handle->sendLength);
					break;
				default:
					cmd = CMD_TCP_DISCONNECT;
					break;
				}
				break;
			case SMSG_SEND_DATA:
				rt_memcpy(&p_handle->sendBuffer, mqData.data, mqData.size);
				p_handle->sendLength = mqData.size;
				NetworkSetDataLength(p_handle->sendLength);
				break;
			case SMSG_READY_TO_SEND:
				NetworkSendData(p_handle->sendBuffer,p_handle->sendLength );
				break;
			case SMSG_SEND_OK:
				rt_timer_stop(p_handle->responseTimeoutTimer);
				rt_timer_start(p_handle->responseTimeoutTimer);
				rt_memset(&p_handle->sendBuffer,'\0',TCP_BUFFER_MAX); //init buffer
				break;
			case SMSG_SEND_FAIL:
				rt_kprintf("TCP Send Fail\r\n");
				cmd = CMD_TCP_DISCONNECT;
				break;
			case SMSG_GET_TCP_STATUS:
				switch(p_handle->networkInfoData.data[0])
				{
				case TcpConnected:
					rt_kprintf("TCP Connected\r\n");
					cmd = CMD_TCP_DISCONNECT;
					break;
				default:
					cmd = CMD_SET_POWER;
					break;
				}
				break;
			case SMSG_RECEIVE_ACK:
				rt_timer_stop(p_handle->responseTimeoutTimer);
				switch(p_handle->networkInfoData.networkStatus)
				{
				case STATUS_RECEIVE_DATA:
					p_handle->retryCount = 0;
				case STATUS_REFUSE_DATA:
					if(RETRY_MAX > p_handle->retryCount++)
					{
						SetSendEvent();
					}
					else
					{
						cmd = CMD_TCP_DISCONNECT;
					}
					break;
				default:
					break;
				}
				break;
			}

			if( RETRY_MAX > p_handle->retryCount++)
			{
				if(CMD_MAX != cmd)
				{
					rt_kprintf("Send Command[%d]\r\n",cmd);
					SendWifiCommand(cmd);
				}
			}
			else
			{
				if(CMD_RESTART == cmd)
				{
					WifiHardwareReset();
				}
				beReboot = ENABLE;
			}
			cmd = CMD_MAX;

			if(ENABLE == beReboot)
			{
				beReboot = DISABLE;
				p_handle->retryCount = 0;
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
	return rt_thread_startup(tid);
}

void InitNetworkManagerInformation(void)
{
	networkInfo.sendLength = 0;
	networkInfo.retryCount = 0;
	networkInfo.networkManagerMq = RT_NULL;
}

void DeInitNetworkManager(void)
{
	DeInitWifi();
	rt_kprintf("DeInitialize Network Manager.\r\n");

	rt_timer_stop(networkInfo.responseTimeoutTimer);
	rt_timer_stop(networkInfo.startIoTGateWayTimer);

	if(RT_EOK != rt_mq_delete(networkInfo.networkManagerMq))
	{
		rt_kprintf("Deleting Network Manager Message Queue Failed.\r\n");
	}
}

rt_bool_t InitNetworkManager(void)
{
	NetworkInfo *h_data = &networkInfo;
	rt_bool_t retVal = RT_FALSE;

	InitNetworkManagerInformation();

	h_data->networkManagerMq = rt_mq_create("networkManagerMq", sizeof(MqData_t), NETWORKMANAGER_MQ_SIZE, RT_IPC_FLAG_FIFO);
	RT_ASSERT(RT_NULL != h_data->networkManagerMq);

	tid = rt_thread_create("networkmanager", NetworkManagerThread, (void *)h_data, NETWORKMANAGER_STACK_SIZE, RT_MAIN_THREAD_PRIORITY, 20);
	RT_ASSERT(RT_NULL != tid);

	h_data->responseTimeoutTimer = rt_timer_create("responseTimeoutTimer", ResponseTimeoutTimer, RT_NULL, rt_tick_from_millisecond(TCP_RESPONSE_TIMEOUT_DELAY), RT_TIMER_FLAG_ONE_SHOT);
	RT_ASSERT(RT_NULL != h_data->responseTimeoutTimer);

	h_data->startIoTGateWayTimer = rt_timer_create("startIoTGateWayTimer", StartIoTGateWayTimer, RT_NULL, rt_tick_from_millisecond(START_IOT_GATEWAY_DELAY), RT_TIMER_FLAG_ONE_SHOT);
	RT_ASSERT(RT_NULL != h_data->startIoTGateWayTimer);

	return (retVal=InitWifi());
}
