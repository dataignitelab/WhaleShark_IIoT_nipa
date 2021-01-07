/*
 * configuration.h
 *
 *  Created on: 2020. 3. 31.
 *      Author: hsna
 */

#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#define AP_MAX_SSID_LENGTH	32
#define AP_MAX_PASSWORD_LENGTH	64
#define IP_MAX_LENGTH	16
#define MAC_LENGTH		18
#define MAX_DOMAIN_LENGTH	64

#pragma pack(push, 1)
typedef struct _NetworkdCofig{
	rt_uint8_t apSSID[AP_MAX_SSID_LENGTH];
	rt_uint8_t apPassword[AP_MAX_PASSWORD_LENGTH];

	rt_uint8_t destIp[IP_MAX_LENGTH];
	rt_uint16_t destPort;
	rt_uint8_t localIp[IP_MAX_LENGTH];
	rt_uint8_t gatewayIp[IP_MAX_LENGTH];
	rt_uint8_t dnsServer[IP_MAX_LENGTH];
	rt_uint8_t subnetMask[IP_MAX_LENGTH];
	rt_uint8_t macAddress[MAC_LENGTH];
	rt_uint8_t dhcpMode;
	rt_uint8_t domainOn;
	rt_uint8_t domainInfo[MAX_DOMAIN_LENGTH];
}NetworkdCofig;
typedef struct _config_tag {
	rt_uint8_t waterMark;
	rt_uint8_t	manufacture;

	NetworkdCofig	networkdCofig;
	rt_uint8_t	device[6]; //Device Info
} Config;
#pragma pack(pop)

void SetReset(void);
rt_uint8_t *GetLocalIP(void);
void SetLocalIP(rt_uint8_t *pData, rt_size_t dataSize);
rt_uint8_t *GetSubnetMask(void);
void SetSubnetMask(rt_uint8_t *pData, rt_size_t dataSize);
rt_uint8_t *GetGatewayIP(void);
void SetGatewayIP(rt_uint8_t *pData, rt_size_t dataSize);
rt_uint8_t *GetDnsServer(void);
void SetDnsServer(rt_uint8_t *pData, rt_size_t dataSize);
rt_uint16_t GetTcpPort(void);
void SetTcpPort(rt_uint16_t data);
rt_uint8_t *GetTcpIp(void);
void SetTcpIP(rt_uint8_t *pData, rt_size_t dataSize);
rt_uint8_t *GetApPassword(void);
void SetApPassword(rt_uint8_t *pData, rt_size_t dataSize);
rt_uint8_t *GetApSSID(void);
void SetApSSID(rt_uint8_t *pData, rt_size_t dataSize);
rt_uint8_t GetDhcpMode(void);
void SetDhcpMode(rt_uint8_t data);
rt_uint8_t *GetMacAddress(void);
rt_bool_t SetMacAddress(rt_uint8_t *pData, rt_size_t dataSize);
rt_uint8_t GetManufactureMode(void);
rt_bool_t SetManufacture(rt_uint8_t on);
rt_uint8_t *GetDeviceInfo(void);
void SetDeviceInfo(rt_uint8_t *pData, rt_size_t dataSize);
rt_uint8_t GetDomainConfig(void);
void SetDomainConfig(rt_uint8_t domainOn);
rt_uint8_t *GetDomainInfo(void);
void SetDomainInfo(rt_uint8_t *pData, rt_size_t dataSize);
void ShowConfig(void);
rt_bool_t InitConfiguration(void);

#endif /* __CONFIGURATION_H__ */

