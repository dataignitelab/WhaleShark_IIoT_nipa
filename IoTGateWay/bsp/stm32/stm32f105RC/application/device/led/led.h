/*
 * led.h
 *
 *  Created on: 2020. 9. 25.
 *      Author: hyuns
 */
#ifndef DEVICE_LED_LED_H_
#define DEVICE_LED_LED_H_

#define POWER_LED		GET_PIN(C, 6)
#define WIFI_LED		GET_PIN(C, 7)
#define CHANNEL1_LED	GET_PIN(C, 8)
#define CHANNEL2_LED	GET_PIN(C, 9)

void StartBlinkTimer(void);
void LedBlinkTurnOn(rt_base_t GPIO_Pin);
void LedTurnOff(rt_base_t GPIO_Pin);
void LedTurnOn(rt_base_t GPIO_Pin);
void DeinitLed(void);
rt_bool_t InitLed(void);


#endif /* DEVICE_LED_LED_H_ */
