/**
  * @file    7_segment_indicator.c
  * @brief   Файл содержит реализации функций 7-сегментного индикатора.
  */

/** Includes **********************************************************************************************************/

#include <stdio.h>
#include <string.h>
#include "gpio.h"
#include "7_segment_indicator.h"
#include "systick.h"

/** Макросы для включения отдельных сегментов *************************************************************************/

#define Turn_On_Segment_A()		(GPIO_set_HIGH(segmentA_Port, segmentA_Pin))
#define Turn_On_Segment_B()		(GPIO_set_HIGH(segmentB_Port, segmentB_Pin))
#define Turn_On_Segment_C()		(GPIO_set_HIGH(segmentC_Port, segmentC_Pin))
#define Turn_On_Segment_D()		(GPIO_set_HIGH(segmentD_Port, segmentD_Pin))
#define Turn_On_Segment_E()		(GPIO_set_HIGH(segmentE_Port, segmentE_Pin))
#define Turn_On_Segment_F()		(GPIO_set_HIGH(segmentF_Port, segmentF_Pin))
#define Turn_On_Segment_G()		(GPIO_set_HIGH(segmentG_Port, segmentG_Pin))

/** Переменные ********************************************************************************************************/

uint32_t Last_Refresh_Time;						// Момент последнего обновления отображения индикатора
uint8_t Current_Position = 0;					// Текущий отображаемый разряд (0, 1 или 2)
char Display[CTRL_PINS_NUMBER] = { 0 };			// Текущие символы для отображения

uint32_t Last_Offset_String;		// Момент последнего сдвига строки влево на 1 символ
uint32_t Current_String_Index = 0;	// Текущая позиция в отображаемой строке

/** Статические функции ***********************************************************************************************/

// Выключение всех разрядов индикатора
static void Turn_Off_All_Positions()
{
	// Массив структур управляющих выводов позволяет перебрать все выводы в цикле
	for (uint8_t i = 0; i < CTRL_PINS_NUMBER; i++)
	{
		GPIO_set_HIGH(Ctrl_Pins_Array[i].GPIO_Port, Ctrl_Pins_Array[i].GPIO_Pin);
	}
}

// Выключение всех сегментов индикатора
static void Turn_Off_ALL_Segments()
{
	// Количество сегментов увеличиваться не может, массив структур не нужен
	GPIO_set_LOW(segmentA_Port, segmentA_Pin);
	GPIO_set_LOW(segmentB_Port, segmentB_Pin);
	GPIO_set_LOW(segmentC_Port, segmentC_Pin);
	GPIO_set_LOW(segmentD_Port, segmentD_Pin);
	GPIO_set_LOW(segmentE_Port, segmentE_Pin);
	GPIO_set_LOW(segmentF_Port, segmentF_Pin);
	GPIO_set_LOW(segmentG_Port, segmentG_Pin);
}

// Включение нужного разряда
static void Set_Active_Position(int position)
{
	// Количество разрядов ограничено в файле 7_segment_indicator.h
	position %= CTRL_PINS_NUMBER;

	// Выключить все разряды
	Turn_Off_All_Positions();

	// Включить нужный разряд
	GPIO_set_LOW(Ctrl_Pins_Array[position].GPIO_Port, Ctrl_Pins_Array[position].GPIO_Pin);
}

// Отображение конкретного символа в конкретном разряде
static void Seven_Segment_Indicate_Symbol(char Character, int position)
{
	char mask = Digit_Masks[37];	// Маска символа для отображения, если Character не соответствует ни одной маске, будет отображён пробел

	// Выбор маски для символа
	if ((Character >= 48) && (Character <= 57)) mask = Digit_Masks[Character - 48];   	// Цифры
	if (Character == 45) mask = Digit_Masks[10];                                      	// Знак -
	if ((Character >= 65) && (Character <= 90)) mask = Digit_Masks[Character - 54];   	// Большие буквы
	if ((Character >= 97) && (Character <= 122)) mask = Digit_Masks[Character - 86];  	// Маленькие буквы
	if (Character == 32) mask = Digit_Masks[37];										// Пробел

	Set_Active_Position(position);	// Включение нужного разряда для отображения
	Turn_Off_ALL_Segments();		// Выключение сегментов

	// Проверка маски символа (нужно ли включить конкретный сегмент)
	if (mask & (1 << segment_A)) Turn_On_Segment_A();
	if (mask & (1 << segment_B)) Turn_On_Segment_B();
	if (mask & (1 << segment_C)) Turn_On_Segment_C();
	if (mask & (1 << segment_D)) Turn_On_Segment_D();
	if (mask & (1 << segment_E)) Turn_On_Segment_E();
	if (mask & (1 << segment_F)) Turn_On_Segment_F();
	if (mask & (1 << segment_G)) Turn_On_Segment_G();
}

/** Публичные функции *************************************************************************************************/

// Отображение числа
void Seven_Segment_Indicate_Number(int number)
{
	char str[20];
	snprintf(str, sizeof(str), "%d", number);
	Seven_Segment_Indicate_String(str);
}

// Отображение строки
void Seven_Segment_Indicate_String(char* str)
{
	// Если длина строки не превышает количество разрядов индикатора, то бегущая строка не нужна
	if (strlen(str) <= CTRL_PINS_NUMBER) Current_String_Index = 0;

	// Определение символов для отображения одновременно
	for (int i = 0; i < 3; i++)
	{
		Display[i] = *(str + Current_String_Index + i);
	}

	// Каждые Refresh_Period мс отображается 1 символ
	if (is_time_passed_ms(Last_Refresh_Time, Refresh_Period))
	{
		Seven_Segment_Indicate_Symbol(Display[Current_Position], Current_Position);	// Отображение одного символа в текущей позиции
		Current_Position++;															// Сдвиг текущей позиции
		if (Current_Position >= CTRL_PINS_NUMBER) Current_Position = 0;				// Текущая позиция ограничена количеством разрядов индикатора
		Last_Refresh_Time = get_current_ms();										// Сохранение момента отображения символа
	}

	// Каждые Running_Str_Period мс бегущая строка сдвигается на 1 символ
	if (is_time_passed_ms(Last_Offset_String, Running_Str_Period))
	{
		Current_String_Index++;
		Last_Offset_String = get_current_ms();
	}

	// Если строка достигла конца => отображение с начала
	if (Current_String_Index == strlen(str)) Current_String_Index = 0;
}
