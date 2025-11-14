#ifndef SYSTICK_H
#define SYSTICK_H

#include "CMSIS/stm32f4xx.h"
//#include "stm32f4xx.h"
#define FREQUENCY   (16000000UL)        //HSI 16 МГц

extern volatile uint32_t systick_counter;           //Счетчик вызовов SysTick_Handler()

//Функции для использования в других модулях
void SysTick_Enable(uint32_t freq);             //Включение SysTick
void delay_ms(uint32_t ms);                     //Блокирующая задержка
uint32_t get_current_time(void);    //Получить текущее системное время после RESET
                                    //как количество произошедших вызовов SysTick_Handler()

uint32_t is_time_passed(uint32_t start_time, uint32_t delay_time);      //Проверка прошло ли время delay_time после момента start_time
                                                                        //измеряется как количество произошедших вызовов SysTick_Handler()

#endif