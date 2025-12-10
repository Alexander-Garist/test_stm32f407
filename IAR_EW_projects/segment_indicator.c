// .c файл семисегментного индикатора, реализация функций

#include "gpio.h"
#include "segment_indicator.h"
#include "systick.h"

// Выключение всех разрядов
static void turn_off_all_positions()
{
	GPIO_set_HIGH(position_0_Port, position_0_Pin);	// отключение отображения 0 разряда
	GPIO_set_HIGH(position_1_Port, position_1_Pin);	// отключение отображения 1 разряда
	GPIO_set_HIGH(position_2_Port, position_2_Pin);	// отключение отображения 2 разряда
}

/********** Включение конкретной цифры ******************/
// Сегменты BC
static void Indicate_Symbol_1()
{
	GPIO_set_HIGH(segmentB_Port, segmentB_Pin);	// Сегмент B
	GPIO_set_HIGH(segmentC_Port, segmentC_Pin);	// Сегмент C
}

// Сегменты ABDFE
static void Indicate_Symbol_2()
{
	GPIO_set_HIGH(segmentA_Port, segmentA_Pin);	// Сегмент A
	GPIO_set_HIGH(segmentB_Port, segmentB_Pin);	// Сегмент B
	GPIO_set_HIGH(segmentD_Port, segmentD_Pin);	// Сегмент D
	GPIO_set_HIGH(segmentF_Port, segmentF_Pin);	// Сегмент F
	GPIO_set_HIGH(segmentE_Port, segmentE_Pin);	// Сегмент E
}

// Сегменты ABCDE
static void Indicate_Symbol_3()
{
	GPIO_set_HIGH(segmentA_Port, segmentA_Pin);	// Сегмент A
	GPIO_set_HIGH(segmentB_Port, segmentB_Pin);	// Сегмент B
	GPIO_set_HIGH(segmentC_Port, segmentC_Pin);	// Сегмент C
	GPIO_set_HIGH(segmentD_Port, segmentD_Pin);	// Сегмент D
	GPIO_set_HIGH(segmentE_Port, segmentE_Pin);	// Сегмент E
}

// Сегменты BCDG
static void Indicate_Symbol_4()
{
	GPIO_set_HIGH(segmentB_Port, segmentB_Pin);	// Сегмент B
	GPIO_set_HIGH(segmentC_Port, segmentC_Pin);	// Сегмент C
	GPIO_set_HIGH(segmentD_Port, segmentD_Pin);	// Сегмент D
	GPIO_set_HIGH(segmentG_Port, segmentG_Pin);	// Сегмент G
}

// Сегменты ACDEG
static void Indicate_Symbol_5()
{
	GPIO_set_HIGH(segmentA_Port, segmentA_Pin);	// Сегмент A
	GPIO_set_HIGH(segmentC_Port, segmentC_Pin);	// Сегмент C
	GPIO_set_HIGH(segmentD_Port, segmentD_Pin);	// Сегмент D
	GPIO_set_HIGH(segmentE_Port, segmentE_Pin);	// Сегмент E
	GPIO_set_HIGH(segmentG_Port, segmentG_Pin);	// Сегмент G
}

// Сегменты ACDEFG
static void Indicate_Symbol_6()
{
	GPIO_set_HIGH(segmentA_Port, segmentA_Pin);	// Сегмент A
	GPIO_set_HIGH(segmentC_Port, segmentC_Pin);	// Сегмент C
	GPIO_set_HIGH(segmentD_Port, segmentD_Pin);	// Сегмент D
	GPIO_set_HIGH(segmentE_Port, segmentE_Pin);	// Сегмент E
	GPIO_set_HIGH(segmentF_Port, segmentF_Pin);	// Сегмент F
	GPIO_set_HIGH(segmentG_Port, segmentG_Pin);	// Сегмент G
}

// Сегменты ABC
static void Indicate_Symbol_7()
{
	GPIO_set_HIGH(segmentA_Port, segmentA_Pin);	// Сегмент A
	GPIO_set_HIGH(segmentB_Port, segmentB_Pin);	// Сегмент B
	GPIO_set_HIGH(segmentC_Port, segmentC_Pin);	// Сегмент C
}

