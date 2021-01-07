/*
 * configuration.c
 *
 *  Created on: 2020. 3. 31.
 *      Author: hsna
 */
#include <string.h>

#include "appconfig.h"
#include "configuration.h"
#include <device/flash/flash.h>

typedef struct _config_info {
	Config cfg;
} ConfigInfo;

static ConfigInfo configInfo;

void SetReset(void)
{
	configInfo.cfg.waterMark = 0xFF;

	FlashWrite((rt_uint8_t *)&configInfo.cfg, sizeof(configInfo.cfg));
}

rt_uint8_t *GetLocalIP(void)
{
	return configInfo.cfg.networkdCofig.localIp;
}

void SetLocalIP(rt_uint8_t *pData, rt_size_t dataSize)
{
	Config *pCfg = &configInfo.cfg;

	if(0 != rt_strncmp((char *)pCfg->networkdCofig.localIp, (char *)pData,dataSize))
	{
		rt_memset(pCfg->networkdCofig.localIp,0,IP_MAX_LENGTH);
		rt_memcpy(pCfg->networkdCofig.localIp,pData,dataSize);
		rt_kprintf("Set Local IP: %s\r\n", pCfg->networkdCofig.localIp);
		FlashWrite((rt_uint8_t *)pCfg, sizeof(Config));
	}
}

rt_uint8_t *GetSubnetMask(void)
{
	return configInfo.cfg.networkdCofig.subnetMask;
}

void SetSubnetMask(rt_uint8_t *pData, rt_size_t dataSize)
{
	Config *pCfg = &configInfo.cfg;

	if(0 != rt_strncmp((char *)pCfg->networkdCofig.subnetMask, (char *)pData, dataSize))
	{
		rt_memset(pCfg->networkdCofig.subnetMask,0,IP_MAX_LENGTH);
		rt_memcpy(pCfg->networkdCofig.subnetMask,pData,dataSize);
		rt_kprintf("Set Subnetmask: %s\r\n", pCfg->networkdCofig.subnetMask);
		FlashWrite((rt_uint8_t *)pCfg, sizeof(Config));
	}
}

rt_uint8_t *GetGatewayIP(void)
{
	return configInfo.cfg.networkdCofig.gatewayIp;
}

void SetGatewayIP(rt_uint8_t *pData, rt_size_t dataSize)
{
	Config *pCfg = &configInfo.cfg;

	if(0 != rt_strncmp((char *)pCfg->networkdCofig.gatewayIp, (char *)pData, dataSize))
	{
		rt_memset(pCfg->networkdCofig.gatewayIp,0,IP_MAX_LENGTH);
		rt_memcpy(pCfg->networkdCofig.gatewayIp,pData,dataSize);
		rt_kprintf("Set GateWay IP: %s\r\n", pCfg->networkdCofig.gatewayIp);
		FlashWrite((rt_uint8_t *)pCfg, sizeof(Config));
	}
}

rt_uint8_t *GetDnsServer(void)
{
	return configInfo.cfg.networkdCofig.dnsServer;
}

void SetDnsServer(rt_uint8_t *pData, rt_size_t dataSize)
{
	Config *pCfg = &configInfo.cfg;

	if(0 != rt_strncmp((char *)pCfg->networkdCofig.dnsServer, (char *)pData, dataSize))
	{
		rt_memset(pCfg->networkdCofig.dnsServer,0,IP_MAX_LENGTH);
		rt_memcpy(pCfg->networkdCofig.dnsServer,pData,dataSize);
		rt_kprintf("Set DNS Server: %s\r\n", pCfg->networkdCofig.dnsServer);
		FlashWrite((rt_uint8_t *)pCfg, sizeof(Config));
	}
}

rt_uint16_t GetTcpPort(void)
{
	return configInfo.cfg.networkdCofig.destPort;
}

void SetTcpPort(rt_uint16_t data)
{
	Config *pCfg = &configInfo.cfg;

	if(pCfg->networkdCofig.destPort != data)
	{
		pCfg->networkdCofig.destPort = data;
		rt_kprintf("Set Destination Port: %d\r\n", pCfg->networkdCofig.destPort);
		FlashWrite((rt_uint8_t *)pCfg, sizeof(Config));
	}
}

rt_uint8_t *GetTcpIp(void)
{
	return configInfo.cfg.networkdCofig.destIp;
}

void SetTcpIP(rt_uint8_t *pData, rt_size_t dataSize)
{
	Config *pCfg = &configInfo.cfg;

	if(0 != rt_strncmp((char *)pCfg->networkdCofig.destIp, (char *)pData, dataSize))
	{
		rt_memset(pCfg->networkdCofig.destIp,0,IP_MAX_LENGTH);
		rt_memcpy(pCfg->networkdCofig.destIp,pData,dataSize);
		rt_kprintf("Set Destination IP: %s\r\n", pCfg->networkdCofig.destIp);
		FlashWrite((rt_uint8_t *)pCfg, sizeof(Config));
	}
}

rt_uint8_t *GetApPassword(void)
{
	return configInfo.cfg.networkdCofig.apPassword;
}

