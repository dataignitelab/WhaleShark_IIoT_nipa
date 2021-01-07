/*
 * wifi.c
 *
 */
#include <stdlib.h>

#include <rtthread.h>
#include <config/appconfig.h>
#include <config/configuration.h>
#include <common/smsgdef.h>
#include <network/networkmanager.h>

#include "wifi.h"

#define WIFI_RX_BUFF	1024
#define WIFI_CMD_PREFIX	"AT+"
#define WIFI_OK			"OK"
#define WIFI_ERROR		"ERROR"
#define WIFI_FAIL		"FAIL"
#define TCP				"TCP"
#define IP				"ip"
#define GATEWAY			"gateway"
#define NETMASK			"netmask"
#define SEND_DATA_OK	"SEND OK"
#define SEND_DATA_FAIL	"SEND FAIL"
#define TCP_RECEIVE		"+IPD" //Received Data
#define ENTER_THE_DATA	">" //Enter The Data
#define TCP_ERROR		"ER"
#define WIFI_BAUD_RATE	BAUD_RATE_115200
#define CMD_MAX_LEN		256 //128 -> 256
#define WIFI_MAX_LEN	64
#define WIFI_RESP_TIMEOUT_DELAY	30000
#define WIFI_SEND_TIMEOUT_DELAY	10000

// ID, COMMAND, COMMAND DESCRIPTION
static WIFICOMMANDINFO commandInfo[] = {
	{CMD_RESTART,"RST","[Restart.]\r\n"},
	{CMD_ECHO_OFF, "ATE0", "[Sets Echo Off.]\r\n"},
	{CMD_SET_POWER,"RFPOWER","[Set RF Power]\r\n"},
	{CMD_DISABLE_SLEEP_MODE, "SLEEP", "[Sets Sleep Mode.]\r\n"},
	{CMD_SET_MODE, "CWMODE_DEF", "[Sets the default Wi-Fi mode (Station/AP/Station+AP).]\r\n"},
	{CMD_QUERY_MODE, "CWMODE_DEF?", "[Query the current Wi-Fi mode.]\r\n"},
	{CMD_GET_MODE, "CWMODE_DEF:", "[Get the current Wi-Fi mode.]\r\n"},
	{CMD_CONNECT_AP, "CWJAP_DEF", "[Connects to an AP. configuration saved in the flash.]\r\n"},
	{CMD_QUERY_AP_INFO, "CWJAP_DEF?", "[Query the connected AP Info.]\r\n"},
	{CMD_GET_AP_INFO, "CWJAP_DEF:", "[Get the connected AP Info.]\r\n"},
	{CMD_SET_DHCP,"CWDHCP_DEF","[Set DHCP Mode]\r\n"},
	{CMD_QUERY_DHCP,"CWDHCP_DEF?","[Query DHCP Mode]\r\n"},
	{CMD_GET_DHCP,"CWDHCP_DEF:","[Get DHCP Mode]\r\n"},
	{CMD_SET_LOCAL_INFO,"CIPSTA_DEF","[Set Local Info.]\r\n"},
	{CMD_QUERY_LOCAL_INFO,"CIPSTA_DEF?","[Query Local Info.]\r\n"},
	{CMD_GET_LOCAL_INFO,"CIPSTA_DEF:","[Get Local Info. (Local IP, Gateway, netmask)]\r\n"},
	{CMD_DISCONNECT_AP, "CWQAP", "[Disconnects from an AP.]\r\n"},
	{CMD_TCP_CONNECT, "CIPSTART", "[Single TCP Connection.]\r\n"},
	{CMD_SET_SEND_DATA_LENGTH,"CIPSEND","[Set Data Length.]\r\n"},
	{CMD_QUERY_MAC_INFO,"CIPSTAMAC_DEF?","[Query Mac Address.]\r\n"},
	{CMD_GET_MAC_INFO,"CIPSTAMAC_DEF:","[Get Mac Address.]\r\n"},
	{CMD_SET_MAC_INFO,"CIPSTAMAC_DEF","[Set Mac Address.]\r\n"},
	{CMD_RECEIVE_DATA,"IPD","[Receive Data.]\r\n"},
	{CMD_TCP_DISCONNECT,"CIPCLOSE","[Close TCP Connection]\r\n"}
};

