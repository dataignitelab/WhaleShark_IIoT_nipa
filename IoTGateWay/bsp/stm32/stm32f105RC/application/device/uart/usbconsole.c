/*
 * usbconsole.c
 *
 */
#include <rtthread.h>
#include <string.h>
#include <stdlib.h>

#include "usbconsole.h"
#include "application.h"
#include <common/common.h>
#include <config/appconfig.h>
#include <config/configuration.h>

#define CLI_CMD_PREFIX						"AT+"
#define CLI_CMD_LENGTH						4 /*AT+XXXX*/

static CLIINFO s_cliInfo[] = {
	{CLI_HELP, "HELP", "[Show Command List]\n"}, 		{CLI_SCFG, "SCFG", "[Show Device Configuration]\n"},
	{CLI_RSET, "RSET", "[Device Reset]\n"}, 			{CLI_RBOT, "RBOT", "[Device Rebooting]\n"},
	{CLI_SMFR, "SMFR", "[Set Manufacture Mode]\n"},		{CLI_SLIP, "SLIP", "[Set Local IP]\n"},
	{CLI_SSNM, "SSNM", "[Set SubnetMask]\n"},			{CLI_SGWY, "SGWY", "[Set Gateway IP]\n"},
	{CLI_SDNS, "SDNS", "[Set DNS Server]\n"},			{CLI_STCP, "STCP", "[Set Destination IP/Port]\n"},
	{CLI_DHCP, "DHCP", "[Set DHCP Mode]\n"},			{CLI_SSID, "SSID", "[Set Target AP SSID/Password]\n"},
	{CLI_SMAC, "SMAC", "[Set MacAddress]\n"},           {CLI_SDEV, "SDEV", "[Set Device Type and Number]\n"},
	{CLI_SDMC, "SDMC", "[Set IP or Domain]\n"},			{CLI_SDMI, "SDMI", "[Set Domain Information]\n"}
};

typedef struct usbconsole_data_tag {
    rt_device_t uport;
    rt_event_t  rx_event;

	rt_uint8_t rxBuf[RT_SERIAL_RB_BUFSZ];

} usbconsole_data_t;

static usbconsole_data_t usbconsole_data;

void OnShowHelp(void)
{  
	rt_kprintf("****************************************************\r\n");
	rt_kprintf("*                CLI Command Help                  *\r\n");
	rt_kprintf("****************************************************\r\n");
	rt_kprintf("* Command Help --------------------------- AT+HELP *\r\n");
	rt_kprintf("* Device Configuration ------------------- AT+SCFG *\r\n");
	rt_kprintf("* Device Factory reset ------------------- AT+RSET *\r\n");
	rt_kprintf("* Set Local IP ------------------- AT+SLIP=<value> *\r\n");
	rt_kprintf("*  value => '<xxx.xxx.xxx.xxx>'                    *\r\n");
	rt_kprintf("*           'None':Current Local IP                *\r\n");
	rt_kprintf("* Set SubnetMask ----------------- AT+SSNM=<value> *\r\n");
	rt_kprintf("*  value => '<xxx.xxx.xxx.xxx>'                    *\r\n");
	rt_kprintf("*           'None':Current SubnetMask              *\r\n");
	rt_thread_delay(200);
	rt_kprintf("* Set Gateway IP ----------------- AT+SGWY=<value> *\r\n");
	rt_kprintf("*  value => '<xxx.xxx.xxx.xxx>'                    *\r\n");
	rt_kprintf("*           'None':Current Gateway IP              *\r\n");
	rt_kprintf("* Set DNS Server ----------------- AT+SDNS=<value> *\r\n");
	rt_kprintf("*  value => '<xxx.xxx.xxx.xxx>'                    *\r\n");
	rt_kprintf("*           'None':Current DNS Server              *\r\n");
	rt_kprintf("* Set Destination IP/Port -------- AT+STCP=<value> *\r\n");
	rt_kprintf("*  value => '<xxx.xxx.xxx.xxx>/<port>'             *\r\n");
	rt_kprintf("*           'None':Current Destination IP and Port *\r\n");
	rt_kprintf("* Set Target AP SSID/Password ---- AT+SSID=<value> *\r\n");
	rt_kprintf("*  value => <SSID>/<Password>                      *\r\n");
	rt_kprintf("*           'None':Current SSID and Password       *\r\n");
	rt_kprintf("* Set DHCP Mode ------------------ AT+DHCP=<value> *\r\n");
	rt_kprintf("*  value => 1: DHCP ON 0: DHCP OFF                 *\r\n");
	rt_kprintf("*           'None':Current DHCP Mode               *\r\n");
	rt_kprintf("* Set Mac Address ---------------- AT+SMAC=<value> *\r\n");
	rt_kprintf("*  value => '<xx:xx:xx:xx:xx:xx>'                  *\r\n");
	rt_kprintf("*           'None':Current Mac Address             *\r\n");
	rt_kprintf("* Set Devie Information----------- AT+SDEV=<value> *\r\n");
	rt_kprintf("*  value => '<xxxxxx>'                             *\r\n");
	rt_kprintf("*           'None':Current Device Information      *\r\n");
	rt_kprintf("* Set Manufacture Mode ------------AT+SMFR=<value> *\r\n");
	rt_kprintf("*  value => 1 or 0                                 *\r\n");
	rt_kprintf("****************************************************\r\n");
}

