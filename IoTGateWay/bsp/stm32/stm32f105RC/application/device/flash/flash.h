/*
 * flash.h
 *
 */

#ifndef DEVICE_FLASH_FLASH_H_
#define DEVICE_FLASH_FLASH_H_

#define FLASH_PAGESIZE						2048

#define WATERMARK_VALUE						0xAF

#define FLASH_START_ADRESS					((uint32_t)0x08009000) //Usable size: 28k
#define FLASH_WATERMART_ADDRESS				FLASH_START_ADRESS

rt_bool_t FlashRead(rt_uint8_t *pData, size_t size);
rt_bool_t FlashWrite(rt_uint8_t *pData, size_t size);

#endif /* DEVICE_FLASH_FLASH_H_ */