typedef struct _WifiInfo
{
	MqData_t mqData;

	rt_device_t uport;
	rt_event_t rx_event;
	rt_timer_t	wifiTimeoutTimer;
	rt_timer_t	sendTimeoutTimer; //Send Tumeout Timer
	NetworkInfoData networkInfoData;

	rt_uint8_t responseLen;
	rt_uint16_t txLen;

	//dhcp
	rt_uint8_t mode;
	rt_uint8_t dhcp;

	//Wi-Fi Info
	rt_uint8_t ssid[AP_MAX_SSID_LENGTH<<1];
	rt_uint8_t password[AP_MAX_PASSWORD_LENGTH<<1];

	//TCP Info
	rt_uint8_t domainConfig;
	rt_uint8_t remoteIp[IP_MAX_LENGTH];
	rt_uint16_t remotePort;
	rt_uint8_t domain[WIFI_MAX_LEN];

	rt_uint8_t txBuf[CMD_MAX_LEN];
	rt_uint8_t rxBuf[RT_SERIAL_RB_BUFSZ];
}WifiInfo;

WifiInfo wifiInfo;

void WifiTimeoutTimer(void *params)
{
	WifiInfo *p_handle = (WifiInfo *)params;

	p_handle->mqData.messge = SMSG_WIFI_TIMEOUT;
    //Copy the Network Status
	rt_memcpy(p_handle->mqData.data,&p_handle->networkInfoData,p_handle->mqData.size=sizeof(p_handle->networkInfoData.data));
	NetworkMangerSendMessage(&p_handle->mqData);

	p_handle->networkInfoData.response = RESPONSE_WIFI_RESPONSE_MAX;
}

void SendTimeoutTimer(void *params)
{
	WifiInfo *p_handle = (WifiInfo *)params;

	p_handle->mqData.messge = SMSG_WIFI_ERROR;
	NetworkMangerSendMessage(&p_handle->mqData);

	p_handle->networkInfoData.response = RESPONSE_WIFI_RESPONSE_MAX;
}

static rt_err_t WifiRxIndicate(rt_device_t dev, rt_size_t size)
{
    return rt_event_send(wifiInfo.rx_event, SMSG_RX_DATA);
}

static rt_size_t WifiTxData(rt_uint8_t *data, rt_size_t tx_size)
{
	rt_int32_t remain = tx_size;
	rt_size_t written = 0;

	if(RT_NULL != data && 0 < tx_size)
	{
		do {
			if(0 < (written=rt_device_write(wifiInfo.uport, 0, (data+written), remain)))
			{
				remain -= written;
			}
		} while(0 < remain);
	}

	return written;
}

rt_uint8_t *MakeWifiFormat(rt_uint8_t *pData)
{
	rt_uint8_t *value = RT_NULL;
	if(RT_NULL != pData)
	{
		rt_uint8_t len = rt_strlen((char *)pData);
		rt_uint8_t *pos=pData, *begin = RT_NULL, *end = RT_NULL;
		begin = pos;
		do{
			if( ('0'< *pos && '9' > *pos) || ('A'< *pos && 'Z' > *pos) || ('a'< *pos && 'z' > *pos) )
			{
				end = pos++;
			}
			else if(0x80 > *pos)
			{
				rt_uint8_t buf[64];
				end = pos;
				rt_memcpy(buf,end,len);
				*end = 0x5c; // '\'

				pos++;
				rt_memcpy(pos++,buf,len);
			}
		}while(len-- && '\0'!= *pos);
		value = begin;
	}

	return value;
}

