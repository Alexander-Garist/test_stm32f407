/**
  * @file    systick.c
  * @brief   Файл содержит реализации функций системного таймера SysTick
  */

/** Includes ******************************************************************/
#include "systick.h"
#include "CMSIS/stm32f4xx.h"

/** Defines *******************************************************************/
#define ms_one_interrupt	100		// Период 1 прерывания SysTick ( в мс )
#define SysTick_FREQUENCY	16000000
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
	- delay_time - время неблокирующей задержки в мс.
	return: 1 если время уже прошло, 0 если еще не прошло.
	*/
uint32_t is_time_passed(uint32_t start_time, uint32_t delay_time)
{
    return(systick_counter - start_time) >= delay_time;
}

	/**
	! Функция is_passed_ms определяет, прошло ли заданное время delay_time неблокирующей задержки на delay_time мс
		независимо от периода прерываний системного таймера.
	- delay_time - время задержки в миллисекундах.
	return: прошло ли заданное время в мс
	*/
uint32_t is_time_passed_ms(uint32_t delay_time)	// время задержки в мс
{
	uint32_t start_time = SysTick->VAL;				// Начальное значение VAL, от которого начинается отсчет 1 мс
	uint32_t stop_time = 0;							// Конечное значение VAL, когда 1 мс пройдет

	// Определение конечного значения VAL
	uint32_t VAL_1_ms = SysTick_FREQUENCY / 1000;		// Значение VAL, соответствующее 1 мс. При частоте 16 МГц за 1 мс таймер отсчитывает 16000 отсчетов
	uint32_t VAL_delay_time = VAL_1_ms * delay_time;	// Значение VAL, соответствующее delay_time мс

	// Если начальное значение VAL позволит отсчитать x мс до перезагрузки счетного регистра (т.е. если оно не меньше 16000)
	if (start_time >= VAL_delay_time)
	{
		stop_time = start_time - VAL_delay_time;
	}

	// Если начальное значение VAL не позволит отсчитать 1 мс до перезагрузки счетного регистра (т.е. если оно меньше 16000)
	if (start_time < VAL_delay_time)
	{
		stop_time = VAL_1_ms * ms_one_interrupt;				// максимальное значение VAL == LOAD 16.000.000 * 100 / 1000 = 1.600.000 отсчетов
		stop_time = stop_time + start_time - VAL_delay_time ;	// отнять оставшиеся отсчеты
	}

	return (SysTick->VAL == stop_time);	//уменьшился ли VAL на 16000, т.е. прошла ли 1 мс
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