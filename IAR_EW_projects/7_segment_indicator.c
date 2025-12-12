/**
  * @file    7_segment_indicator.c
  * @brief   Файл содержит реализации функций 7-сегментного индикатора.
  */

/** Includes **********************************************************************************************************/

#include "gpio.h"
#include "7_segment_indicator.h"

/** Макросы для включения отдельных сегментов *************************************************************************/

#define Turn_On_Segment_A()		(GPIO_set_HIGH(segmentA_Port, segmentA_Pin))
#define Turn_On_Segment_B()		(GPIO_set_HIGH(segmentB_Port, segmentB_Pin))
#define Turn_On_Segment_C()		(GPIO_set_HIGH(segmentC_Port, segmentC_Pin))
#define Turn_On_Segment_D()		(GPIO_set_HIGH(segmentD_Port, segmentD_Pin))
#define Turn_On_Segment_E()		(GPIO_set_HIGH(segmentE_Port, segmentE_Pin))
#define Turn_On_Segment_F()		(GPIO_set_HIGH(segmentF_Port, segmentF_Pin))
#define Turn_On_Segment_G()		(GPIO_set_HIGH(segmentG_Port, segmentG_Pin))

/** Таблица соответствия символа для отображения набору его сегментов, двоичной маске и HEX маске. Для добавления новых
	символов нужно определить какие сегменты должны быть включены и составить двоичную маску вида 0b0ABCDEFG.

Символ//Набор сегментов//	Двоичная маска//	HEX маска
0		ABCEFG				0b01110111			0x77
1		BC		 			0b00110000  		0x30
2		ABDEF	 			0b01101110			0x6E
3		ABCDE	 			0b01111100 			0x7C
4		BCDG	 			0b00111001 			0x39
5		ACDEG	 			0b01011101 			0x5D
6		ACDEFG	 			0b01011111 			0x5F
7		ABC		 			0b01110000 			0x70
8		ABCDEFG	 			0b01111111 			0x7F
9		ABCDEG	 			0b01111101 			0x7D
-		D		 			0b00001000 			0x08
A		ABCDFG				0b01111011			0x7B
b		CDEFG				0b00011111			0x1F
C		AEFG				0b01000111			0x47
d		BCDEF				0b00111110			0x3E
E		ADEFG				0b01001111			0x4F
F		ADFG				0b01001011			0x4B
G		ACEFG				0b01010111			0x57
H		BCDFG				0b00111011			0x3B
I		FG					0b00000011			0x03
J		BCE					0b00110100			0x34
K												0x00	//заглушка чтобы индекс не сбивался
L		EFG					0b00000111			0x07
M												0x00	//заглушка чтобы индекс не сбивался
n		CDF					0b00011010			0x1A
o		CDEF				0b00011110			0x1E
P		ABDFG				0b01101011			0x6B
q		ABCDG				0b01111001			0x79
r		DF					0b00001010			0x0A
S		ACDEG				0b01011101			0x5D
t		DEFG				0b00001111			0x0F
U		BCEFG				0b00110111			0x37
v		CEF					0b00010110			0x16
w												0x00	//заглушка чтобы индекс не сбивался
x												0x00	//заглушка чтобы индекс не сбивался
Y		BCDEG				0b00111101			0x3D
Z												0x00	//заглушка чтобы индекс не сбивался
*/

// Набор HEX масок цифр, знака "-" и букв
char Digit_Masks[] = {
	0x77, 0x30, 0x6E, 0x7C, 0x39, 0x5D, 0x5F, 0x70, 0x7F, 0x7D,	// Цифры 0-9 индексы 0-9
	0x08,	// minus	знак	индекс 10
	0x7B,	// A		буквы	индекс 11 - 36
	0x1F,	// b
	0x47, 	// C
	0x3E, 	// d
	0x4F, 	// E
	0x4B, 	// F
	0x57, 	// G
	0x3B, 	// H
	0x03, 	// I
	0x34, 	// J
	0x00,	// K
	0x07,	// L
	0x00,	// M
	0x1A,	// n
	0x1E,	// o
	0x6B,	// P
	0x79,	// q
	0x0A,	// r
	0x5D,	// S
	0x0F,	// t
	0x37,	// U
	0x16,	// v
	0x00,	// w
	0x00, 	// x
	0x3D,	// Y
	0x00	// z
};

