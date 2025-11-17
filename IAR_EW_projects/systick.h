/**
  * @file    systick.h
  * @brief   Файл содержит прототипы функций SysTick
  */

/** Define to prevent recursive inclusion *************************************/
#ifndef __SYSTICK_H__
#define __SYSTICK_H__

/** Includes ******************************************************************/
#include <stdint.h>

/** Functions *****************************************************************/

	/**
	! Функция SysTick_Init инициализирует системный таймер SysTick.
	- frequency - тактовая частота МК.
	*/
void SysTick_Init(uint32_t frequency);

	/**
	! Функция delay_ms создает блокирующую задержку.
	- ms - продолжительность блокирубщей задержки в миллисекундах.
	*/
void delay_ms(uint32_t ms);

	/**
	! Функция get_current_time возвращает текущее системное время после сброса
		как количество произошедших вызовов SysTick_Handler().
	return: текущее системное время.
	*/
uint32_t get_current_time(void);

	/**
	! Функция is_time_passed определяет, прошло ли заданное время delay_time
		после момента start_time. Время определяется как количество прерываний
		SysTick_Handler().
	- start_time - стартовое время, от которого отсчитывается задержка.
	- delay_time - время неблокирующей задержки.
	return: 1 если время уже прошло, 0 если еще не прошло.
	*/
uint32_t is_time_passed(
	uint32_t	start_time,
	uint32_t	delay_time
);

	/**
	! Функция SysTick_Handler является обработчиком прерывания SysTick.
		Увеличивает значение счетчика прерываний.
	*/
void SysTick_Handler(void);

#endif /*__SYSTICK_H__ */