static void OnSetLocalIP(rt_uint8_t *pData, rt_size_t dataSize)
{
	if( 0 < dataSize)
	{
		SetLocalIP(pData,dataSize);
	}
	else
	{
		rt_kprintf("Get Local IP: %s\r\n",GetLocalIP());
	}
}

static void OnSetSubnetMask(rt_uint8_t *pData, rt_size_t dataSize)
{
	if( 0 < dataSize)
	{
		SetSubnetMask(pData,dataSize);
	}
	else
	{
		rt_kprintf("Get SubnetMask: %s\r\n", GetSubnetMask());
	}
}

static void OnSetGatewayIP(rt_uint8_t *pData, rt_size_t dataSize)
{
	if( 0 < dataSize)
	{
		SetGatewayIP(pData,dataSize);
	}
	else
	{
		rt_kprintf("Get Gateway Ip: %s\r\n",GetGatewayIP());
	}
}

static void OnSetDnsServer(rt_uint8_t *pData, rt_size_t dataSize)
{
	if( 0 < dataSize)
	{
		SetDnsServer(pData,dataSize);
	}
	else
	{
		rt_kprintf("Get DNS Server: %s\r\n",GetDnsServer());
	}
}

static void OnSetTcp(rt_uint8_t *pData, rt_size_t dataSize)
{
	if( 0 < dataSize)
	{
		char buff[32];

		rt_memcpy(buff,pData,dataSize);

		char *begin = strtok(buff,"/"); //IP
		char *end = strtok(RT_NULL,RT_NULL); //Port
		SetTcpIP((rt_uint8_t *)begin,(rt_size_t)(end-begin));
		rt_uint16_t port = atoi(end);
		SetTcpPort(port);
	}
	else
	{
		rt_kprintf("Get Destination IP/Port Information\r\n");
		rt_kprintf("Destination IP: %s\r\n", GetTcpIp());
		rt_kprintf("Destination Port Information: %d\r\n",GetTcpPort());
	}
}

static void OnSetApSSID(rt_uint8_t *pData, rt_size_t dataSize)
{
	if( 0 < dataSize)
	{
		char *apInformation = RT_NULL;
		char buff[32];

		rt_memset(buff,0,sizeof(buff));
		rt_memcpy(buff,pData,dataSize);

		 //AP SSID
		apInformation = strtok(buff,"/");
		SetApSSID((rt_uint8_t *)apInformation,strlen(apInformation));

		//AP Password
		if('\0' != *(buff+strlen(buff)+1))
		{
			apInformation = strtok(NULL,"\r");
		}
		else
		{
			rt_uint8_t empty = '\0';
			*apInformation = empty;
			rt_kprintf("Password empty\r\n");
		}
		SetApPassword((rt_uint8_t *)apInformation,strlen(apInformation));
	}
	else
	{
		rt_kprintf("Get SSID Information\r\n");
		rt_kprintf("SSID Name: %s\r\n",GetApSSID());
		rt_kprintf("SSID Password: %s\r\n",GetApPassword());
	}
}

