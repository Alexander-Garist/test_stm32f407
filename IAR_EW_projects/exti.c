/**
  * @file    exti.c
  * @brief   Файл содержит реализации функций EXTI
  */

/** Includes **********************************************************************************************************/
#include "exti.h"
#include "CMSIS/stm32f4xx.h"
#include "gpio.h"
#include "LED.h"
#include "systick.h"

#include "button.h"

/********************** Статические функции ***************************************************************************/

// Включение тактирования + настройка обработки внешних прерываний
static void EXTI_InitPin(EXTI_Port EXTI_port, uint32_t EXTI_pin, uint32_t EXTI_trigger)
{
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;	// Включение тактирования

	/** Выбор конкретного регистра EXTICR(0,1,2,3)
		Пины 0,1,2,3 определяются регистром EXTICR[0]
		Пины 4,5,6,7 определяются регистром EXTICR[1]
		Пины 8,9,10,11 определяются регистром EXTICR[2]
		Пины 12,13,14,15 определяются регистром EXTICR[3]
	*/
	uint32_t EXTI_CR_INDEX = EXTI_pin / 4;

	/** Конкретный бит в регистре
		Определение порта, на котором будут обрабатываться внешние прерывания (A, B, C и т.д.)
		например порт D (0x03): вместо #define SYSCFG_EXTICR2_EXTI5_PD  ((uint16_t)0x0030) // PD[5] EXTI_pin
		пин 5: (5 % 4)*4 = 4, значит  0x03 сдвиг на 4 позиции влево, получается 0x30
	*/
	uint32_t EXTI_CR_POSITION = (EXTI_pin % 4) * 4;

    SYSCFG->EXTICR[EXTI_CR_INDEX] &= ~(0xF << EXTI_CR_POSITION);		// Начальное обнуление всех битов
    SYSCFG->EXTICR[EXTI_CR_INDEX] |= (EXTI_port << EXTI_CR_POSITION);	// Установка значения регистра EXTICR[x]

	// Начальное обнуление регистров триггеров
    EXTI->RTSR &= ~(0x1 << EXTI_pin);
    EXTI->FTSR &= ~(0x1 << EXTI_pin);

	// Установка триггера
    if(EXTI_trigger & EXTI_TRIGGER_RISING) EXTI->RTSR |= (0x1 << EXTI_pin);
    if(EXTI_trigger & EXTI_TRIGGER_FALLING) EXTI->FTSR |= (0x1 << EXTI_pin);

	//Начальное обнуление регистра ожидания
    EXTI->PR = (1 << EXTI_pin);
}

// Разрешение обработки внешних прерываний
static void EXTI_EnableIRQ(uint32_t EXTI_pin)
{
    EXTI->IMR |= (1 << EXTI_pin);

    switch(EXTI_pin) {
        case 0:
            NVIC_EnableIRQ(EXTI0_IRQn);
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

// Запрет обработки внешних прерываний
static void EXTI_DisableIRQ(uint32_t EXTI_pin)
{
    EXTI->IMR &= ~(1 << EXTI_pin);

    switch(EXTI_pin) {
        case 0:
            NVIC_DisableIRQ(EXTI0_IRQn);
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

/*********************** Глобальные функции ***************************************************************************/

// Включение обработки внешних прерываний
void EXTI_Enable_Pin(EXTI_Port EXTI_port, uint32_t EXTI_pin, uint32_t EXTI_trigger)
{
    EXTI_InitPin(EXTI_port, EXTI_pin, EXTI_trigger);
    EXTI_EnableIRQ(EXTI_pin);
}

// Выключение обработки внешних прерываний
void EXTI_Disable_Pin(EXTI_Port EXTI_port, uint32_t EXTI_pin)
{
    EXTI_InitPin(EXTI_port, EXTI_pin, 0x0);
    EXTI_DisableIRQ(EXTI_pin);
}

// Очистка флага ожидания (вызывается в обработчике прерывания)
void EXTI_Clear_Flag(uint32_t EXTI_pin)
{
    EXTI->PR = (1 << EXTI_pin);
}


// Обработчик внешнего прерывания на выводах Px0, где x => GPIOx
void EXTI0_IRQHandler(void)
{
    GPIO_set_HIGH(GPIOD, 15);								// Индикация нажатия кнопки синим светодиодом
	//delay_ms(100);
    uint32_t current_time = get_current_ms();				// Момент времени начала обработки прерывания
    if (current_time - button_last_time > DEBOUNCE_TIME)
    {
        LED_change_blink_mode(LED_Set_Blink_Period);		// Логика нажатия на кнопку - смена режима
		button_last_time = get_current_ms();				// Обновить время последнего нажатия
    }
    EXTI_Clear_Flag(0);										// Сброс флага для выхода из прерывания
    LED_turnOFF_4_LED();									// Выключение всех светодиодов
}