// Сегменты ABCDEFG
static void Indicate_Symbol_8()
{
	GPIO_set_HIGH(segmentA_Port, segmentA_Pin);	// Сегмент A
	GPIO_set_HIGH(segmentB_Port, segmentB_Pin);	// Сегмент B
	GPIO_set_HIGH(segmentC_Port, segmentC_Pin);	// Сегмент C
	GPIO_set_HIGH(segmentD_Port, segmentD_Pin);	// Сегмент D
	GPIO_set_HIGH(segmentE_Port, segmentE_Pin);	// Сегмент E
	GPIO_set_HIGH(segmentF_Port, segmentF_Pin);	// Сегмент F
	GPIO_set_HIGH(segmentG_Port, segmentG_Pin);	// Сегмент G
}

// Сегменты ABCDEG
static void Indicate_Symbol_9()
{
	GPIO_set_HIGH(segmentA_Port, segmentA_Pin);	// Сегмент A
	GPIO_set_HIGH(segmentB_Port, segmentB_Pin);	// Сегмент B
	GPIO_set_HIGH(segmentC_Port, segmentC_Pin);	// Сегмент C
	GPIO_set_HIGH(segmentD_Port, segmentD_Pin);	// Сегмент D
	GPIO_set_HIGH(segmentE_Port, segmentE_Pin);	// Сегмент E
	GPIO_set_HIGH(segmentG_Port, segmentG_Pin);	// Сегмент G
}

// Сегменты ABCEFG
static void Indicate_Symbol_0()
{
	GPIO_set_HIGH(segmentA_Port, segmentA_Pin);	// Сегмент A
	GPIO_set_HIGH(segmentB_Port, segmentB_Pin);	// Сегмент B
	GPIO_set_HIGH(segmentC_Port, segmentC_Pin);	// Сегмент C
	GPIO_set_HIGH(segmentE_Port, segmentE_Pin);	// Сегмент E
	GPIO_set_HIGH(segmentF_Port, segmentF_Pin);	// Сегмент F
	GPIO_set_HIGH(segmentG_Port, segmentG_Pin);	// Сегмент G
}

// Сегменты D
static void Indicate_Symbol_Minus()
{
	GPIO_set_HIGH(segmentD_Port, segmentD_Pin);	// Сегмент D
}


// ф-я для отображения конкретного символа в конкретном разряде
// логика:
//		отключить все разряды (установить все в HIGH состояние => светодиоды не светятся нигде)
//		включить только нужный разряд (установить соответствующий пин разряда в LOW)
//		включить сегменты, соответствующие символу symbol
static void Indicate_Digit(char symbol, int position)
{
	// Выключить все разряды
	turn_off_all_positions();

	// Выбор нужного разряда
	switch (position)
	{
		case 0:
		GPIO_set_LOW(GPIOD, 6);
		break;

		case 1:
		GPIO_set_LOW(GPIOD, 4);
		break;

		case 2:
		GPIO_set_LOW(GPIOD, 2);
		break;
	}

	// Символ в выбранном разряде
	switch (symbol)
	{
		case '0':
		Indicate_Symbol_0(); break;

		case '1':
		Indicate_Symbol_1(); break;

		case '2':
		Indicate_Symbol_2(); break;

		case '3':
		Indicate_Symbol_3(); break;

		case '4':
		Indicate_Symbol_4(); break;

		case '5':
		Indicate_Symbol_5(); break;

		case '6':
		Indicate_Symbol_6(); break;

		case '7':
		Indicate_Symbol_7(); break;

		case '8':
		Indicate_Symbol_8(); break;

		case '9':
		Indicate_Symbol_9(); break;

		case ('-'):
		Indicate_Symbol_Minus(); break;
	}
}

// Ф-я отображения трехзначного числа
void Indicate_Number(int number)
{
	// На входе число - преобразовать в последовательность символов, символы вывести на нужных разрядах
	char digits[3];				// 123
	// оптимизировать
	digits[2] = number % 10;	// 3
	number /= 10;				// 12
	digits[1] = number % 10;	// 2
	number /= 10;				// 1
	digits[1] = number % 10;	// 1

	while(1)
	{
		// разряд i - цифра[i]
		Indicate_Digit(digits[0], 0);
		delay_ms(1);
		Indicate_Digit(digits[1], 1);
		delay_ms(1);
		Indicate_Digit(digits[2], 2);
		delay_ms(1);

	}

}











