/*
 * wifi.c
 *
 */
#include <stdlib.h>
#include <string.h>

#include <rtthread.h>
#include <config/appconfig.h>
#include <config/configuration.h>
#include <common/smsgdef.h>
#include <network/networkmanager.h>

#include "wifi.h"

#define WIFI_MODULE_RESET	GET_PIN(B, 8)

#define WIFI_RX_BUFF	256
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
#define ENTER_THE_DATA	'>' //Enter The Data
#define TCP_OK			"OK"
#define TCP_ERROR		"ER"
#define RESPONSE		'+'
#define VALUE			":"
#define WIFI_BAUD_RATE	BAUD_RATE_115200
#define CMD_MAX_LEN		256
#define WIFI_MAX_LEN	128
#define WIFI_RESP_TIMEOUT_DELAY	10000
#define WIFI_SEND_TIMEOUT_DELAY	10000

// ID, COMMAND, COMMAND DESCRIPTION
static WIFICOMMANDINFO commandInfo[] = {
	{CMD_QUERY_CONNECTION_STATUS,"CIPSTATUS","[Query the connection status.]\r\n"},
	{CMD_GET_CONNECTION_STATUS,"STATUS:","[Get the connection status.]\r\n"},
	{CMD_RESTART,"RST","[Restart.]\r\n"},
	{CMD_ECHO_OFF, "ATE0", "[Sets Echo Off.]\r\n"},
	{CMD_SET_POWER,"RFPOWER","[Set RF Power]\r\n"},
	{CMD_DISABLE_SLEEP_MODE, "SLEEP", "[Sets Sleep Mode.]\r\n"},
	{CMD_SET_MODE, "CWMODE", "[Sets the default Wi-Fi mode (Station/AP/Station+AP).]\r\n"},
	{CMD_QUERY_MODE, "CWMODE?", "[Query the current Wi-Fi mode.]\r\n"},
	{CMD_GET_MODE, "CWMODE:", "[Get the current Wi-Fi mode.]\r\n"},
	{CMD_DELETE_IP_PORT_WITH_DATA, "CIPDINFO","[Delete IP/Port with +IPD.]\r\n"},
	{CMD_CONNECT_AP, "CWJAP", "[Connects to an AP. configuration saved in the flash.]\r\n"},
	{CMD_QUERY_AP_INFO, "CWJAP?", "[Query the connected AP Info.]\r\n"},
	{CMD_GET_AP_INFO, "CWJAP:", "[Get the connected AP Info.]\r\n"},
	{CMD_SET_DHCP,"CWDHCP","[Set DHCP Mode]\r\n"},
	{CMD_QUERY_DHCP,"CWDHCP?","[Query DHCP Mode]\r\n"},
	{CMD_GET_DHCP,"CWDHCP:","[Get DHCP Mode]\r\n"},
	{CMD_SET_LOCAL_INFO,"CIPSTA","[Set Local Info.]\r\n"},
	{CMD_QUERY_LOCAL_INFO,"CIPSTA?","[Query Local Info.]\r\n"},
	{CMD_GET_LOCAL_INFO,"CIPSTA:","[Get Local Info. (Local IP, Gateway, netmask)]\r\n"},
	{CMD_DISCONNECT_AP, "CWQAP", "[Disconnects from an AP.]\r\n"},
	{CMD_TCP_CONNECT, "CIPSTART", "[Single TCP Connection.]\r\n"},
	{CMD_SET_TRANSMISSION_MODE, "CIPMODE", "[Set Transmission mode.]\r\n"},
	{CMD_SET_SEND_DATA_LENGTH,"CIPSEND","[Set Data Length.]\r\n"},
	{CMD_QUERY_MAC_INFO,"CIPSTAMAC_DEF?","[Query Mac Address.]\r\n"},
	{CMD_GET_MAC_INFO,"CIPSTAMAC_DEF:","[Get Mac Address.]\r\n"},
	{CMD_SET_MAC_INFO,"CIPSTAMAC_DEF","[Set Mac Address.]\r\n"},
	{CMD_RECEIVE_DATA,"IPD","[Receive Data.]\r\n"},
	{CMD_TCP_DISCONNECT,"CIPCLOSE","[Close TCP Connection]\r\n"},
	{CMD_MQTT_USER_CFG,"MQTTUSERCFG","[Set and Query MQTT user configuration]\r\n"},
	{CMD_MQTT_CLIENT_ID,"MQTTCLIENTID:","[Set and Query MQTT client ID]\r\n"},
	{CMD_MQTT_USER_NAME,"MQTTUSERNAME","[Set and Query MQTT user-name]\r\n"},
	{CMD_MQTT_PASSWORD,"MQTTPASSWORD","[Set and Query MQTT password]\r\n"},
	{CMD_MQTT_CONNECT_CFG,"MQTTCONNCFG","[Set and Query configuration of MQTT Connection]\r\n"},
	{CMD_MQTT_QUERY_CONNECT_STATE,"MQTTCONN?","[Query the MQTT broker]\r\n"},
	{CMD_MQTT_GET_CONNECT_STATE,"MQTTCONN:","[Get Connection State]\r\n"},
	{CMD_MQTT_CONNECT,"MQTTCONN","[Connect to MQTT Broker]\r\n"},
	{CMD_MQTT_CONNECTED,"MQTTCONNECTED","[Connected to MQTT Broker]\r\n"},
	{CMD_MQTT_PUB_STR,"MQTTPUB","[Publish MQTT Data in string format]\r\n"},
	{CMD_MQTT_PUB_BIN,"MQTTPUBRAW","[Publish MQTT message in binary-coded format]\r\n"},
	{CMD_MQTT_SUB,"MQTTSUB","[Subscribe MQTT Topic]\r\n"},
	{CMD_MQTT_UNSUB,"MQTTUNSUB","[Un-Subscribe MQTT Topic]\r\n"},
	{CMD_MQTT_RECEIVE_DATA,"MQTTSUBRECV","[Receive Data from MQTT]\r\n"},
	{CMD_MQTT_DISCONNECT,"MQTTCLEAN","[Close the MQTT Connection]\r\n"},
	{CMD_MQTT_DISCONNECTED,"MQTTDISCONNECTED","[Success Disconnecting MQTT]\r\n"}
};