void SetApPassword(rt_uint8_t *pData, rt_size_t dataSize)
{
	Config *pCfg = &configInfo.cfg;

	if(0 != rt_strncmp((char *)pCfg->networkdCofig.apPassword, (char *)pData, dataSize))
	{
		rt_memset(pCfg->networkdCofig.apPassword,0,AP_MAX_PASSWORD_LENGTH);
		rt_memcpy(pCfg->networkdCofig.apPassword,pData,dataSize);
		rt_kprintf("Set Ap Password: %s\r\n", pCfg->networkdCofig.apPassword);
		FlashWrite((rt_uint8_t *)pCfg, sizeof(Config));
	}
}

rt_uint8_t *GetApSSID(void)
{
	return configInfo.cfg.networkdCofig.apSSID;
}

void SetApSSID(rt_uint8_t *pData, rt_size_t dataSize)
{
	Config *pCfg = &configInfo.cfg;

	if(0 != rt_strncmp((char *)pCfg->networkdCofig.apSSID, (char *)pData, dataSize))
	{
		rt_memset(pCfg->networkdCofig.apSSID,0,AP_MAX_SSID_LENGTH);
		rt_memcpy(pCfg->networkdCofig.apSSID,pData,dataSize);
		rt_kprintf("Set Ap SSID: %s\r\n", pCfg->networkdCofig.apSSID);
		FlashWrite((rt_uint8_t *)pCfg, sizeof(Config));
	}
}

rt_uint8_t GetDhcpMode(void)
{
	return configInfo.cfg.networkdCofig.dhcpMode;
}

void SetDhcpMode(rt_uint8_t data)
{
	Config *pCfg = &configInfo.cfg;

	if(pCfg->networkdCofig.dhcpMode != data)
	{
		pCfg->networkdCofig.dhcpMode = data;
		rt_kprintf("Set DHCP Mode: %d\r\n", pCfg->networkdCofig.dhcpMode);
		FlashWrite((rt_uint8_t *)pCfg, sizeof(Config));
	}
}

rt_uint8_t *GetMacAddress(void)
{
	return configInfo.cfg.networkdCofig.macAddress;
}

rt_bool_t SetMacAddress(rt_uint8_t *pData, rt_size_t dataSize)
{
	rt_bool_t retVal = RT_FALSE;
	Config *pCfg = &configInfo.cfg;

	if(0 != rt_strncmp((char *)pCfg->networkdCofig.macAddress, (char *)pData, dataSize))
	{
		rt_memset(pCfg->networkdCofig.macAddress,0,MAC_LENGTH);
		rt_memcpy(pCfg->networkdCofig.macAddress,pData,dataSize);
		rt_kprintf("Set Mac Address: %s\r\n", pCfg->networkdCofig.macAddress);
		retVal = FlashWrite((rt_uint8_t *)pCfg, sizeof(Config));
	}

	return retVal;
}

//Save the Device Info
rt_uint8_t *GetDeviceInfo(void)
{
	return configInfo.cfg.device;
}

void SetDeviceInfo(rt_uint8_t *pData, rt_size_t dataSize)
{
	Config *pCfg = &configInfo.cfg;

	if(0 != rt_strncmp((char *)pCfg->device, (char *)pData, dataSize))
	{
		rt_memset(pCfg->device,0,sizeof(pCfg->device));
		rt_memcpy(pCfg->device,pData,dataSize);
		rt_kprintf("Set Device Info: %s\r\n", pCfg->device);
		FlashWrite((rt_uint8_t *)pCfg, sizeof(Config));
	}
}

rt_uint8_t GetManufactureMode(void)
{
	return configInfo.cfg.manufacture;
}

rt_bool_t SetManufacture(rt_uint8_t on)
{
	rt_bool_t retVal = RT_FALSE;
	Config *pCfg = &configInfo.cfg;

	if(pCfg->manufacture != on)
	{
		rt_kprintf("Set Manufacture %s\r\n", (ENABLE==on)?"On":"Off");
		pCfg->manufacture = on;
		retVal = FlashWrite((rt_uint8_t *)pCfg, sizeof(Config));
	}

	return retVal;
}

rt_uint8_t GetDomainConfig(void)
{
	return configInfo.cfg.networkdCofig.domainOn;
}

void SetDomainConfig(rt_uint8_t domainOn)
{
	Config *pCfg = &configInfo.cfg;

	if(pCfg->networkdCofig.domainOn != domainOn)
	{
		pCfg->networkdCofig.domainOn = domainOn;
		rt_kprintf("Set Domain %s \r\n", (ENABLE == pCfg->networkdCofig.domainOn)?"On":"Off");
		FlashWrite((rt_uint8_t *)pCfg, sizeof(Config));
	}
}

rt_uint8_t *GetDomainInfo(void)
{
	return configInfo.cfg.networkdCofig.domainInfo;
}