rt_err_t SendWifiCommand(wifiCommandID id)
{
	rt_thread_delay(500);
	rt_uint8_t cmd[CMD_MAX_LEN];
	rt_uint8_t len = 0;
	switch(id)
	{
	case CMD_RESTART:
		wifiInfo.networkInfoData.networkStatus = STATUS_WIFI_RESTART; //set status
		len = rt_sprintf((char *)cmd,"%s%s",WIFI_CMD_PREFIX,commandInfo[CMD_RESTART].szCli);
		break;
	case CMD_ECHO_OFF: //uart echo off
		wifiInfo.networkInfoData.networkStatus = STATUS_ECHO_OFF;
		len = rt_sprintf((char *)cmd,"%s",commandInfo[CMD_ECHO_OFF].szCli);
		break;
	case CMD_SET_POWER:
		wifiInfo.networkInfoData.networkStatus = STATUS_SET_RFPOWER;
		len = rt_sprintf((char *)cmd,"%s%s=82",WIFI_CMD_PREFIX,commandInfo[CMD_SET_POWER].szCli);
		break;
	case CMD_DISABLE_SLEEP_MODE:
		wifiInfo.networkInfoData.networkStatus = STATUS_DISABLE_SLEEP_MODE;
		len = rt_sprintf((char *)cmd,"%s%s=0",WIFI_CMD_PREFIX,commandInfo[CMD_DISABLE_SLEEP_MODE].szCli);
		break;
	case CMD_SET_MODE:
		rt_kprintf("Set Station Mode\r\n");
		wifiInfo.networkInfoData.networkStatus = STATUS_SET_WIFI_MODE;
		len = rt_sprintf((char *)cmd,"%s%s=%c",WIFI_CMD_PREFIX,commandInfo[CMD_SET_MODE].szCli,StationMode);
		break;
	case CMD_QUERY_MODE:
		wifiInfo.networkInfoData.networkStatus = STATUS_CHECK_WIFI_MODE;
		wifiInfo.responseLen = rt_strlen(commandInfo[CMD_GET_MODE].szCli);
		len = rt_sprintf((char *)cmd,"%s%s",WIFI_CMD_PREFIX,commandInfo[CMD_QUERY_MODE].szCli);
		break;
	case CMD_SET_DHCP:
		wifiInfo.networkInfoData.networkStatus = STATUS_SET_DHCP;
		len = rt_sprintf((char *)cmd,"%s%s=%d,%d",WIFI_CMD_PREFIX,commandInfo[CMD_SET_DHCP].szCli,wifiInfo.mode,wifiInfo.dhcp);
		break;
	case CMD_QUERY_DHCP:
		wifiInfo.responseLen = rt_strlen(commandInfo[CMD_GET_DHCP].szCli);
		wifiInfo.networkInfoData.networkStatus = STATUS_QUERY_DHCP_INFO;
		len = rt_sprintf((char *)cmd,"%s%s",WIFI_CMD_PREFIX,commandInfo[CMD_QUERY_DHCP].szCli);
		break;
	case CMD_SET_LOCAL_INFO:
		wifiInfo.networkInfoData.networkStatus = STATUS_SET_LOCAL_INFO;
		len = rt_sprintf((char *)cmd,"%s%s=\"%s\",\"%s\",\"%s\"",WIFI_CMD_PREFIX,commandInfo[CMD_SET_LOCAL_INFO].szCli,GetLocalIP(),GetGatewayIP(),GetSubnetMask());
		break;
	case CMD_QUERY_LOCAL_INFO:
		wifiInfo.responseLen = rt_strlen(commandInfo[CMD_GET_LOCAL_INFO].szCli);
		wifiInfo.networkInfoData.networkStatus = STATUS_QUERY_LOCAL_INFO;
		len = rt_sprintf((char *)cmd,"%s%s",WIFI_CMD_PREFIX,commandInfo[CMD_QUERY_LOCAL_INFO].szCli);
		break;
	case CMD_CONNECT_AP:
		rt_kprintf("Try to Connect AP \r\n");
		wifiInfo.responseLen = rt_strlen(commandInfo[CMD_GET_AP_INFO].szCli);
		wifiInfo.networkInfoData.networkStatus = STATUS_CONNECT_WIFI;
		len = rt_sprintf((char *)cmd,"%s%s=\"%s\",\"%s\"",WIFI_CMD_PREFIX,commandInfo[CMD_CONNECT_AP].szCli,wifiInfo.ssid,wifiInfo.password);
		break;
	case CMD_QUERY_AP_INFO:
		wifiInfo.networkInfoData.networkStatus = STATUS_QUERY_AP_INFO;
		len = rt_sprintf((char *)cmd,"%s%s",WIFI_CMD_PREFIX,commandInfo[CMD_QUERY_AP_INFO].szCli);
		break;
	case CMD_DISCONNECT_AP:
		len = rt_sprintf((char *)cmd,"%s%s",WIFI_CMD_PREFIX,commandInfo[CMD_DISCONNECT_AP].szCli);
		break;
	case CMD_TCP_CONNECT:
		wifiInfo.networkInfoData.networkStatus = STATUS_TCP_CONNECT;
		// "TCP","remote IP","remote Port"
		if(DISABLE == wifiInfo.domainConfig)
		{
			len = rt_sprintf((char *)cmd,"%s%s=\"%s\",\"%s\",%d",WIFI_CMD_PREFIX,commandInfo[CMD_TCP_CONNECT].szCli,TCP,wifiInfo.remoteIp,wifiInfo.remotePort);
		}
		else
		{
			len = rt_sprintf((char *)cmd,"%s%s=\"%s\",\"%s\",%d",WIFI_CMD_PREFIX,commandInfo[CMD_TCP_CONNECT].szCli,TCP,wifiInfo.domain,wifiInfo.remotePort);
		}
		break;
	case CMD_QUERY_MAC_INFO:
		wifiInfo.networkInfoData.networkStatus = STATUS_QUERY_MAC_INFO;
		wifiInfo.responseLen = rt_strlen(commandInfo[CMD_QUERY_MAC_INFO].szCli);
		len = rt_sprintf((char *)cmd,"%s%s",WIFI_CMD_PREFIX,commandInfo[CMD_QUERY_MAC_INFO].szCli);
		break;
	case CMD_SET_MAC_INFO:
		wifiInfo.networkInfoData.networkStatus = STATUS_SET_MAC_INFO;
		wifiInfo.responseLen = rt_strlen(commandInfo[CMD_SET_MAC_INFO].szCli);
		len = rt_sprintf((char *)cmd,"%s%s=\"%s\"",WIFI_CMD_PREFIX,commandInfo[CMD_SET_MAC_INFO].szCli,GetMacAddress());
		break;
	case CMD_TCP_DISCONNECT: //Tcp disconnection
		wifiInfo.networkInfoData.networkStatus = STATUS_TCP_DISCONNECT;
		wifiInfo.responseLen = rt_strlen(commandInfo[CMD_TCP_DISCONNECT].szCli);
		len = rt_sprintf((char *)cmd,"%s%s",WIFI_CMD_PREFIX,commandInfo[CMD_TCP_DISCONNECT].szCli);
		break;
	default:
		rt_kprintf("Invalid Id\r\n");
		break;
	}
	cmd[len++] = CR;
	cmd[len++] = LF;
	rt_memcpy(wifiInfo.txBuf,cmd,len);

	wifiInfo.txLen = len;//set length of transmitting

	return rt_event_send(wifiInfo.rx_event, SMSG_TX_DATA);
}