typedef struct _WifiInfo
{
	MqData_t mqData;

	rt_device_t uport;
	rt_sem_t rxSem;
	rt_timer_t	wifiTimeoutTimer;
	NetworkInfoData networkInfoData;

	rt_uint8_t responseLen;

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

	//MQTT Info
	rt_uint8_t mqtt;
	rt_uint8_t mqttClientId[MQTT_CLIENT_ID_MAX];
	rt_uint8_t mqttUserName[MQTT_USER_NAME_MAX];
	rt_uint8_t mqttPassword[MQTT_PASSWORD_MAX];
	rt_uint16_t mqttKeepAlive;

	rt_uint8_t rxBuf[WIFI_RX_BUFF];
}WifiInfo;

WifiInfo wifiInfo;

void WifiTimeoutTimer(void *params)
{
	wifiInfo.mqData.messge = SMSG_WIFI_TIMEOUT;
    //Copy the Network Status
	rt_memcpy(wifiInfo.mqData.data, &wifiInfo.networkInfoData, wifiInfo.mqData.size=sizeof(wifiInfo.networkInfoData.data));
	NetworkMangerSendMessage(&wifiInfo.mqData);

	wifiInfo.networkInfoData.response = RESPONSE_ESP8266_MAX;
}

static rt_err_t WifiRxIndicate(rt_device_t dev, rt_size_t size)
{
	rt_sem_release(wifiInfo.rxSem);

	return RT_EOK;
}

static void WifiTxData(rt_uint8_t *data, rt_size_t tx_size)
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

	if(0 == remain)
	{
		rt_timer_start(wifiInfo.wifiTimeoutTimer);
	}
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

