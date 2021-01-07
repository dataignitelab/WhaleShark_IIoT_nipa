/*
 * usbconsole.h
 *
 */

#ifndef __USBCONSOLE_H__
#define __USBCONSOLE_H__

typedef enum cli_ID
{
	CLI_HELP=0, CLI_SCFG, CLI_RSET,
	CLI_RBOT, CLI_SLIP, CLI_SSNM,
	CLI_SGWY, CLI_SDNS, CLI_STCP,
	CLI_DHCP, CLI_SSID, CLI_SMFR,
	CLI_SMAC, CLI_SDEV,	CLI_SDMC,
	CLI_SDMI,

	CLI_MAX
} cliId;

#pragma pack(push, 1)
typedef struct _cli_Info{
	cliId	id;
	char	szCli[5];
	char	szComment[64];
} CLIINFO;
#pragma pack(pop)

rt_bool_t InitUsbconsole(void);

#endif /* __USBCONSOLE_H__ */