rt_err_t TcpSetDataLength(rt_uint16_t length)
{
	wifiInfo.networkInfoData.networkStatus = STATUS_SET_SEND_DATA_LENGTH;
	rt_uint8_t cmd[CMD_MAX_LEN];
	rt_uint8_t len = 0;
	//rt_kprintf("Set Data Length: %d \r\n", length);
	len = rt_sprintf((char *)cmd,"%s%s=%d",WIFI_CMD_PREFIX,commandInfo[CMD_SET_SEND_DATA_LENGTH].szCli,length);

	cmd[len++] = CR;
	cmd[len++] = LF;

	wifiInfo.txLen = len;

	rt_memcpy(wifiInfo.txBuf,cmd,len);

	return rt_event_send(wifiInfo.rx_event, SMSG_TX_DATA);
}

rt_err_t TcpSendData(rt_uint8_t *pData, rt_uint16_t length)
{
	//rt_kprintf("Start Send Data\r\n");
	rt_timer_start(wifiInfo.sendTimeoutTimer);
	rt_memcpy(wifiInfo.txBuf,pData,length);
	wifiInfo.txBuf[length++] = LF;
	wifiInfo.networkInfoData.networkStatus = STATUS_SEND_DATA;

	wifiInfo.txLen = length;

	return rt_event_send(wifiInfo.rx_event, SMSG_TX_DATA);
}