void SendWifiCommand(wifiCommandID id)
{
	rt_uint8_t cmd[CMD_MAX_LEN];
	rt_uint8_t len = 0;
	switch(id)
	{
	case CMD_QUERY_CONNECTION_STATUS:
		rt_kprintf("Request Connection Information\r\n");
		wifiInfo.networkInfoData.networkStatus = STATUS_QUERY_CONNECTION_STATUS; //set status
		len = rt_sprintf((char *)cmd,"%s%s",WIFI_CMD_PREFIX,commandInfo[CMD_QUERY_CONNECTION_STATUS].szCli);
		break;
	case CMD_RESTART:
		rt_kprintf("Try to Restart WiFi Module\r\n");
		wifiInfo.networkInfoData.networkStatus = STATUS_WIFI_RESTART; //set status
		len = rt_sprintf((char *)cmd,"%s%s",WIFI_CMD_PREFIX,commandInfo[CMD_RESTART].szCli);
		break;
	case CMD_ECHO_OFF: //uart echo off
		rt_kprintf("Set echo off\r\n");
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
	case CMD_DELETE_IP_PORT_WITH_DATA:
		wifiInfo.networkInfoData.networkStatus = STATUS_DELETE_IP_PORT_WITH_DATA;
		len = rt_sprintf((char *)cmd,"%s%s=0",WIFI_CMD_PREFIX,commandInfo[CMD_DELETE_IP_PORT_WITH_DATA].szCli);
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
		rt_kprintf("Try to Connect TCP \r\n");
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
	case CMD_SET_TRANSMISSION_MODE:
		wifiInfo.networkInfoData.networkStatus = STATUS_SET_TRANSMISSION_MODE;
		len = rt_sprintf((char *)cmd,"%s%s=0",WIFI_CMD_PREFIX,commandInfo[CMD_SET_TRANSMISSION_MODE].szCli);
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
		rt_kprintf("Try to disconnect TCP \r\n");
		wifiInfo.networkInfoData.networkStatus = STATUS_TCP_DISCONNECT;
		len = rt_sprintf((char *)cmd,"%s%s",WIFI_CMD_PREFIX,commandInfo[CMD_TCP_DISCONNECT].szCli);
		break;
	case CMD_MQTT_USER_CFG:
		rt_kprintf("%s", commandInfo[CMD_MQTT_USER_CFG].szComment);
		wifiInfo.networkInfoData.networkStatus = STATUS_MQTT_USER_CFG;
		len = rt_sprintf((char *)cmd,"%s%s=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"",WIFI_CMD_PREFIX,commandInfo[CMD_MQTT_USER_CFG].szCli,wifiInfo.mqttClientId,wifiInfo.mqttUserName,wifiInfo.mqttPassword);
		break;
	case CMD_MQTT_CLIENT_ID:
		rt_kprintf("%s", commandInfo[CMD_MQTT_CLIENT_ID].szComment);
		wifiInfo.networkInfoData.networkStatus = STATUS_MQTT_CLIENT_ID;
		len = rt_sprintf((char *)cmd,"%s%s=0,%s",WIFI_CMD_PREFIX,commandInfo[CMD_MQTT_CLIENT_ID].szCli,wifiInfo.mqttClientId);
		break;
	case CMD_MQTT_USER_NAME:
		rt_kprintf("%s", commandInfo[CMD_MQTT_USER_NAME].szComment);
		wifiInfo.networkInfoData.networkStatus = STATUS_MQTT_USER_NAME;
		len = rt_sprintf((char *)cmd,"%s%s=0,%s",WIFI_CMD_PREFIX,commandInfo[CMD_MQTT_USER_NAME].szCli,wifiInfo.mqttUserName);
		break;
	case CMD_MQTT_PASSWORD:
		rt_kprintf("%s", commandInfo[CMD_MQTT_PASSWORD].szComment);
		wifiInfo.networkInfoData.networkStatus = STATUS_MQTT_PASSWORD;
		len = rt_sprintf((char *)cmd,"%s%s=0,%s",WIFI_CMD_PREFIX,commandInfo[CMD_MQTT_PASSWORD].szCli,wifiInfo.mqttPassword);
		break;
	case CMD_MQTT_CONNECT_CFG:
		rt_kprintf("%s", commandInfo[CMD_MQTT_CONNECT_CFG].szComment);
		wifiInfo.networkInfoData.networkStatus = STATUS_MQTT_CONNECT_CFG;
		len = rt_sprintf((char *)cmd,"%s%s=0,\"%s\",1",WIFI_CMD_PREFIX,commandInfo[CMD_MQTT_CONNECT_CFG].szCli,wifiInfo.mqttKeepAlive);
		break;
	case CMD_MQTT_QUERY_CONNECT_STATE:
		rt_kprintf("%s", commandInfo[CMD_MQTT_QUERY_CONNECT_STATE].szComment);
		wifiInfo.networkInfoData.networkStatus = STATUS_MQTT_QUERY_CONNECT_STATE;
		wifiInfo.responseLen = rt_strlen(commandInfo[CMD_MQTT_GET_CONNECT_STATE].szCli);
		len = rt_sprintf((char *)cmd,"%s%s",WIFI_CMD_PREFIX,commandInfo[CMD_MQTT_QUERY_CONNECT_STATE].szCli);
		break;
	case CMD_MQTT_CONNECT:
		rt_kprintf("%s", commandInfo[CMD_MQTT_CONNECT].szComment);
		wifiInfo.networkInfoData.networkStatus = STATUS_MQTT_CONNECT;
		if(DISABLE == wifiInfo.domainConfig)
		{
			len = rt_sprintf((char *)cmd,"%s%s=0,\"%s\",%d,0",WIFI_CMD_PREFIX,commandInfo[CMD_MQTT_CONNECT].szCli,wifiInfo.remoteIp,wifiInfo.remotePort);
		}
		else
		{
			len = rt_sprintf((char *)cmd,"%s%s=0,\"%s\",%d,0",WIFI_CMD_PREFIX,commandInfo[CMD_MQTT_CONNECT].szCli,wifiInfo.domain,wifiInfo.remotePort);
		}
		break;
	case CMD_MQTT_DISCONNECT:
		rt_kprintf("%s", commandInfo[CMD_MQTT_DISCONNECT].szComment);
		wifiInfo.networkInfoData.networkStatus = STATUS_MQTT_DISCONNECT;
		len = rt_sprintf((char *)cmd,"%s%s=0",WIFI_CMD_PREFIX,commandInfo[CMD_MQTT_DISCONNECT].szCli);
		break;
	case CMD_MQTT_SUB:
		rt_kprintf("%s", commandInfo[CMD_MQTT_SUB].szComment);
		wifiInfo.networkInfoData.networkStatus = STATUS_MQTT_SUB;
		len = rt_sprintf((char *)cmd,"%s%s=0,\"%s\",1",WIFI_CMD_PREFIX,commandInfo[CMD_MQTT_SUB].szCli,GetDeviceInfo());
		rt_kprintf("%s \r\n",cmd);
		break;
	default:
		rt_kprintf("Invalid Id\r\n");
		break;
	}
	cmd[len++] = CR;
	cmd[len++] = LF;

	WifiTxData(cmd, len);
}

void NetworkSetDataLength(rt_uint16_t length)
{
	wifiInfo.networkInfoData.networkStatus = STATUS_SET_SEND_DATA_LENGTH;
	rt_uint8_t cmd[CMD_MAX_LEN>>2];
	rt_uint8_t len = 0;
    if(ENABLE == wifiInfo.mqtt)
	{
		len = rt_sprintf((char *)cmd,"%s%s=0,\"%s\",%d,1,0",WIFI_CMD_PREFIX,commandInfo[CMD_MQTT_PUB_BIN].szCli,GetDeviceInfo(),length);
	}
	else
	{
		len = rt_sprintf((char *)cmd,"%s%s=%d",WIFI_CMD_PREFIX,commandInfo[CMD_SET_SEND_DATA_LENGTH].szCli,length);
	}

	cmd[len++] = CR;
	cmd[len++] = LF;

	WifiTxData(cmd, len);
}

void NetworkSendData(rt_uint8_t *pData, rt_uint16_t length)
{
	rt_uint8_t txBuf[CMD_MAX_LEN<<1];
	rt_memcpy(txBuf,pData,length);
	wifiInfo.networkInfoData.networkStatus = STATUS_SEND_DATA;

	WifiTxData(txBuf, length);
}

rt_size_t ParserWifiData(rt_uint8_t *p_base, rt_size_t rx_size, NetworkInfoData *pNetworkInfoData)
{
	rt_size_t consumed = 0;

	rt_uint8_t *begin=RT_NULL;

	if(0 == rt_strncmp((char *)p_base, WIFI_OK, rt_strlen(WIFI_OK)))
	{
		pNetworkInfoData->response = RESPONSE_WIFI_OK;
	}
	else if(0 == rt_strncmp((char *)p_base, commandInfo[CMD_GET_CONNECTION_STATUS].szCli, rt_strlen(commandInfo[CMD_GET_CONNECTION_STATUS].szCli)))
	{
		pNetworkInfoData->data[0]=*(p_base+rt_strlen(commandInfo[CMD_GET_CONNECTION_STATUS].szCli));
		pNetworkInfoData->response = RESPONSE_GET_TCP_STATUS;
	}
	else if(0 == rt_strncmp((char *)p_base, SEND_DATA_OK, rt_strlen(SEND_DATA_OK)))
	{
		pNetworkInfoData->response = RESPONSE_TCP_SEND_OK;
	}
	else if(0 == rt_strncmp((char *)p_base, SEND_DATA_FAIL, rt_strlen(SEND_DATA_FAIL)))
	{
		pNetworkInfoData->response = RESPONSE_TCP_SEND_FAIL;
	}
	else if(0 == rt_strncmp((char *)p_base, WIFI_FAIL, rt_strlen(WIFI_FAIL)))
	{
		pNetworkInfoData->response = RESPONSE_WIFI_FAIL;
	}
	else if(0 == rt_strncmp((char *)p_base, WIFI_ERROR, rt_strlen(WIFI_ERROR)))
	{
		pNetworkInfoData->response = RESPONSE_WIFI_ERROR;
	}
	else if(RT_NULL != (begin = (rt_uint8_t *)strchr((char *)p_base, RESPONSE)))
	{
		begin++;//skip '+'
		rt_int8_t id = CMD_MAX;
		rt_uint8_t keepResponseLen  = wifiInfo.responseLen;

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
			if( 0 == rt_strncmp((char *)begin,(char *)commandInfo[id].szCli,wifiInfo.responseLen))	break;
		}

		if(id < 0)
		{
			rt_kprintf("Invalid Command\r\n");
			pNetworkInfoData->response = RESPONSE_WIFI_ERROR;
		}
		else
		{
			rt_uint8_t *value = RT_NULL;
			(rt_uint8_t *)strtok((char *)begin, VALUE);
			value = (rt_uint8_t *)strtok(RT_NULL,RT_NULL);
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
				break;
			case CMD_GET_DHCP:
				wifiInfo.networkInfoData.networkStatus = STATUS_GET_DHCP_INFO;
				rt_memcpy(&pNetworkInfoData->data,value,rt_strlen((char *)value));
				rt_uint8_t result = 0x01 & (atoi((char *)value)>>1);
				pNetworkInfoData->data[DHCP_MODE_INFO] = result;
				pNetworkInfoData->data[DHCP_COMPARE_RESULT] = (result == GetDhcpMode());
				break;
			case CMD_GET_AP_INFO:
				rt_memcpy(&pNetworkInfoData->data,value,rt_strlen((char *)value));
				switch(wifiInfo.networkInfoData.networkStatus)
				{
				case STATUS_CONNECT_WIFI:
					pNetworkInfoData->response = RESPONSE_WIFI_FAIL;
					break;
				case STATUS_QUERY_AP_INFO:
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
				rt_memcpy(&pNetworkInfoData->data,value,rt_strlen((char *)value));
				wifiInfo.networkInfoData.networkStatus = STATUS_GET_MAC_INFO;
				break;
			case CMD_MQTT_RECEIVE_DATA:
				pNetworkInfoData->response = RESPONSE_RECEIVE_ACK;
				break;
			case CMD_RECEIVE_DATA:
				pNetworkInfoData->response = RESPONSE_RECEIVE_ACK;
				uint8_t dataStatus = STATUS_NONE;
				if(0 == rt_strncmp((char *)value,TCP_OK,rt_strlen(TCP_OK)))
				{
					dataStatus = STATUS_RECEIVE_DATA;
				}
				else if(0 == rt_strncmp((char *)value,TCP_ERROR,rt_strlen(TCP_ERROR)))
				{
					dataStatus = STATUS_REFUSE_DATA;
				}
				wifiInfo.networkInfoData.networkStatus = dataStatus;
				break;
			case CMD_TCP_DISCONNECT:
				break;
			case CMD_MQTT_DISCONNECTED:
			case CMD_MQTT_CONNECTED:
				pNetworkInfoData->response = RESPONSE_WIFI_OK;
				break;
			case CMD_MQTT_GET_CONNECT_STATE:
				rt_memcpy(&pNetworkInfoData->data, value, rt_strlen((char *)value));
				wifiInfo.networkInfoData.networkStatus = STATUS_MQTT_GET_CONNECT_STATE;
				pNetworkInfoData->response = RESPONSE_WIFI_OK;
				break;
			default:
				rt_kprintf("Command ID: %d\r\n", commandInfo[id].id);
				break;
			}
		}
	}

	return consumed;
}

