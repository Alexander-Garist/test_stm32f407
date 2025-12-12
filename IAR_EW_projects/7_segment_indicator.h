/**
  * @file    7_segment_indicator.h
  * @brief   Файл содержит пины GPIO, к которым подключен 7-сегментный индикатор с общим коллектором (для включения
				разряда нужно подать низкий уровень на соответствующий управляющий пин), перечисление сегментов и
				прототипы функций индикатора.
  */

/** Define to prevent recursive inclusion *****************************************************************************/
#ifndef __7_SEGMENT_INDICATOR_H__
#define __7_SEGMENT_INDICATOR_H__

/** Управляющие пины для включения конкретного разряда (установить LOW чтобы включился нужный разряд). На данный момент
	их 3 (для 3-разрядного индикатора, для работы с 4-разрядным индикатором нужно добавить еще один управляющий пин) */
#define position_0_Port		GPIOD
#define position_0_Pin		6
#define position_1_Port		GPIOD
#define position_1_Pin		4
#define position_2_Port		GPIOD
#define position_2_Pin		2

// Для 4-разрядного индикатора добавить еще один управляющий вывод:
// #define position_3_Port		GPIOx
// #define position_3_Pin		pin

/** Пины для включения конкретного сегмента (установить HIGH чтобы включился нужный сегмент). */

// Сегмент A	PD0
#define segmentA_Port	GPIOD
#define segmentA_Pin	0

// Сегмент B	PC11
#define segmentB_Port	GPIOC
#define segmentB_Pin	11

// Сегмент C	PA15
#define segmentC_Port	GPIOA
#define segmentC_Pin	15

// Сегмент D	PA13
#define segmentD_Port	GPIOA
#define segmentD_Pin	13

// Сегмент E	PA8
#define segmentE_Port	GPIOA
#define segmentE_Pin	8

// Сегмент F	PC9
#define segmentF_Port	GPIOC
#define segmentF_Pin	9

// Сегмент G	PC7
#define segmentG_Port	GPIOC
#define segmentG_Pin	7

/** Перечисление сегментов для проверки бита в позиции, соответсвующей нужному сегменту */
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


	/**
	! Функция отображения символа в конкретном разряде.
	- Mask_Index - номер маски, соответствующей данному символу. Массив масок находится в файле 7_segment_indicator.c
	- position - разряд индикатора, в котором будет отображаться символ.
	*/
void Seven_Segment_Indicate_Symbol(char Character, int position);



void Seven_Segment_Indicate_Number(int number);

#endif /* __7_SEGMENT_INDICATOR_H__ */