rt_uint8_t *FindValue( rt_uint8_t *pCmd)
{
	rt_uint8_t *value = RT_NULL;

	if(RT_NULL != pCmd)
	{
		rt_uint8_t *pos = pCmd;
		rt_uint8_t len = rt_strlen((char *)pos);

		while(len-- && ':' != *pos)
		{
			pos++;
		}

		if(':' == *pos)
		{
			value = ++pos;
		}
		else
		{
			value = (pCmd+(rt_strlen((char *)pCmd)));
		}

		pos = (pCmd+(rt_strlen((char *)pCmd)));
		while(value != pos && (CR == *pos || LF == *pos)) *pos-- = '\0';
	}

	return value;
}

rt_size_t ParserWifiData(rt_uint8_t *p_base, rt_size_t rx_size, NetworkInfoData *pNetworkInfoData)
{
	rt_size_t consumed = 0;

	if(RT_NULL != p_base && 0 < rx_size)
	{
		rt_uint8_t *begin=RT_NULL, *end=p_base, *last=(p_base+rx_size);

		do {
			if( (('+' == *end) || ('O' == *end) || ('E' == *end) || ('S' == *end) || ( '>' == *end)) && (RT_NULL == begin))
			{
				begin = end;
			}
			else if('>' == *begin && CR == (*end) && (STATUS_SET_SEND_DATA_LENGTH ==  wifiInfo.networkInfoData.networkStatus) )
			{  //treat the response of set sending length
				pNetworkInfoData->response = RESPONSE_WIFI_OK;
				wifiInfo.networkInfoData.networkStatus = STATUS_READY_TO_SEND_DATA;
			}
			else if( RT_NULL != begin && LF ==  *(end) && CR == *(end-1))
			{
				if( 0 == rt_strncmp(WIFI_OK,(char *)begin,rt_strlen(WIFI_OK)))
				{
					pNetworkInfoData->response = RESPONSE_WIFI_OK;
				}
				else if( 0 == rt_strncmp(WIFI_ERROR,(char *)begin,rt_strlen(WIFI_ERROR)))
				{
					pNetworkInfoData->response = RESPONSE_WIFI_ERROR;
				}
				else if( 0 == rt_strncmp(WIFI_FAIL,(char *)begin,rt_strlen(WIFI_ERROR)))
				{
					pNetworkInfoData->response = RESPONSE_WIFI_FAIL;
				}
				else if( 0 == rt_strncmp(SEND_DATA_OK,(char *)begin,rt_strlen(SEND_DATA_OK)))
				{
					pNetworkInfoData->response = RESPONSE_WIFI_OK;
				}
				else if(0 == rt_strncmp(SEND_DATA_FAIL,(char *)begin,rt_strlen(SEND_DATA_FAIL)))
				{
					pNetworkInfoData->response = RESPONSE_WIFI_FAIL;
				}
				else if( '+' == *begin++)// skip '+'
				{
					rt_int8_t id = CMD_MAX;
					rt_uint8_t szCmd[CMD_MAX_LEN];
					rt_uint8_t len = (rt_size_t)(end-begin);
					rt_uint8_t keepResponseLen  = wifiInfo.responseLen;

					rt_memcpy(szCmd,begin,len);

					while(id--)
					{
                        //treat the wifi module response
						if(CMD_RECEIVE_DATA == id)
						{
							wifiInfo.responseLen = 3;
						}
						else
						{
							wifiInfo.responseLen = keepResponseLen;
						}
						if( 0 == rt_strncmp((char *)szCmd,(char *)commandInfo[id].szCli,wifiInfo.responseLen))	break;
					}

					if(id < 0)
					{
						rt_kprintf("Invalid Command.\r\n");
						rt_kprintf("%s\r\n",szCmd);
					}
					else
					{
						*(begin+len) = '\0';
						rt_uint8_t *value = FindValue(begin);
#if 0
						if(CMD_MAX > commandInfo[id].id)
						{
							rt_kprintf("%s", commandInfo[id].szComment);
						}
#endif

						switch(commandInfo[id].id)
						{
						case CMD_GET_MODE:
							wifiInfo.networkInfoData.networkStatus = STATUS_GET_WIFI_MODE;
							pNetworkInfoData->data[0]=value[0];
							//rt_kprintf("%s", wifiModeInfo[(value[0]-'0')].Comment);
							break;
						case CMD_GET_DHCP:
							wifiInfo.networkInfoData.networkStatus = STATUS_GET_DHCP_INFO;
							rt_memcpy(&pNetworkInfoData->data,value,rt_strlen((char *)pNetworkInfoData->data));
							rt_uint8_t result = 0x01 & (atoi((char *)value)>>1);
							pNetworkInfoData->data[DHCP_MODE_INFO] = result;
							pNetworkInfoData->data[DHCP_COMPARE_RESULT] = (result == GetDhcpMode());
							//rt_kprintf("%s", dhchInfo[result].Comment);
							break;
						case CMD_GET_AP_INFO:
							rt_memcpy(&pNetworkInfoData->data,value,rt_strlen((char *)pNetworkInfoData->data));
							switch(wifiInfo.networkInfoData.networkStatus)
							{
							case STATUS_CONNECT_WIFI:
								//pNetworkInfoData->response = RESPONSE_WIFI_FAIL;
								//rt_kprintf("%s", wifiErrorInfo[(value[0]-'0')].Comment);
								break;
							case STATUS_QUERY_AP_INFO:
								//rt_kprintf("AP Info: %s",value);
								pNetworkInfoData->data[DHCP_MODE_INFO] =  GetDhcpMode();
								wifiInfo.networkInfoData.networkStatus = STATUS_GET_AP_INFO;
								break;
							default:
								break;
							}
							break;
						case CMD_GET_LOCAL_INFO:
							{
								rt_memcpy(&pNetworkInfoData->data,value,rt_strlen((char *)pNetworkInfoData->data));
								if( 0 == rt_strncmp(IP,(char *)value,sizeof(IP)) && (0 != rt_strncmp((char *)pNetworkInfoData->data,(char *)GetLocalIP(),rt_strlen((char *)GetLocalIP()))) )
								{
									rt_kprintf("Local IP is different\r\n");
									pNetworkInfoData->data[LOCAL_IP_INDEX] = CHANGE_LOCAL_INFO;
									pNetworkInfoData->data[GATEWAY_IP_INDEX] = '\0';
								}
								else if(0 == rt_strncmp(GATEWAY,(char *)value,sizeof(GATEWAY)) && (0 != rt_strncmp((char *)pNetworkInfoData->data,(char *)GetGatewayIP(),rt_strlen((char *)GetGatewayIP()))))
								{
									rt_kprintf("Gateway IP is different\r\n");
									pNetworkInfoData->data[GATEWAY_IP_INDEX] = CHANGE_LOCAL_INFO;
									pNetworkInfoData->data[NETMASK_IP_INDEX] = '\0';
								}
								else if(0 == rt_strncmp(NETMASK,(char *)value,sizeof(NETMASK)) && (0 != rt_strncmp((char *)pNetworkInfoData->data,(char *)GetSubnetMask(),rt_strlen((char *)GetSubnetMask()))))
								{
									rt_kprintf("Subnetmask IP is different\r\n");
									pNetworkInfoData->data[NETMASK_IP_INDEX] = CHANGE_LOCAL_INFO;
									pNetworkInfoData->data[MAX_INDEX] = '\0';
								}
							}
							wifiInfo.networkInfoData.networkStatus = STATUS_GET_LOCAL_INFO;
							break;
						case CMD_GET_MAC_INFO:
							rt_memcpy(&pNetworkInfoData->data,value,rt_strlen((char *)pNetworkInfoData->data));
							wifiInfo.networkInfoData.networkStatus = STATUS_GET_MAC_INFO;
							break;
						case CMD_RECEIVE_DATA:
							{
								rt_timer_stop(wifiInfo.sendTimeoutTimer);
								uint8_t dataStatus = STATUS_NONE;
								if(0 == rt_strncmp((char *)value,WIFI_OK,rt_strlen(WIFI_OK)))
								{
									pNetworkInfoData->response = RESPONSE_WIFI_OK;
									dataStatus = STATUS_RECEIVE_DATA;
								}
								else if(0 == rt_strncmp((char *)value,TCP_ERROR,rt_strlen(TCP_ERROR)))
								{
									pNetworkInfoData->response = RESPONSE_WIFI_ERROR;
									dataStatus = STATUS_REFUSE_DATA;
								}
								wifiInfo.networkInfoData.networkStatus = dataStatus;
							}
							break;
						default:
							break;
						}
					}
				}
				consumed = (rt_size_t)(end-p_base);
			}
		} while(++end <= last);
	}

	return consumed;
}