void WifiRxThread(void *params)
{
	WifiInfo *p_handle = (WifiInfo *)params;
	rt_uint8_t *pBuf = p_handle->rxBuf;
	rt_size_t uRemain = 0;
	rt_uint8_t data;

	struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
	rt_err_t err;

	config.baud_rate = WIFI_BAUD_RATE;
	err = rt_device_control(p_handle->uport, RT_DEVICE_CTRL_CONFIG, (void *)&config);
	RT_ASSERT(err == RT_EOK);

	err = rt_device_open(p_handle->uport, RT_DEVICE_OFLAG_RDWR| RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_INT_TX);//uart tx interrupt
	RT_ASSERT(err == RT_EOK);

	err = rt_device_set_rx_indicate(p_handle->uport, WifiRxIndicate);
	RT_ASSERT(err == RT_EOK);

	while(1)
	{
		while(rt_device_read(p_handle->uport, -1, &data, 1) != 1)
		{
			if(WIFI_RX_BUFF <= uRemain)
			{ // buffer clear
				uRemain = 0;
			}
			pBuf[uRemain] = data;
			uRemain += 1;

			if(ENTER_THE_DATA == data)
			{
				p_handle->networkInfoData.response = RESPONSE_READY_TO_SEND;
			}
			else if(LF == data)
			{
				ParserWifiData(pBuf, uRemain, &p_handle->networkInfoData);
				rt_memset(pBuf,0,WIFI_RX_BUFF);
				uRemain = 0;
			}

			if(RESPONSE_ESP8266_MAX > p_handle->networkInfoData.response)
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
				case RESPONSE_READY_TO_SEND:
					p_handle->mqData.messge = SMSG_READY_TO_SEND;
					break;
				case RESPONSE_MQTT_SEND_OK:
				case RESPONSE_TCP_SEND_OK:
					p_handle->mqData.messge = SMSG_SEND_OK;
					break;
				case RESPONSE_MQTT_SEND_FAIL:
				case RESPONSE_TCP_SEND_FAIL:
					p_handle->mqData.messge = SMSG_SEND_FAIL;
					break;
				case RESPONSE_GET_TCP_STATUS:
					p_handle->mqData.messge = SMSG_GET_TCP_STATUS;
					break;
				case RESPONSE_RECEIVE_ACK:
					p_handle->mqData.messge = SMSG_RECEIVE_ACK;
					break;
				default:
					break;
				}
				rt_timer_stop(p_handle->wifiTimeoutTimer);
				p_handle->networkInfoData.response = RESPONSE_ESP8266_MAX;
				rt_memcpy(p_handle->mqData.data,&p_handle->networkInfoData,p_handle->mqData.size=sizeof(p_handle->networkInfoData.data));
				NetworkMangerSendMessage(&p_handle->mqData);
			}
			rt_sem_take(p_handle->rxSem, RT_WAITING_FOREVER);
		}
	}
}

