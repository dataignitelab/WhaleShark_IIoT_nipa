/*
 * wifi.h
 *
 */

#ifndef NETWORK_WIFI_WIFI_H_
#define NETWORK_WIFI_WIFI_H_

#define CHANGE_LOCAL_INFO	0xBF
#define DHCP_MODE_INFO		0
#define DHCP_COMPARE_RESULT	1

typedef enum
{
	LOCAL_IP_INDEX = 16,
	GATEWAY_IP_INDEX,
	NETMASK_IP_INDEX,
	MAX_INDEX,
}LOCALINFO_INDEX;

typedef enum
{
	RESPONSE_WIFI_OK,
	RESPONSE_WIFI_ERROR,
	RESPONSE_WIFI_FAIL,
	RESPONSE_WIFI_RESPONSE_MAX
}WIFI_RESPONSE;

typedef enum
{
	//Wifi Mode
	NullMode = '0',
	StationMode,
	SoftApMode,
	SoftAp_StationMode,
	ModeMax,
	//Connection Error Code
	ConnectionTimeout = '1',
	WrongPassword,
	NotFindTargetAP,
	ConnectionFailed,
	EroorMax
}WIFI_STATUS;

typedef enum _WifiCommandID
{
	CMD_RESTART,
	CMD_ECHO_OFF,
	CMD_SET_POWER,
	CMD_DISABLE_SLEEP_MODE,
	CMD_SET_MODE,
	CMD_QUERY_MODE,
	CMD_GET_MODE,
	CMD_CONNECT_AP,
	CMD_QUERY_AP_INFO,
	CMD_GET_AP_INFO,
	CMD_SET_DHCP,
	CMD_QUERY_DHCP,
	CMD_GET_DHCP,
	CMD_SET_LOCAL_INFO,
	CMD_QUERY_LOCAL_INFO,
	CMD_GET_LOCAL_INFO,
	CMD_DISCONNECT_AP,
	CMD_TCP_CONNECT,
	CMD_SET_SEND_DATA_LENGTH,
	CMD_QUERY_MAC_INFO,
	CMD_GET_MAC_INFO,
	CMD_SET_MAC_INFO,
	CMD_RECEIVE_DATA,
	CMD_TCP_DISCONNECT,

	CMD_MAX
} wifiCommandID;

typedef struct _WIFICOMMANDINFO{
	wifiCommandID	id;
	char	szCli[16];
	char	szComment[64];
} WIFICOMMANDINFO;

void DeInitWifi(void);
rt_bool_t InitWifi(void);
rt_err_t SendWifiCommand(wifiCommandID id);
void ConnectWifi(void);
rt_err_t TcpSetDataLength(rt_uint16_t length);
rt_err_t TcpSendData(rt_uint8_t *pData, rt_uint16_t length);

#endif /* NETWORK_WIFI_WIFI_H_ */