void WifiRxThread(void *params)
{
	WifiInfo *p_handle = (WifiInfo *)params;
	rt_uint8_t *pBuf = p_handle->rxBuf;
	rt_size_t uRemain = 0;
	rt_uint32_t events = 0;

	struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
	rt_err_t err;

	config.baud_rate = WIFI_BAUD_RATE;
	err = rt_device_control(p_handle->uport, RT_DEVICE_CTRL_CONFIG, (void *)&config);
	RT_ASSERT(err == RT_EOK);

	err = rt_device_open(p_handle->uport, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_INT_TX);//uart tx interrupt
	RT_ASSERT(err == RT_EOK);

	err = rt_device_set_rx_indicate(p_handle->uport, WifiRxIndicate);
	RT_ASSERT(err == RT_EOK);

	while(1)
	{
		if(RT_EOK == rt_event_recv(p_handle->rx_event, (SMSG_RX_DATA|SMSG_TX_DATA), (RT_EVENT_FLAG_OR|RT_EVENT_FLAG_CLEAR), RT_WAITING_FOREVER, &events))
		{
			if(SMSG_RX_DATA & events)
			{
				rt_size_t ulBytes;

				if(WIFI_RX_BUFF <= uRemain)
				{ // buffer clear
					uRemain = 0;
				}

				do {
					if((ulBytes=rt_device_read(p_handle->uport, 0, (pBuf+uRemain), (WIFI_RX_BUFF-uRemain))) > 0)
					{
						rt_size_t consumed = ParserWifiData(pBuf, (uRemain += ulBytes), &p_handle->networkInfoData);

						if(consumed > 0 && ((uRemain -= consumed) > 0))
						{
							rt_memmove(pBuf, pBuf+consumed, uRemain);
						}

					}
				} while(0 < ulBytes);

				if(RESPONSE_WIFI_RESPONSE_MAX > p_handle->networkInfoData.response)
				{
					switch(p_handle->networkInfoData.response)
					{
					case RESPONSE_WIFI_OK:
						p_handle->mqData.messge = SMSG_WIFI_OK;
						break;
					case RESPONSE_WIFI_ERROR:
						p_handle->mqData.messge = SMSG_WIFI_ERROR;
						break;
					case RESPONSE_WIFI_FAIL:
						p_handle->mqData.messge = SMSG_WIFI_FAIL;
						break;
					default:
						break;
					}
					rt_timer_stop(p_handle->wifiTimeoutTimer);
					p_handle->networkInfoData.response = RESPONSE_WIFI_RESPONSE_MAX;
					rt_memcpy(p_handle->mqData.data,&p_handle->networkInfoData,p_handle->mqData.size=sizeof(p_handle->networkInfoData.data));
					NetworkMangerSendMessage(&p_handle->mqData);
				}
			}
			else if(SMSG_TX_DATA & events)
			{
				WifiTxData(p_handle->txBuf,p_handle->txLen);
				rt_timer_start(p_handle->wifiTimeoutTimer);
				p_handle->txLen = 0;
			}
		}
	}
}

