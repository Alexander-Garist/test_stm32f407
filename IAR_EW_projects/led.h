/**
  * @file    LED.h
  * @brief   Файл содержит информацию о подключении встроенных светодиодов,	прототипы функций включения и режима работы.
  */

/** Define to prevent recursive inclusion *****************************************************************************/
#ifndef __LED_H__
#define __LED_H__

/** Выводы GPIO к которым подключены светодиоды на плате **************************************************************/

// Зеленый LED
#define GPIO_PORT_LED_0	GPIOD
#define GPIO_PIN_LED_0	12

// Оранжевый LED
#define GPIO_PORT_LED_1	GPIOD
#define GPIO_PIN_LED_1	13

// Красный LED
#define GPIO_PORT_LED_2	GPIOD
#define GPIO_PIN_LED_2	14

// Синий LED
#define GPIO_PORT_LED_3	GPIOD
#define GPIO_PIN_LED_3	15

/** Периоды моргания светодиодов в 3 режимах **************************************************************************/

#define TIME_SHORT      100
#define TIME_MEDIUM     200
#define TIME_LONG       500

// Период моргания
extern uint32_t Blink_Period;

// Счетчики морганий
extern uint32_t Green_Blinks_Counter;
extern uint32_t Orange_Blinks_Counter;
extern uint32_t Red_Blinks_Counter;
extern uint32_t Blue_Blinks_Counter;

// Текущий режим моргания
extern uint32_t Blink_Mode;

	/**
	! Функция установки режима моргания.
	- period - новый период моргания в мс.
	*/
void LED_Set_Blink_Period(uint32_t period);

	/**
	! Функция смены режима моргания светодиодов.
	- (*callback)(uint32_t) - callback функция для установки нового режима моргания.
	*/
void LED_change_blink_mode(void (*callback)(uint32_t));

	/**
	! Функция включения всех 4 пользовательских светодиодов на плате: PD12, PD13, PD14, PD15.
	*/
void LED_turnON_4_LED(void);

	/**
	! Функция выключения всех 4 пользовательских светодиодов на плате: PD12, PD13, PD14, PD15.
	*/
void LED_turnOFF_4_LED(void);

#endif /* __LED_H__ */