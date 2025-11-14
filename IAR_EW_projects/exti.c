#include "exti.h"

//Статические функции, нужны для сокрытия их реализации в других модулях
static void EXTI_InitPin(EXTI_Port port, uint32_t pin, uint32_t trigger)    //Инициализация вывода для разрешения обработки внешних прерываний
{
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;                           //Для разрешения обработки прерываний
    uint32_t EXTI_CR_INDEX = pin / 4;  //Выбрать конкретный регистр EXTICR(0,1,2,3)
                                       //Пины 0,1,2,3 EXTICR[0]
                                       //Пины 4,5,6,7 EXTICR[1]
                                       //Пины 8,9,10,11 EXTICR[2]
                                       //Пины 12,13,14,15 EXTICR[3]
    uint32_t EXTI_CR_POSITION = (pin % 4) * 4;  //Конкретный бит в регистре
                                                //определяет порт A, B, C и т.д.
                                                //например порт D (0x03): вместо #define SYSCFG_EXTICR2_EXTI5_PD  ((uint16_t)0x0030) /*!<PD[5] pin */
                                                //пин 5: (5 % 4)*4 = 4, значит  0x03 сдвигаем на 4 позиции влево, получается 0x30

    SYSCFG->EXTICR[EXTI_CR_INDEX] &= ~(0xF << EXTI_CR_POSITION);    //Начальное обнуление всех битов
    SYSCFG->EXTICR[EXTI_CR_INDEX] |= (port << EXTI_CR_POSITION);

    EXTI->RTSR &= ~(0x1 << pin);    //Начальное обнуление регистров триггеров
    EXTI->FTSR &= ~(0x1 << pin);

    if(trigger & EXTI_TRIGGER_RISING)EXTI->RTSR |= (0x1 << pin);  //Выбор триггера
    if(trigger & EXTI_TRIGGER_FALLING)EXTI->FTSR |= (0x1 << pin);

    EXTI->PR = (1 << pin);  //Начальное обнуление регистра ожидания
}
static void EXTI_EnableIRQ(uint32_t pin)                                    //Разрешение внешних прерываний на конкретном пине
{
    EXTI->IMR |= (1 << pin);

    switch(pin) {                           //Включение внешних прерываний на конкретном пине в вектор прерываний
        case 0:
            NVIC_EnableIRQ(EXTI0_IRQn);     //Включение обработчиков внешних прерываний
            break;
        case 1:
            NVIC_EnableIRQ(EXTI1_IRQn);
            break;
        case 2:
            NVIC_EnableIRQ(EXTI2_IRQn);
            break;
        case 3:
            NVIC_EnableIRQ(EXTI3_IRQn);
            break;
        case 4:
            NVIC_EnableIRQ(EXTI4_IRQn);
            break;
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            NVIC_EnableIRQ(EXTI9_5_IRQn);
            break;
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            NVIC_EnableIRQ(EXTI15_10_IRQn);
            break;
    }
}
static void EXTI_DisableIRQ(uint32_t pin)                                   //Запрет внешних прерываний на конкретном пине
{
    EXTI->IMR &= ~(1 << pin);

    switch(pin) {                               //Отключение внешних прерываний на конкретном пине в вектор прерываний
        case 0:
            NVIC_DisableIRQ(EXTI0_IRQn);        //Отключение обработчиков внешних прерываний
            break;
        case 1:
            NVIC_DisableIRQ(EXTI1_IRQn);
            break;
        case 2:
            NVIC_DisableIRQ(EXTI2_IRQn);
            break;
        case 3:
            NVIC_DisableIRQ(EXTI3_IRQn);
            break;
        case 4:
            NVIC_DisableIRQ(EXTI4_IRQn);
            break;
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            NVIC_DisableIRQ(EXTI9_5_IRQn);
            break;
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            NVIC_DisableIRQ(EXTI15_10_IRQn);
            break;
    }
}
static void EXTI_ClearPR(uint32_t pin)                                      //Сброс флага ожидания (нужно вызывать в обработчике прерывания, чтобы при выходе из него не попасть сразу же снова в него)
{
    EXTI->PR = (1 << pin);
}

//Функции для использования в других модулях
void EXTI_Enable_Pin(EXTI_Port port, uint32_t pin, uint32_t trigger)         //Разрешить внешние прерывания на конкретном пине
{
    EXTI_InitPin(port, pin, trigger);
    EXTI_EnableIRQ(pin);
}
void EXTI_Disable_Pin(EXTI_Port port, uint32_t pin, uint32_t trigger)        //Запретить внешние прерывания на конкретном пине
{
    EXTI_InitPin(port, pin, trigger);
    EXTI_DisableIRQ(pin);
}
void EXTI_Clear_Flag(uint32_t pin)                                           //Сбросить флаг ожидания на конкретном пине (функция вызывается в обработчике внешнего прерывания)
{
    EXTI_ClearPR(pin);
}