/*
 * File      : startup.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://openlab.rt-thread.com/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-08-31     Bernard      first implementation
 */

#include <rthw.h>
#include <rtthread.h>
#include <stm32f1xx_hal.h>
#include <stm32f1xx.h>

#include <board.h>
#include <drv_usart.h>
/**
 * @addtogroup STM32
 */

/*@{*/

extern int  rt_application_init(void);

#ifdef __CC_ARM
extern int Image$$RW_IRAM1$$ZI$$Limit;
#elif __ICCARM_
#pragma section="HEAP"
#else
extern int __bss_end;
#endif

/*******************************************************************************
* Function Name  : assert_failed
* Description    : Reports the name of the source file and the source line number
*                  where the assert error has occurred.
* Input          : - file: pointer to the source file name
*                  - line: assert error line source number
* Output         : None
* Return         : None
*******************************************************************************/
void assert_failed(uint8_t * file, uint32_t line)
{
    rt_kprintf("\n\r Wrong parameter value detected on\r\n");
    rt_kprintf("       file  %s\r\n", file);
    rt_kprintf("       line  %d\r\n", line);
    while (1) ;
}

extern void rt_hw_systick_init(void);
void rt_hw_board_init()
{
    /* HAL_Init() function is called at the beginning of the program */
    HAL_Init();

    /* System clock initialization */
    SystemClock_Config();

    rt_hw_systick_init();

    /* Heap initialization */
#if defined(RT_USING_HEAP)
    rt_system_heap_init((void *)HEAP_BEGIN, (void *)HEAP_END);
#endif

    /* Pin driver initialization is open by default */
#ifdef RT_USING_PIN
    rt_hw_pin_init();
#endif

    /* USART driver initialization is open by default */
#ifdef RT_USING_SERIAL
    rt_hw_usart_init();
#endif
}
/*@}*/
