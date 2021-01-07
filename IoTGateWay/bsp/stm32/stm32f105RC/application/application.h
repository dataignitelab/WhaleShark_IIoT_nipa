/*
 * File      : application.h
 */
 
#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include <common/smsgdef.h>

void ApplicationSendMessage(MqData_t *pMqData);
void DeviceReboot(void);
void AppSendMessage(MqData_t *pMqData);
rt_bool_t StartApplication(void);
rt_bool_t InitApplication(void);
#endif /* #ifndef __APPLICATION_H__ */
