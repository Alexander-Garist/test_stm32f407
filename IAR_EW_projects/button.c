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

Button_Press_t Button_State = RELEASED;		// Состояние кнопки (изначально не нажата)

uint32_t Button_Last_Press_Time_ms = 0;		// Время последнего нажатия кнопки
uint32_t Button_Last_Release_Time_ms = 0;	// Время последнего отпускания кнопки
uint32_t Button_Press_Duration_ms = 0;		// Продолжительность последнего нажатия кнопки
uint8_t button_is_pressed = 0;	// кнопка нажата
uint8_t button_is_released = 0;	// кнопка отпущена
uint32_t Short_Press_Release_ms = 0;	// запомнить момент отпускания после короткого нажатия
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
	GPIO_toggle_Pin(GPIOD, 15);
	Button_State = RELEASED;
}