void InitWifInformation(void)
{
	wifiInfo.networkInfoData.response = RESPONSE_WIFI_RESPONSE_MAX;
	wifiInfo.networkInfoData.networkStatus = STATUS_NONE;

	wifiInfo.responseLen = 0;
	wifiInfo.txLen = 0;

	wifiInfo.mode = 1; //Station
	wifiInfo.dhcp = GetDhcpMode();

	rt_uint8_t ssid[AP_MAX_SSID_LENGTH];
	rt_uint8_t password[AP_MAX_PASSWORD_LENGTH];
	rt_memcpy(ssid,GetApSSID(),AP_MAX_SSID_LENGTH);
	rt_memcpy(wifiInfo.ssid,MakeWifiFormat(ssid),AP_MAX_SSID_LENGTH);
	rt_memcpy(password,GetApPassword(),AP_MAX_PASSWORD_LENGTH);
	rt_memcpy(wifiInfo.password,MakeWifiFormat(password),AP_MAX_PASSWORD_LENGTH);

	rt_memcpy(wifiInfo.remoteIp,GetTcpIp(),sizeof(wifiInfo.remoteIp));
	rt_memcpy(wifiInfo.domain,GetDomainInfo(),sizeof(wifiInfo.domain));
	wifiInfo.remotePort = GetTcpPort();
	wifiInfo.domainConfig = GetDomainConfig();

	wifiInfo.uport = RT_NULL;
	wifiInfo.rx_event = RT_NULL;
	wifiInfo.wifiTimeoutTimer = RT_NULL;
}

