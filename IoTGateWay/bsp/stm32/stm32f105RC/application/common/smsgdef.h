/*
 * File      : smsgdef.h
 */
 
#ifndef __SMSGDEF_H__
#define __SMSGDEF_H__

#define SMSG_TX_DATA					0x00000001 // uart send data
#define SMSG_RX_DATA					0x00000002 // uart receive data
#define SMSG_INC_DATA					0x00000004 // indicate data

#define DEFAULT_MQ_SIZE					5U
#define APPLICATION_MQ_SIZE				DEFAULT_MQ_SIZE
#define NETWORKMANAGER_MQ_SIZE			DEFAULT_MQ_SIZE
#define MANUFACTURE_MQ_SIZE				(DEFAULT_MQ_SIZE<<1)

typedef struct mq_data_tag {
    rt_uint32_t messge;
	rt_int16_t  size; // data size
	rt_uint8_t 	data[128];
} MqData_t;

#endif  // #ifndef __SMSGDEF_H__
