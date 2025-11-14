#ifndef EXTI_H
#define EXTI_H

#include "CMSIS/stm32f4xx.h"

typedef enum        //9.2.3 SYSCFG_EXTICR[x]
{
    EXTI_PortA = 0x00,
    EXTI_PortB = 0x01,
    EXTI_PortC = 0x02,
    EXTI_PortD = 0x03,
    EXTI_PortE = 0x04,
    EXTI_PortF = 0x05,
    EXTI_PortG = 0x06,
    EXTI_PortH = 0x07,
    EXTI_PortI = 0x08
}EXTI_Port;

//Маски триггеров
#define EXTI_TRIGGER_RISING         (0x01)
#define EXTI_TRIGGER_FALLING        (0x02)
#define EXTI_TRIGGER_RISING_FALLING (0x03)

//пользовательские функции, можно использовать в других модулях
void EXTI_Enable_Pin(EXTI_Port port, uint32_t pin, uint32_t trigger);        //Разрешить внешние прерывания на конкретном пине
void EXTI_Disable_Pin(EXTI_Port port, uint32_t pin, uint32_t trigger);       //Запретить внешние прерывания на конкретном пине
void EXTI_Clear_Flag(uint32_t pin);                                          //Сбросить флаг ожидания на конкретном пине (функция вызывается в обработчике внешнего прерывания)
#endif