/**
  * @file    exti.c
  * @brief   Файл содержит реализации функций EXTI
  */

/** Includes **********************************************************************************************************/
#include <stdio.h>
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


uint32_t Interrupt_EXTI0_Occured = 0;
uint32_t Last_Interrupt_Time_ms = 0;	// момент последнего вызова прерывания



// Обработчик внешнего прерывания на выводах Px0, где x => GPIOx
void EXTI0_IRQHandler(void)
{
	// Прерывание вызывается и при нажатии, и при отпускании кнопки из-за дребезга контактов кнопки

	uint32_t current_time_ms = get_current_ms();

	// короткое нажатие длится 70-120 мс, значит отпускание тоже успеет вызвать свое прерывание
	if (is_time_passed_ms(Last_Interrupt_Time_ms, 50))
	{
		Last_Interrupt_Time_ms = current_time_ms;
		Interrupt_EXTI0_Occured = 1;	// Поднимается флаг "прерывание вызвано"
	}

	EXTI_Clear_Flag(BUTTON_GPIO_PIN);


//========================================================================================================================================================
/*	// Рабочая версия прерывания, все нажатия корректно обрабатываются, дребезга и ошибочных нажатий нет, но длительность прерывания 250 мс

	// Вход в критическую секцию => новое прерывание не должно вызываться чтобы не сбить логику двойного нажатия
	EXTI_Disable_Pin(BUTTON_EXTI_PORT, BUTTON_GPIO_PIN);

	// Моменты нажатия и отпускания кнопки
	uint32_t pressing_time = get_current_ms();	// Прерывание вызвано при нажатии
	uint32_t releasing_time;					// Кнопка еще может быть не отпущена

	uint32_t interrupt_start_ms = get_current_ms();

	// Ожидание пока кнопка нажата, чтобы определить длинное нажатие
	// длится 80 мс
	while (GPIO_Read_Pin(BUTTON_GPIO_PORT, BUTTON_GPIO_PIN))
	{
		delay_ms(1);
	}

	uint32_t interrupt_duration = get_current_ms();
	interrupt_duration -= interrupt_start_ms;

	//printf("%d\n", interrupt_duration);

	// Цикл завершается когда кнопку отпустили, этот момент запоминается
	releasing_time = get_current_ms();

	// Проверка продолжительности нажатия, в случае длинного нажатия больше проверок нет
	if (is_time_passed_ms(pressing_time, LONG_PRESS_TIME))
	{
		Button_State = SINGLE_LONG_PRESS;			// Зафиксировано одно длинное нажатие
		Button_Last_Press_Time_ms = get_current_ms();	// Обновление времени последнего нажатия
	}
	// В случае короткого еще может быть двойное нажатие
	else
	{
		// Проверка продолжительности нажатия (антидребезг)
		if (releasing_time - Button_Last_Press_Time_ms >= DEBOUNCE_TIME)
		{
			LED_change_blink_mode(LED_Set_Blink_Period);	// Логика одиночного нажатия на кнопку - смена режима моргания
			Button_State = SINGLE_SHORT_PRESS;				// Зафиксировано одно короткое нажатие
			Button_Last_Press_Time_ms = get_current_ms();		// Обновить время последнего нажатия
		}

		// Ожидание второго короткого нажатия
		while (!is_time_passed_ms(Button_Last_Press_Time_ms, RELEASED_TIME))
		{
			if (GPIO_Read_Pin(BUTTON_GPIO_PORT, BUTTON_GPIO_PIN))
			{
				releasing_time = get_current_ms();
			}
			if (releasing_time - Button_Last_Press_Time_ms >= DEBOUNCE_TIME)
			{
				Button_State = DOUBLE_PRESS;
			}
		}
	}

	// Сброс флага для выхода из прерывания
	EXTI_Clear_Flag(0);

	//printf("%d\n", interrupt_duration); // длительнось прерывания 250+ мс

	// Выход из критической секции => включение обработки прерываний
	EXTI_Enable_Pin(BUTTON_EXTI_PORT, BUTTON_GPIO_PIN, BUTTON_TRIGGER);
*/
//========================================================================================================================================================

}









