/*
 * flash.c
 *
 */

#include <rtthread.h>
#include <drv_flash/drv_flash.h>

#include "flash.h"

rt_bool_t FlashRead(rt_uint8_t *pData, size_t size)
{
	rt_bool_t retVal = RT_FALSE;

	if(RT_NULL != pData && 0 < size)
	{
		rt_uint8_t buf[FLASH_PAGESIZE];
		rt_int32_t i32Val = stm32_flash_read(FLASH_WATERMART_ADDRESS, buf, size);
		if(0 < i32Val)
		{
			rt_memcpy(pData,buf,size);
			retVal = RT_TRUE;
		}
		else
		{
			rt_kprintf("Fail to Flash read.(%d)\r\n", i32Val);
		}
	}

	return retVal;
}

rt_bool_t FlashWrite(rt_uint8_t *pData, size_t size)
{
	rt_bool_t retVal = RT_FALSE;

	if(RT_NULL != pData && 0 < size)
	{
		rt_int32_t i32Val = stm32_flash_erase(FLASH_WATERMART_ADDRESS, FLASH_PAGESIZE);
		if(0 < i32Val)
		{
			if(0 < (i32Val=stm32_flash_write(FLASH_WATERMART_ADDRESS, pData, size)))
			{
				retVal = RT_TRUE;
			}
			else
			{
				rt_kprintf("Fail to Flash Write\r\n");
			}
		}
		else
		{
			rt_kprintf("Fail to Flash Erase\r\n");
		}
	}
	else
	{
		rt_kprintf("Fail write data...\r\n");
	}

	return retVal;
}

