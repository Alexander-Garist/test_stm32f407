/**
  * @file    7_segment_indicator.h
  * @brief   Файл содержит выводы GPIO (управляющие выводы для выбора нужного разряда и выводы для включения отдельных
				сегментов индикатора), к которым подключен 7-сегментный индикатор с общим коллектором (для включения
				разряда нужно подать низкий уровень на соответствующий управляющий вывод), перечисление сегментов и
				прототипы функций индикатора (отображение строки и числа).
  */

/** Define to prevent recursive inclusion *****************************************************************************/
#ifndef __7_SEGMENT_INDICATOR_H__
#define __7_SEGMENT_INDICATOR_H__

// Количество разрядов индикатора (количество управляющих выводов)
#define CTRL_PINS_NUMBER	3

/******* Управляющие выводы для включения конкретного разряда (установить LOW чтобы включился нужный разряд). *********/
/******* Для увеличения количества управляющих выводов нужно:
			1. Установить правильное значение CTRL_PINS_NUMBER
			2. Определить GPIO порты и пины всех управляющих выводов через #define
			3. Добавить все новые управляющие выводы в массив структур Ctrl_Pins_Array
*/
#define CTRL_0_PORT		GPIOC
#define CTRL_0_PIN		15
#define CTRL_1_PORT		GPIOC
#define CTRL_1_PIN		14
#define CTRL_2_PORT		GPIOE
#define CTRL_2_PIN		6

// Раскомментировать для добавления портов управления
/*
#define CTRL_3_Port		GPIOx
#define CTRL_3_Pin		pinx
*/

// Структура управляющего вывода
typedef struct
{
	GPIO_TypeDef*	GPIO_Port;
	uint8_t			GPIO_Pin;
}Control_Pins_t;

// Массив структур управляющих выводов (нужен для удобного перебора с помощью цикла всех управляющих выводов индикатора)
static Control_Pins_t Ctrl_Pins_Array[CTRL_PINS_NUMBER] = {
	{CTRL_0_PORT, CTRL_0_PIN},
	{CTRL_1_PORT, CTRL_1_PIN},
	{CTRL_2_PORT, CTRL_2_PIN}
	// {CTRL_3_Port, CTRL_3_Pin} Раскомментировать для добавления портов управления
};

/********* Выводы для включения конкретного сегмента (установить HIGH чтобы включился нужный сегмент). ****************/
#define SEGMENT_A_PORT	GPIOE		// Сегмент A	PE4
#define SEGMENT_A_PIN	4
#define SEGMENT_B_PORT	GPIOE		// Сегмент B	PE2
#define SEGMENT_B_PIN	2
#define SEGMENT_C_PORT	GPIOE		// Сегмент C	PE0
#define SEGMENT_C_PIN	0
#define SEGMENT_D_PORT	GPIOC		// Сегмент D	PC13
#define SEGMENT_D_PIN	13
#define SEGMENT_E_PORT	GPIOE		// Сегмент E	PE5
#define SEGMENT_E_PIN	5
#define SEGMENT_F_PORT	GPIOE		// Сегмент F	PE3
#define SEGMENT_F_PIN	3
#define SEGMENT_G_PORT	GPIOE		// Сегмент G	PE1
#define SEGMENT_G_PIN	1

#define REFRESH_PERIOD					10		// Максимальный период обновления индикации (мс)
#define RUNNING_STR_PERIOD				1000		// Период сдвига бегущей строки (мс)

/******************************************** Переменные **************************************************************/

extern uint32_t Last_Refresh_Time;		// Время последнего обновления показаний индикатора

/****************** Перечисление сегментов для проверки бита в позиции, соответсвующей нужному сегменту ***************/

typedef enum
{
	segment_A = 6,
    segment_B = 5,
    segment_C = 4,
    segment_D = 3,
    segment_E = 2,
    segment_F = 1,
    segment_G = 0
}Segments_t;

