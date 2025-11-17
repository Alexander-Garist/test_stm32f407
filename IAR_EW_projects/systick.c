/**
  * @file    systick.c
  * @brief   Файл содержит реализации функций системного таймера SysTick
  */

/** Includes ******************************************************************/
#include "systick.h"
#include "CMSIS/stm32f4xx.h"

/** Defines *******************************************************************/
#define ms_one_interrupt	10		// Период 1 прерывания SysTick ( в мс )

/** Variables *****************************************************************/
volatile uint32_t systick_counter = 0;      // Счетчик вызовов SysTick_Handler()

/** Functions *****************************************************************/

	/**
	! Функция SysTick_Init инициализирует системный таймер SysTick.
	- frequency - тактовая частота МК.
	*/
void SysTick_Init(uint32_t frequency)
{
    SysTick->CTRL = 0;															// Начальное обнуление регистра управления и статуса
    SysTick->LOAD = (frequency / 1000) * ms_one_interrupt - 1;					// Регистр предзагрузки, устанавливается значение для перезагрузки при обнулении
    SysTick->VAL = 0;															// Начальное обнуление счетного регистра

	// Установка регистра управления и статуса
    SysTick->CTRL |= (0x1 << 2)       //	Выбор clock source
        |(0x1 << 1)                   //	Разрешение вызывать обработчик прерывания SysTick_Handler() при достижении 0 в счетчике
        |(0x1 << 0);                  //	Включение
}

	/**
	! Функция delay_ms создает блокирующую задержку.
	- ms - продолжительность блокирубщей задержки в миллисекундах.
	*/
void delay_ms(uint32_t ms)
{
    uint32_t startTime = systick_counter;
    while ((systick_counter - startTime) < ms){}
}

	/**
	! Функция get_current_time возвращает текущее системное время после сброса
		как количество произошедших вызовов SysTick_Handler().
	return: текущее системное время.
	*/
uint32_t get_current_time(void)
{
    return systick_counter;
}

	/**
	! Функция is_time_passed определяет, прошло ли заданное время delay_time
		после момента start_time. Время определяется как количество прерываний
		SysTick_Handler().
	- start_time - стартовое время, от которого отсчитывается задержка.
	- delay_time - время неблокирующей задержки.
	return: 1 если время уже прошло, 0 если еще не прошло.
	*/
uint32_t is_time_passed(uint32_t start_time, uint32_t delay_time)
{
    return(systick_counter - start_time) >= delay_time;
}

/*************** Обработчик прерываний системного таймера *********************/

	/**
	! Функция SysTick_Handler является обработчиком прерывания SysTick.
		Увеличивает значение счетчика прерываний.
	*/
void SysTick_Handler(void)
{
    systick_counter++;
}