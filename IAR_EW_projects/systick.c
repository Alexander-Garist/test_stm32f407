/**
  * @file    systick.c
  * @brief   Файл содержит реализации функций системного таймера SysTick
  */

/** Includes ******************************************************************/
#include "systick.h"
#include "CMSIS/stm32f4xx.h"

/** Defines *******************************************************************/
#define SysTick_FREQUENCY	16000000							// Тактовая частота (16.000.000)
#define ms_per_interrupt	100									// Период прерываний SysTick в мс
#define ticks_per_ms		(SysTick_FREQUENCY / 1000)			// Количество тактов за 1 мс (16.000)
#define LOAD_max_val		(ticks_per_ms * ms_per_interrupt)	// Значение LOAD (1.600.000)

/** Variables *****************************************************************/
volatile uint32_t systick_counter = 0;      // Счетчик вызовов SysTick_Handler()
static uint32_t ms_counter = 0;				// Счетчик миллисекунд

/** Functions *****************************************************************/

	/**
	! Функция SysTick_Init инициализирует системный таймер SysTick. Тактовая частота 16 МГц, период одного прерывания
		SysTick_Handler 100 мс.
	*/
void SysTick_Init()
{
    SysTick->CTRL = 0;						// Начальное обнуление регистра управления и статуса
    SysTick->LOAD = LOAD_max_val - 1;		// Регистр предзагрузки, устанавливается значение для перезагрузки при обнулении
    SysTick->VAL = 0;						// Начальное обнуление счетного регистра

	// Установка регистра управления и статуса
    SysTick->CTRL |= (0x1 << 2)			// Выбор clock source
        |(0x1 << 1)						// Разрешение вызывать обработчик прерывания SysTick_Handler() при достижении 0 в счетчике
        |(0x1 << 0);					// Включение
}

	/**
	! Статическая функция SysTick_Update обновляет текущее значение счетчика мс. Так как прерывание системного таймера
		обновит только 100 мс, то данную функцию нужно вызывать во всех функциях задержек и получения текущего времени.
	*/
static void SysTick_Update_ms(void)
{
    static uint32_t accumulated_ticks = 0;		// Прошедшие такты процессора с момента последнего вызова данной функции
    uint32_t current_val = SysTick->VAL;		// Текущее значение счетчика

    // Определение сколько тактов прошло с последнего вызова
    static uint32_t last_VAL = 1600000 - 1;		// Последнее значение регистра VAL
    uint32_t elapsed_ticks;						// Количество прошедших тактов

	// Значение VAL могло просто уменьшиться, а могло перезагрузиться после пересечения 0
    if (current_val <= last_VAL)
	{
        elapsed_ticks = last_VAL - current_val;					// Нормальный случай: счетчик уменьшился
    }
	else
	{
        elapsed_ticks = (1600000 - current_val) + last_VAL;		// Счетчик перезагрузился (прошел через 0)
    }

	// Обновление счетчика прошедших тактов и изменение текущего значения VAL
    accumulated_ticks += elapsed_ticks;
    last_VAL = current_val;

    // Если накопилось достаточно тактов для 1 мс
    while (accumulated_ticks >= ticks_per_ms) {
        ms_counter++;
        accumulated_ticks -= ticks_per_ms;
    }
}

	/**
	! Функция get_current_ms используется для получения текущего системного времени в миллисекундах.
	return: текущее значение счетчика миллисекунд
	*/
uint32_t get_current_ms(void)
{
	SysTick_Update_ms();
	return ms_counter;
}

	/**
	! Функция is_time_passed_ms определяет, прошло ли заданное время delay_time_ms неблокирующей задержки от момента
		 start_time_ms.
	- start_time_ms - время начала отсчета задержки в миллисекундах.
	- delay_time - время задержки в миллисекундах.
	return: прошло ли заданное время в мс.
	*/
uint32_t is_time_passed_ms(uint32_t start_time_ms, uint32_t delay_time_ms)
{
	SysTick_Update_ms();
	return (ms_counter - start_time_ms) >= delay_time_ms;
}

	/**
	! Функция delay_ms создает блокирующую задержку.
	- ms - продолжительность блокирубщей задержки в миллисекундах.
	*/
void delay_ms(uint32_t ms)
{
    uint32_t startTime = ms_counter;
    while ((ms_counter - startTime) < ms)
	{
		SysTick_Update_ms();
	}
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