void WifiHardwareReset(void)
{
	rt_pin_mode(WIFI_MODULE_RESET, PIN_MODE_OUTPUT);
	//WiFi Module Reset
	rt_pin_write(WIFI_MODULE_RESET, DISABLE);
	rt_thread_delay(1000);
	rt_pin_write(WIFI_MODULE_RESET, ENABLE);
}

void InitWifInformation(void)
{
	wifiInfo.networkInfoData.response = RESPONSE_ESP8266_MAX;
	wifiInfo.networkInfoData.networkStatus = STATUS_NONE;

	wifiInfo.responseLen = 0;

	wifiInfo.mode = 1; //Station
	wifiInfo.dhcp = GetDhcpMode();
	wifiInfo.mqtt = GetMqttOn();

	rt_uint8_t ssid[AP_MAX_SSID_LENGTH];
	rt_uint8_t password[AP_MAX_PASSWORD_LENGTH];
	rt_memcpy(ssid,GetApSSID(),AP_MAX_SSID_LENGTH);
	rt_memcpy(wifiInfo.ssid,MakeWifiFormat(ssid),AP_MAX_SSID_LENGTH);
	rt_memcpy(password,GetApPassword(),AP_MAX_PASSWORD_LENGTH);
	rt_memcpy(wifiInfo.password,MakeWifiFormat(password),AP_MAX_PASSWORD_LENGTH);

	rt_memcpy(wifiInfo.remoteIp,GetTcpIp(),IP_MAX_LENGTH);
	rt_memcpy(wifiInfo.domain,GetDomainInfo(),WIFI_MAX_LEN);
	wifiInfo.remotePort = GetNetworkPort();
	wifiInfo.domainConfig = GetDomainConfig();

	rt_memcpy(wifiInfo.mqttClientId,GetMqttClientId(),MQTT_CLIENT_ID_MAX);
	rt_memcpy(wifiInfo.mqttUserName,GetMqttUserName(),MQTT_USER_NAME_MAX);
	rt_memcpy(wifiInfo.mqttPassword,GetMqttPassword(),MQTT_PASSWORD_MAX);
	wifiInfo.mqttKeepAlive = GetMqttKeepAlive();

	wifiInfo.uport = RT_NULL;
	wifiInfo.wifiTimeoutTimer = RT_NULL;
}