void SetDomainInfo(rt_uint8_t *pData, rt_size_t dataSize)
{
	Config *pCfg = &configInfo.cfg;

	if(0 != rt_strncmp((char *)pCfg->networkdCofig.domainInfo, (char *)pData, dataSize))
	{
		rt_memset(pCfg->networkdCofig.domainInfo,0,MAX_DOMAIN_LENGTH);
		rt_memcpy(pCfg->networkdCofig.domainInfo,pData,dataSize);
		rt_kprintf("Set Domain Info: %s\r\n", pCfg->networkdCofig.domainInfo);
		FlashWrite((rt_uint8_t *)pCfg, sizeof(Config));
	}
}

void ShowConfig(void)
{
	rt_kprintf("-----------------------------------------------\r\n");
	rt_kprintf("           Device Config Information           \r\n");
	rt_kprintf("-----------------------------------------------\r\n");
	rt_kprintf(" Model : %s(%s)\r\n", MODEL_NAME, PRODUCT_NAME);
	rt_kprintf(" Local IP : %s\r\n", configInfo.cfg.networkdCofig.localIp);
	rt_kprintf(" SubNet Mask : %s\r\n", configInfo.cfg.networkdCofig.subnetMask);
	rt_kprintf(" Default Gateway : %s\r\n", configInfo.cfg.networkdCofig.gatewayIp);
	rt_kprintf(" DNS Server : %s\r\n", configInfo.cfg.networkdCofig.dnsServer);
	rt_kprintf(" Destination IP/Port : %s/%d\r\n",configInfo.cfg.networkdCofig.destIp, configInfo.cfg.networkdCofig.destPort);
	rt_kprintf(" AP Information SSID/Password.(%s/%s)\r\n", configInfo.cfg.networkdCofig.apSSID, configInfo.cfg.networkdCofig.apPassword);
	rt_kprintf(" Mac Address : %s\r\n",configInfo.cfg.networkdCofig.macAddress);
	rt_kprintf(" DHCP Mode: %s\r\n",(ENABLE == GetDhcpMode())?"On":"Off" );
	rt_kprintf(" Device Info: %s \r\n", configInfo.cfg.device);
	rt_kprintf(" Domain %s \r\n", (ENABLE == GetDomainConfig())?"On":"Off");
	rt_kprintf(" Domain Information: %s \r\n", configInfo.cfg.networkdCofig.domainInfo);
	rt_kprintf("-----------------------------------------------\r\n");
}

rt_bool_t LoadConfig(void)
{
	rt_kprintf("Load Config.\r\n");
	Config *pCfg = &configInfo.cfg;

	rt_bool_t retVal = FlashRead((rt_uint8_t *)pCfg, sizeof(Config));

	if(RT_FALSE == retVal || WATERMARK_VALUE != pCfg->waterMark)
	{
		rt_kprintf("Set Factory Reset...\r\n");
		rt_memset(pCfg,0,sizeof(Config));
		pCfg->networkdCofig.destPort = 0;
		pCfg->networkdCofig.dhcpMode = 1;
		pCfg->networkdCofig.domainOn = DISABLE;
		pCfg->waterMark = WATERMARK_VALUE;
		pCfg->manufacture = DISABLE;
		rt_memcpy(pCfg->networkdCofig.apSSID,DEFAULT_SSID,rt_strlen(DEFAULT_SSID));
		rt_memcpy(pCfg->networkdCofig.apPassword,DEFAULT_PASSWORD,rt_strlen(DEFAULT_PASSWORD));
		rt_memcpy(pCfg->networkdCofig.destIp,DEFAULT_IP,rt_strlen(DEFAULT_IP));
		rt_memcpy(pCfg->networkdCofig.dnsServer,DEFAULT_IP,rt_strlen(DEFAULT_IP));
		rt_memcpy(pCfg->networkdCofig.gatewayIp,DEFAULT_IP,rt_strlen(DEFAULT_IP));
		rt_memcpy(pCfg->networkdCofig.localIp,DEFAULT_IP,rt_strlen(DEFAULT_IP));
		rt_memcpy(pCfg->networkdCofig.subnetMask,DEFAULT_IP,rt_strlen(DEFAULT_IP));
		rt_memcpy(pCfg->networkdCofig.macAddress,DEFAULT_MAC,rt_strlen(DEFAULT_MAC));
		rt_memcpy(pCfg->device,DEFAULT_DEVICE,rt_strlen(DEFAULT_DEVICE));
		rt_memcpy(pCfg->networkdCofig.domainInfo,DEFAULT_DOMAIN,rt_strlen(DEFAULT_DOMAIN));

		retVal = FlashWrite((rt_uint8_t *)pCfg, sizeof(Config));
	}

	return retVal;
}

static void InitConfigurationInfo(void)
{
	rt_memset(&configInfo.cfg, 0, sizeof(configInfo.cfg));
}	

rt_bool_t InitConfiguration(void)
{
	rt_kprintf("Init Configuration.\r\n");
	rt_bool_t retVal = RT_FALSE;

	InitConfigurationInfo();
	if(RT_TRUE == (retVal=LoadConfig()))
	{
		ShowConfig();
	}
	else
	{
		rt_kprintf("init configuration failed.\r\n");
	}

	return retVal;
}
