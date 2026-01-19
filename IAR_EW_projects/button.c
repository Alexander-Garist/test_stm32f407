/**
  * @file    button.c
  * @brief   Файл содержит реализации функций кнопки.
  */

/** Includes **********************************************************************************************************/

#include "button.h"
#include "gpio.h"
#include "systick.h"
#include "exti.h"
#include "LED.h"

uint8_t Button_State = RELEASED;		// Состояние кнопки (изначально не нажата)
uint32_t Button_Last_Press_Time = 0;	// Время последнего нажатия

// В обработчиках нажатий должно переключаться состояние кнопки в RELEASED после выполнения логики нажатия

// Обработчик одиночного короткого нажатия
void Button_Single_Short_Press_Handler()
{
	GPIO_toggle_Pin(GPIOD, 12);
	Button_State = RELEASED;
}

// Обработчик одиночного длинного нажатия
void Button_Single_Long_Press_Handler()
{
	GPIO_toggle_Pin(GPIOD, 13);
	Button_State = RELEASED;
}

// Обработчик двойного нажатия
void Button_Double_Press_Handler()
{
	GPIO_toggle_Pin(GPIOD, 14);
	Button_State = RELEASED;
}

// Обработчик внешнего прерывания, вызываемого нажатием кнопки
void Button_IRQHandler(void)
{
	// Вход в критическую секцию => новое прерывание не должно вызываться чтобы не сбить логику двойного нажатия
	EXTI_Disable_Pin(BUTTON_EXTI_PORT, BUTTON_GPIO_PIN);

	// Моменты нажатия и отпускания кнопки
    uint32_t pressing_time = get_current_ms();	// Прерывание вызвано при нажатии
	uint32_t releasing_time;					// Кнопка еще может быть не отпущена

	while (GPIO_Read_Pin(BUTTON_GPIO_PORT, BUTTON_GPIO_PIN))	// Ожидание пока кнопка нажата, чтобы определить длинное нажатие
	{
		delay_ms(1);
	}

	// Цикл завершается когда кнопку отпустили, этот момент запоминается
	releasing_time = get_current_ms();

	// Проверка продолжительности нажатия, в случае длинного нажатия больше проверок нет
	if (releasing_time - pressing_time >= LONG_PRESS_TIME)
	{
		Button_State = SINGLE_LONG_PRESS;			// Зафиксировано одно длинное нажатие
		Button_Last_Press_Time = get_current_ms();	// Обновление времени последнего нажатия
	}
	// В случае короткого еще может быть двойное нажатие
	else
	{
		// Проверка продолжительности нажатия (антидребезг)
		if (releasing_time - Button_Last_Press_Time >= DEBOUNCE_TIME)
		{
			LED_change_blink_mode(LED_Set_Blink_Period);	// Логика одиночного нажатия на кнопку - смена режима моргания
			Button_State = SINGLE_SHORT_PRESS;				// Зафиксировано одно короткое нажатие
			Button_Last_Press_Time = get_current_ms();		// Обновить время последнего нажатия
		}

		// Ожидание второго короткого нажатия
		while (get_current_ms() - Button_Last_Press_Time < RELEASED_TIME)
		{
			if (GPIO_Read_Pin(BUTTON_GPIO_PORT, BUTTON_GPIO_PIN))
			{
				releasing_time = get_current_ms();
			}
			if (releasing_time - Button_Last_Press_Time >= DEBOUNCE_TIME)
			{
				Button_State = DOUBLE_PRESS;
			}
		}
	}

	// Сброс флага для выхода из прерывания
    EXTI_Clear_Flag(0);

	// Выход из критической секции => включение обработки прерываний
	EXTI_Enable_Pin(BUTTON_EXTI_PORT, BUTTON_GPIO_PIN, BUTTON_TRIGGER);
}