void DeInitWifi(void)
{
	rt_kprintf("DeInitialize Wi-Fi Module.\r\n");

	rt_timer_stop(wifiInfo.wifiTimeoutTimer);

	if(RT_EOK != rt_device_close(wifiInfo.uport))
	{
		rt_kprintf("Closing WiFi Uart Device Failed.\r\n");
	}
}

rt_bool_t InitWifi(void)
{
	WifiInfo *h_data = &wifiInfo;
	rt_thread_t tidRx;

	InitWifInformation();

	h_data->uport =  rt_device_find(UART3_DEV_NAME);
	RT_ASSERT(RT_NULL != h_data->uport);

	h_data->rxSem = rt_sem_create("rxSem", 0, RT_IPC_FLAG_FIFO);

	h_data->wifiTimeoutTimer = rt_timer_create("wifiTimeoutTimer", WifiTimeoutTimer, RT_NULL, rt_tick_from_millisecond(WIFI_RESP_TIMEOUT_DELAY), RT_TIMER_FLAG_ONE_SHOT);
	RT_ASSERT(RT_NULL != h_data->wifiTimeoutTimer);

	tidRx = rt_thread_create("WifiRxThread", WifiRxThread, (void *)h_data, WIFI_RX_STACK_SZIE, RT_MAIN_THREAD_PRIORITY, 20);
	RT_ASSERT(RT_NULL != tidRx);

	/* thread start  */
	rt_thread_startup(tidRx);

	return RT_TRUE;
}