static void OnSetDhcpMode(rt_uint8_t *pData, rt_size_t dataSize)
{
	if( 0 < dataSize)
	{
		rt_uint8_t dhcpMode = atoi((char *)pData);
		SetDhcpMode(dhcpMode);
	}
	else
	{
		rt_kprintf("DHCP Mode: %s", (ENABLE == GetDhcpMode())?"On":"Off" );
	}
}

static rt_bool_t OnSetMacAddress(rt_uint8_t *pData, rt_size_t dataSize)
{
	rt_bool_t retVal = RT_FALSE;

	if(0 < dataSize)
	{
		retVal = SetMacAddress(pData,dataSize);
	}
	else
	{
		rt_kprintf("Get Mac Address\r\n");
		rt_kprintf("Mac Address: %s\r\n", GetMacAddress());
	}

	return retVal;
}
//Set Device Info
static void OnSetDeviceInfo(rt_uint8_t *pData, rt_size_t dataSize)
{
	if( 0 < dataSize)
	{
		SetDeviceInfo(pData,dataSize);
	}
	else
	{
		rt_kprintf("Get Device Information: %s\r\n", GetDeviceInfo());
	}
}

static rt_bool_t OnSetManufacture(rt_uint8_t *pData, rt_size_t dataSize)
{
	rt_bool_t retVal = RT_FALSE;

	if( 0 < dataSize)
	{
		retVal = SetManufacture(strtoul((char *)pData, NULL, 10));
	}
	else
	{
		rt_kprintf("Get Manufacture Status: %s\r\n", (ENABLE == GetManufactureMode())?"On":"Off");
	}

	return retVal;
}

static void OnSetDomainConfig(rt_uint8_t *pData, rt_size_t dataSize)
{
	if( 0 < dataSize)
	{
		SetDomainConfig(strtoul((char *)pData, NULL, 10));
	}
	else
	{
		rt_kprintf("Get Domain Configuration: %s\r\n", (ENABLE == GetDomainConfig())?"On":"Off");
	}
}

static void OnSetDomainInfo(rt_uint8_t *pData, rt_size_t dataSize)
{
	if( 0 < dataSize)
	{
		SetDomainInfo(pData,dataSize);
	}
	else
	{
		rt_kprintf("Get Domain Information: %s\r\n", GetDomainInfo());
	}
}

static rt_err_t usbconsole_rx_ind(rt_device_t dev, rt_size_t size)
{
    return rt_event_send(usbconsole_data.rx_event, SMSG_RX_DATA);
}

