/*
 * File      : main.c
 * COPYRIGHT (C) 2020 
 */

/**
 * @addtogroup STM32
 */
/*@{*/

#include <rtthread.h>

#include "application.h"
#include "manufacture.h"
#include <util/debug.h>
#include <device/uart/usbconsole.h>
#include <config/configuration.h>
#include <config/appconfig.h>

int main(void)
{
	if(RT_TRUE == InitUsbconsole() && RT_TRUE == InitConfiguration())
	{
		if(ENABLE == GetManufactureMode())
		{
			if(RT_TRUE == InitManufacture())
			{
				StartManufacture();
			}
		}
		else if(RT_TRUE == InitApplication())
		{
			StartApplication();
		}
	}

	return RT_EOK;
}


/*@}*/
