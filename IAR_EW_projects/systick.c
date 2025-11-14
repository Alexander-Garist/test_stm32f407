/**
  * @file    systick.c
  * @brief   Файл содержит реализации функций системного таймера SysTick
  */

/* Includes ------------------------------------------------------------------*/
#include "systick.h"

volatile uint32_t systick_counter = 0;      // Счетчик вызовов SysTick_Handler()

/************************* Глобальные функции *********************************/

void SysTick_Init(uint32_t frequency, uint32_t ms_one_interrupt)
{
    SysTick->CTRL = 0;															// Начальное обнуление регистра управления и статуса
    SysTick->LOAD = (frequency / 1000) * ms_one_interrupt - 1;					// Регистр предзагрузки, устанавливается значение для перезагрузки при обнулении
    SysTick->VAL = 0;															// Начальное обнуление счетного регистра

	// Установка регистра управления и статуса
    SysTick->CTRL |= (0x1 << 2)       //	Выбор clock source
        |(0x1 << 1)                   //	Разрешение вызывать обработчик прерывания SysTick_Handler() при достижении 0 в счетчике
        |(0x1 << 0);                  //	Включение
}

void delay_ms(uint32_t ms)
{
    uint32_t startTime = systick_counter;
    while((systick_counter - startTime) < ms){}
}

uint32_t get_current_time(void)
{
    return systick_counter;
}

uint32_t is_time_passed(uint32_t start_time, uint32_t delay_time)
{
    return(systick_counter - start_time) >= delay_time;
}

/*************** Обработчик прерываний системного таймера *********************/
void SysTick_Handler(void)
{
    systick_counter++;
}