rt_size_t ParserCliCommand(rt_uint8_t *p_buff, rt_size_t rx_size)
{
	rt_size_t consumed = 0;

	if(RT_NULL != p_buff && 0 < rx_size)
	{
		rt_uint8_t *begin=p_buff, *end=begin, *last=(p_buff+rx_size);

		do {
			if(CR == *end)
			{
				rt_uint8_t len = strlen(CLI_CMD_PREFIX);

				do {
					if(end < (begin+len))
					{
						begin = end;
					}
					else if(0 == _strncasecmp((const char *)begin, CLI_CMD_PREFIX, len))
					{ // find CLI Command("AT+")
						int8_t id = CLI_MAX;

						begin += len; /* skip "AT+" */
						while(id-- && (0 != _strncasecmp((const char *)begin, s_cliInfo[id].szCli, CLI_CMD_LENGTH)))
						{
							; // none
						}

						if(id < 0)
						{
							rt_kprintf("Not find the cli command.\r\n");
						}
						else
						{
							rt_size_t valLen = 0;
							rt_bool_t bReboot = RT_FALSE;

							begin += CLI_CMD_LENGTH;
							// AT+XXXX => get or AT+XXXX=xxxxxxxxxx => set
							if(begin != end)
							{ //
								begin++; // skip '='
								valLen = (rt_size_t)(end-begin);
							}

							switch(s_cliInfo[id].id)
							{
							case CLI_HELP:
								OnShowHelp();	
								break;
							case CLI_SCFG:
								ShowConfig();
								break;
							case CLI_RSET:
								SetReset();
							case CLI_RBOT:
								bReboot = RT_TRUE;
								break;
							case CLI_SLIP:
								OnSetLocalIP(begin,valLen);
								break;
							case CLI_SSNM:
								OnSetSubnetMask(begin,valLen);
								break;
							case CLI_SGWY:
								OnSetGatewayIP(begin,valLen);
								break;
							case CLI_SDNS:
								OnSetDnsServer(begin,valLen);
								break;
							case CLI_STCP:
								OnSetTcp(begin,valLen);
								break;
							case CLI_SSID:
								OnSetApSSID(begin,valLen);
								break;
							case CLI_DHCP:
								OnSetDhcpMode(begin,valLen);
								break;
							case CLI_SMFR:
								bReboot = OnSetManufacture(begin,valLen);
								break;
							case CLI_SMAC:
								OnSetMacAddress(begin,valLen);
								break;
							case CLI_SDEV: //Set Device Info
								OnSetDeviceInfo(begin,valLen);
								break;
							case CLI_SDMC:
								OnSetDomainConfig(begin,valLen);
								break;
							case CLI_SDMI:
								OnSetDomainInfo(begin,valLen);
								break;
							default :
								rt_kprintf("Unknown cli command.\r\n");
							}

							if(RT_TRUE == bReboot)
							{
								DeviceReboot();
							}
						}

						begin = end;
					}
				} while(++begin < end);
			}
		} while(++end < last);

		consumed = (rt_size_t)(begin-p_buff);
	}

	return consumed;
}

static void usbconsole_rx_thread(void *params)
{
    usbconsole_data_t *p_handle = (usbconsole_data_t *)params;
	rt_uint8_t *pBuf = p_handle->rxBuf;	
	rt_size_t uRemain = 0;
    rt_uint32_t events;

    rt_err_t err = rt_device_set_rx_indicate(p_handle->uport, usbconsole_rx_ind);
    RT_ASSERT(err == RT_EOK);
    while (1)
    {
		if(RT_EOK == (err=rt_event_recv(p_handle->rx_event, SMSG_RX_DATA, (RT_EVENT_FLAG_OR|RT_EVENT_FLAG_CLEAR), RT_WAITING_FOREVER, &events))
			&& (SMSG_RX_DATA & events))
        {
			rt_size_t ulBytes;

			do {
				if(RT_SERIAL_RB_BUFSZ <= uRemain)
				{
					uRemain = 0;
				}

				if((ulBytes=rt_device_read(p_handle->uport, 0, (pBuf+uRemain), (RT_SERIAL_RB_BUFSZ-uRemain))) > 0)
				{
					rt_size_t consumed = ParserCliCommand(pBuf, (uRemain += ulBytes));

					if(consumed > 0 && ((uRemain -= consumed) > 0))
					{
						memmove(pBuf, pBuf+consumed, uRemain);
					}
				}
			} while(0 < ulBytes);
        }
    }
}

void InitUsbconsoleData(void)
{
	usbconsole_data.uport = RT_NULL;
	usbconsole_data.rx_event = RT_NULL;
}	

rt_bool_t InitUsbconsole(void)
{
	rt_kprintf("Initialize usbconsole...\r\n");
    usbconsole_data_t *h_data = &usbconsole_data;
    rt_thread_t tid;

	InitUsbconsoleData();

    h_data->uport = rt_device_find(RT_CONSOLE_DEVICE_NAME);
    RT_ASSERT(h_data->uport != RT_NULL);

    h_data->rx_event = rt_event_create("usbconsole_rx", RT_IPC_FLAG_FIFO);
    RT_ASSERT(h_data->rx_event != RT_NULL);

    tid = rt_thread_create("usbconsole_main", usbconsole_rx_thread, (void *)h_data, USBCONSOLE_STACK_SIZE, RT_MAIN_THREAD_PRIORITY, 20);
    RT_ASSERT(tid != RT_NULL);

    /* thread start  */
    return (RT_EOK == rt_thread_startup(tid));
}
