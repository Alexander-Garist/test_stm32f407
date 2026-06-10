/**
  * @file    button.h
  * @brief   Файл содержит информацию о подключении кнопки, временные промежутки нажатий/задержек,
				прототипы функций кнопки.
  */

/** Define to prevent recursive inclusion *****************************************************************************/
#ifndef __BUTTON_H__
#define __BUTTON_H__

/** Includes **********************************************************************************************************/
#include <stdint.h>

/** Defines ***********************************************************************************************************/

/** GPIO порт и пин, к которому подключена кнопка *********************************************************************/
#define BUTTON_GPIO_PORT	GPIOA
#define BUTTON_GPIO_PIN		0
#define BUTTON_EXTI_PORT	EXTI_PortA

/** Триггер по возрастающему фронту вызывает прерывание сразу при нажатии кнопки, что позволяет измерить время нажатия*/
#define BUTTON_TRIGGER  EXTI_TRIGGER_RISING_FALLING

// Временные промежутки для определения вида нажатия
#define DEBOUNCE_TIME			5	// Задержка для фильтрации дребезга
#define SHORT_PRESS_TIME		50	// Минимальная продолжительность короткого нажатия
#define LONG_PRESS_TIME			300	// Продолжительность длинного нажатия
#define RELEASED_TIME			250	// Максимальное время отжатия кнопки, после которого еще можно зафиксировать двойное нажатие, а не второе одиночное
#define INTERRUPT_ENABLE_TIME 	20	// Продолжительность запрета на обработку прерываний в мс

#define TRUE 	1	// Флаг установлен
#define FALSE 	0	// Флаг сброшен

// Виды нажатия
typedef enum
{
	RELEASED 		   = 0,	// Кнопка отпущена
	SINGLE_SHORT_PRESS = 1,	// одиночное короткое
	SINGLE_LONG_PRESS  = 2,	// одиночное длинное
	DOUBLE_PRESS 	   = 3	// двойное
}Button_Press_t;

extern Button_Press_t Button_State;					// Состояние кнопки нажата/не нажата, если нажата, то как именно
extern uint32_t Button_Last_Press_Time_ms;		// Время последнего нажатия
extern uint32_t Button_Last_Release_Time_ms;	// Время последнего нажатия
extern uint32_t Button_Press_Duration_ms;		// Продолжительность последнего нажатия кнопки
extern uint8_t button_is_pressed;
extern uint8_t button_is_released;	// кнопка отпущена

extern uint32_t Short_Press_Release_ms;	// запомнить момент отпускания после короткого нажатия



/** Функции-обработчики конкретных нажатий ****************************************************************************/

	/**
	! Функция-обработчик одиночного короткого нажатия.
	*/
void Button_Single_Short_Press_Handler(void);

	/**
	! Функция-обработчик одиночного длинного нажатия.
	*/
void Button_Single_Long_Press_Handler(void);

	/**
	! Функция-обработчик двойного нажатия.
	*/
void Button_Double_Press_Handler(void);

#endif /* __BUTTON_H__ */