/*
 * manufacture.h
 *
 */

#ifndef MANUFACTURE_H_
#define MANUFACTURE_H_

#include <common/smsgdef.h>

rt_bool_t StartManufacture(void);
rt_bool_t InitManufacture(void);
void ManufactureSendMessage(MqData_t *pMqData);

#endif /* MANUFACTURE_H_ */
