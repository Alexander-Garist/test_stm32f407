#include <stdint.h>
#include "LED.h"
#include "gpio.h"

uint32_t Blink_Period = TIME_SHORT;		// Период моргания светодиодов в основном цикле while в мс

uint32_t Green_Blinks_Counter = 0;
uint32_t Orange_Blinks_Counter = 0;
uint32_t Red_Blinks_Counter = 0;
uint32_t Blue_Blinks_Counter = 0;

uint32_t Blink_Mode = 0;

// callback функция для установки частоты моргания
void LED_Set_Blink_Period(uint32_t time)
{
    Blink_Period = time;
}

	/**
	! Функция изменения режима моргания, вызывается в обработчике внешнего прерывания EXTI0_IRQHandler.
	- указатель на callback функцию.
*/
void LED_change_blink_mode(void (*callback)(uint32_t))
{
    Blink_Mode++;									// Нажатие кнопки переключает режим моргания на следующий
    if (Blink_Mode > 2) Blink_Mode %= 3;			// Защита от переполнения количества режимов моргания

    if (Blink_Mode == 0) callback(TIME_SHORT);		// Эквивалентно Blink_Period = TIME_SHORT;
    if (Blink_Mode == 1) callback(TIME_MEDIUM);		// Эквивалентно Blink_Period = TIME_MEDIUM;
    if (Blink_Mode == 2) callback(TIME_LONG);		// Эквивалентно Blink_Period = TIME_LONG;
}

	/**
	! Функция включения всех 4 пользовательских светодиодов на плате: PD12, PD13, PD14, PD15.
	*/
void LED_turnON_4_LED(void)
{
    GPIO_set_HIGH(GPIO_PORT_LED_0, GPIO_PIN_LED_0);
    GPIO_set_HIGH(GPIO_PORT_LED_1, GPIO_PIN_LED_1);
    GPIO_set_HIGH(GPIO_PORT_LED_2, GPIO_PIN_LED_2);
    GPIO_set_HIGH(GPIO_PORT_LED_3, GPIO_PIN_LED_3);
}

	/**
	! Функция выключения всех 4 пользовательских светодиодов на плате: PD12, PD13, PD14, PD15.
	*/
void LED_turnOFF_4_LED(void)
{
    GPIO_set_LOW(GPIO_PORT_LED_0, GPIO_PIN_LED_0);
    GPIO_set_LOW(GPIO_PORT_LED_1, GPIO_PIN_LED_1);
    GPIO_set_LOW(GPIO_PORT_LED_2, GPIO_PIN_LED_2);
    GPIO_set_LOW(GPIO_PORT_LED_3, GPIO_PIN_LED_3);
}