void DeInitWifi(void)
{
	rt_kprintf("DeInitialize Wi-Fi Module.\r\n");

	rt_event_delete(wifiInfo.rx_event);
	rt_device_close(wifiInfo.uport);
}

rt_bool_t InitWifi(void)
{
	WifiInfo *h_data = &wifiInfo;
	rt_thread_t tidRx;

	InitWifInformation();

	h_data->uport =  rt_device_find(UART3_DEV_NAME);
	RT_ASSERT(RT_NULL != h_data->uport);

	h_data->rx_event = rt_event_create("WifiRx",RT_IPC_FLAG_FIFO);
	RT_ASSERT(RT_NULL != h_data->rx_event);

	h_data->wifiTimeoutTimer = rt_timer_create("wifiTimeoutTimer", WifiTimeoutTimer, (void *)h_data, rt_tick_from_millisecond(WIFI_RESP_TIMEOUT_DELAY), RT_TIMER_FLAG_ONE_SHOT);
	RT_ASSERT(RT_NULL != h_data->wifiTimeoutTimer);

	h_data->sendTimeoutTimer = rt_timer_create("sendTimeoutTimer", SendTimeoutTimer, (void *)h_data, rt_tick_from_millisecond(WIFI_SEND_TIMEOUT_DELAY), RT_TIMER_FLAG_ONE_SHOT);
	RT_ASSERT(RT_NULL != h_data->sendTimeoutTimer);

	tidRx = rt_thread_create("WifiRxThread", WifiRxThread, (void *)h_data, WIFI_RX_STACK_SZIE, RT_MAIN_THREAD_PRIORITY, 20);
	RT_ASSERT(RT_NULL != tidRx);

	/* thread start  */
	rt_err_t err = rt_thread_startup(tidRx);
	RT_ASSERT(RT_EOK == err);

	return RT_TRUE;
}
