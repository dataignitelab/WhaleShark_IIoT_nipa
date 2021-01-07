/*
 * led.c
 *
 *  Created on: 2020. 9. 25.
 *      Author: hyuns
 */
#include <board.h>
#include <rtthread.h>

#include "led.h"

#define	BLINK_PERIOD			1000 // millisecond

typedef struct _LEDINFO {
	rt_bool_t ledInitialized;
	rt_base_t blinkGPIOPin;
	rt_uint32_t blinkPeriod;

	rt_timer_t blinkTimer;
} LedInfo;

static LedInfo ledInfo;

static void BlinkTimer(void *params)
{
	if(0 != ledInfo.blinkGPIOPin)
	{
		rt_base_t pin = ledInfo.blinkGPIOPin;
		rt_uint8_t pinVal = rt_pin_read(pin);

		rt_pin_write(pin, !pinVal);
	}
}

void StartBlinkTimer(void)
{
	rt_timer_stop(ledInfo.blinkTimer);
	rt_timer_start(ledInfo.blinkTimer);
}

void LedBlinkTurnOn(rt_base_t GPIO_Pin)
{
	rt_kprintf("Led Blink Turn On.\r\n");
	ledInfo.blinkGPIOPin = GPIO_Pin;

	StartBlinkTimer();
}

void LedTurnOff(rt_base_t GPIO_Pin)
{
	rt_kprintf("Led Turn Off. \r\n");
	rt_timer_stop(ledInfo.blinkTimer);
	rt_pin_write(GPIO_Pin, DISABLE);
}

void LedTurnOn(rt_base_t GPIO_Pin)
{
	rt_kprintf("Led Turn On. \r\n");
	rt_timer_stop(ledInfo.blinkTimer);
	rt_pin_write(GPIO_Pin, ENABLE);
}

void DeinitLed(void)
{
	if(RT_TRUE == ledInfo.ledInitialized)
	{
		ledInfo.ledInitialized = RT_FALSE;
	}
}

void InitLedInformation()
{
	ledInfo.ledInitialized = RT_FALSE;
	ledInfo.blinkGPIOPin = 0;
	ledInfo.blinkPeriod = BLINK_PERIOD;
}

rt_bool_t InitLed(void)
{
	rt_bool_t retVal = RT_TRUE;

	InitLedInformation();

	rt_pin_mode(POWER_LED, PIN_MODE_OUTPUT);
	rt_pin_mode(WIFI_LED, PIN_MODE_OUTPUT);
	rt_pin_mode(CHANNEL1_LED, PIN_MODE_OUTPUT);
	rt_pin_mode(CHANNEL2_LED, PIN_MODE_OUTPUT);

	rt_pin_write(POWER_LED, 1);

	ledInfo.blinkTimer = rt_timer_create(
								"blinktimer",
								BlinkTimer,
								&ledInfo,
								rt_tick_from_millisecond(BLINK_PERIOD),
								RT_TIMER_FLAG_PERIODIC);
	return retVal;
}

