/*
 * plccomm.h
 *
 */

#ifndef __DUSTCOMM_H__
#define __DUSTCOMM_H__

typedef struct pl_data_tag {
	rt_uint8_t size; // data size
	rt_uint8_t data[128];
}PlData;

typedef struct pl_data_Info_tag {
	PlData data;
	rt_uint8_t valid;
} PlDataInfo;

void SetSendEvent(void);
void SetTcpStatus(rt_uint8_t tcpOn);
rt_uint8_t GetTcpStatus(void);
rt_bool_t InitPlcComm(void);

#endif /* __DUSTCOMM_H__ */