// Массив масок символов
static char Digit_Masks[] = {																													// Маска	Символ	Индекс
	(1 << segment_A) | (1 << segment_B) | (1 << segment_C) | 					(1 << segment_E) | (1 << segment_F) | (1 << segment_G),			// ABCEFG	0		0
					   (1 << segment_B) | (1 << segment_C),																						// BC		1		1
	(1 << segment_A) | (1 << segment_B) | 					 (1 << segment_D) | (1 << segment_E) | (1 << segment_F),							// ABDEF	2		2
	(1 << segment_A) | (1 << segment_B) | (1 << segment_C) | (1 << segment_D) | (1 << segment_E),												// ABCDE	3		3
					   (1 << segment_B) | (1 << segment_C) | (1 << segment_D) |										  (1 << segment_G), 		// BCDG		4		4
	(1 << segment_A) | 					  (1 << segment_C) | (1 << segment_D) | (1 << segment_E) |					  (1 << segment_G), 		// ACDEG	5		5
	(1 << segment_A) |					  (1 << segment_C) | (1 << segment_D) | (1 << segment_E) | (1 << segment_F) | (1 << segment_G), 		// ACDEFG	6		6
	(1 << segment_A) | (1 << segment_B) | (1 << segment_C),																						// ABC		7		7
	(1 << segment_A) | (1 << segment_B) | (1 << segment_C) | (1 << segment_D) | (1 << segment_E) | (1 << segment_F) | (1 << segment_G), 		// ABCDEFG	8		8
	(1 << segment_A) | (1 << segment_B) | (1 << segment_C) | (1 << segment_D) | (1 << segment_E) 					| (1 << segment_G),			// ABCDEG	9		9
															 (1 << segment_D),																	// D		-		10
	(1 << segment_A) | (1 << segment_B) | (1 << segment_C) | (1 << segment_D) | 				   (1 << segment_F) | (1 << segment_G),			// ABCDFG	A		11
										  (1 << segment_C) | (1 << segment_D) | (1 << segment_E) | (1 << segment_F) | (1 << segment_G),			// CDEFG	b		12
	(1 << segment_A) | 															(1 << segment_E) | (1 << segment_F) | (1 << segment_G), 		// AEFG		C		13
					   (1 << segment_B) | (1 << segment_C) | (1 << segment_D) | (1 << segment_E) | (1 << segment_F),							// BCDEF	d		14
	(1 << segment_A) |										 (1 << segment_D) | (1 << segment_E) | (1 << segment_F) | (1 << segment_G),			// ADEFG	E		15
	(1 << segment_A) |										 (1 << segment_D) |					   (1 << segment_F) | (1 << segment_G),			// ADFG		F		16
	(1 << segment_A) |					  (1 << segment_C) |					(1 << segment_E) | (1 << segment_F) | (1 << segment_G),			// ACEFG	G		17
					   (1 << segment_B) | (1 << segment_C) | (1 << segment_D) |					   (1 << segment_F) | (1 << segment_G),			// BCDFG	H		18
																								   (1 << segment_F) | (1 << segment_G),			// FG		I		19
					   (1 << segment_B) | (1 << segment_C) | 					(1 << segment_E),												// BCE		J		20
	0x00,																																		// 			K		21
																				(1 << segment_E) | (1 << segment_F) | (1 << segment_G),			// EFG		L		22
	0x00,																																		// 			M		23
										  (1 << segment_C) | (1 << segment_D) |					   (1 << segment_F),							// CDF		n		24
	(1 << segment_A) | (1 << segment_B) | (1 << segment_C) |					(1 << segment_E) | (1 << segment_F) | (1 << segment_G),			// ABCEFG	O		25
	(1 << segment_A) | (1 << segment_B) |					 (1 << segment_D) |					   (1 << segment_F) | (1 << segment_G),			// ABDFG	P		26
	(1 << segment_A) | (1 << segment_B) | (1 << segment_C) | (1 << segment_D)										| (1 << segment_G),			// ABCDG	q		27
															 (1 << segment_D) |					   (1 << segment_F),							// DF		r		28
	(1 << segment_A) |					  (1 << segment_C) | (1 << segment_D) | (1 << segment_E) |					  (1 << segment_G),			// ACDEG	S		29
															 (1 << segment_D) | (1 << segment_E) | (1 << segment_F) | (1 << segment_G),			// DEFG		t		30
					   (1 << segment_B) | (1 << segment_C) |					(1 << segment_E) | (1 << segment_F) | (1 << segment_G),			// BCEFG	U		31
										  (1 << segment_C) |					(1 << segment_E) | (1 << segment_F),							// CEF		v		32
	0x00,																																		//			w		33
	0x00,																																		// 			x		34
					   (1 << segment_B) | (1 << segment_C) | (1 << segment_D) | (1 << segment_E) |					  (1 << segment_G),			// BCDEG	Y		35
	0x00,																																		// 			z		36
	0x00																																		// 			space  	37
};

/******************************************** Функции *****************************************************************/

	/**
	! Функция отображения строки.
	- str - указатель на массив символов, который нужно отобразить на индикаторе.
	*/
void Seven_Segment_Indicate_String(char* str);

	/**
	! Функция отображения числа.
	- Number - число, которое нужно отобразить на индикаторе.
	*/
void Seven_Segment_Indicate_Number(int number);

#endif /* __7_SEGMENT_INDICATOR_H__ */