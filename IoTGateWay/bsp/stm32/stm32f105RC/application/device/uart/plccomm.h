/*
 * plccomm.h
 *
 */

#ifndef __DUSTCOMM_H__
#define __DUSTCOMM_H__

void SetSendEvent(void);
void SetReceiveOn(rt_uint8_t receiveOn);
void SetTcpStatus(rt_uint8_t tcpOn);
rt_bool_t InitPlcComm(void);

#endif /* __DUSTCOMM_H__ */