// Выключение всех разрядов
static void Turn_Off_All_Positions()
{
	GPIO_set_HIGH(position_0_Port, position_0_Pin);
	GPIO_set_HIGH(position_1_Port, position_1_Pin);
	GPIO_set_HIGH(position_2_Port, position_2_Pin);
}

// Выключение всех сегментов
static void Turn_Off_ALL_Segments()
{
	GPIO_set_LOW(segmentA_Port, segmentA_Pin);
	GPIO_set_LOW(segmentB_Port, segmentB_Pin);
	GPIO_set_LOW(segmentC_Port, segmentC_Pin);
	GPIO_set_LOW(segmentD_Port, segmentD_Pin);
	GPIO_set_LOW(segmentE_Port, segmentE_Pin);
	GPIO_set_LOW(segmentF_Port, segmentF_Pin);
	GPIO_set_LOW(segmentG_Port, segmentG_Pin);
}

void Print_Error()
{
	Turn_Off_All_Positions();
	Turn_Off_ALL_Segments();
	Seven_Segment_Indicate_Symbol('E', 0);
	Seven_Segment_Indicate_Symbol('r', 1);
	Seven_Segment_Indicate_Symbol('r', 2);
}







/** Выбор нужного разряда (включить только один из трех за раз).
		Для работы с 4-разрядным индикатором нужно добавить case 3 и определить position_3_Port, position_3_Pin
		в файле 7_segment_indicator.h */
static void Choose_Position(int position)
{
	Turn_Off_All_Positions();

	switch (position)
	{
		case 0:
		GPIO_set_LOW(position_0_Port, position_0_Pin); break;

		case 1:
		GPIO_set_LOW(position_1_Port, position_1_Pin); break;

		case 2:
		GPIO_set_LOW(position_2_Port, position_2_Pin); break;

	/*	Для 4-разрядного индикатора:
		case 3:
		GPIO_set_LOW(position_3_Port, position_3_Pin); break;
	*/
		// если ввести неправильный номер разряда, выведется "Err"
		default:
		Print_Error();
	}
}

// Отобразить нужный символ в конкретном разряде
// передать номер маски и разряд, в котором нужно отобразить символ
void Seven_Segment_Indicate_Symbol(char Character, int position)
{
	char symbol;
	if ((Character >= 48) && (Character <= 57)) symbol = Digit_Masks[Character - 48];   // digits
	if (Character == 45) symbol = Digit_Masks[10];                                      // -
	if ((Character >= 65) && (Character <= 90)) symbol = Digit_Masks[Character - 54];   // LETTERS
	if ((Character >= 97) && (Character <= 122)) symbol = Digit_Masks[Character - 86];  // letters

	Choose_Position(position);				// выбор нужного разряда

	// сброс уже включенных сегментов
	Turn_Off_ALL_Segments();

	if (symbol & (1 << segment_A)) Turn_On_Segment_A();
	if (symbol & (1 << segment_B)) Turn_On_Segment_B();
	if (symbol & (1 << segment_C)) Turn_On_Segment_C();
	if (symbol & (1 << segment_D)) Turn_On_Segment_D();
	if (symbol & (1 << segment_E)) Turn_On_Segment_E();
	if (symbol & (1 << segment_F)) Turn_On_Segment_F();
	if (symbol & (1 << segment_G)) Turn_On_Segment_G();
}

void Seven_Segment_Indicate_Number(int number)
{
    int temp = number;
    char digits[3] = {48, 48, 48};

    for (int i = 2; i >=0; i--)
    {
        digits[i] += temp % 10;
        temp /= 10;
    }
    if (temp > 0)	// если число 4-разрядное
    {
        Print_Error();	// пока что error, потом будет бегущая строка
        return;
    }

    for (int i = 0; i < 3; i++)
    {
        Seven_Segment_Indicate_Symbol(digits[i], i);
    }
}