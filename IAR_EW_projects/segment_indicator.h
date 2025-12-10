// прототипы функций отображения цифр + маски для каждой цифры
/** Пины для выбора цифры (для выбора конкретной цифры на соответствующем пине должен быть низкий уровень, на остальных высокий)
разряд 0		PD6
разряд 1		PD4
разряд 2		PD2

Например для отображения крайнего левого символа (разряд 0) PD6 должен быть LOW, PD4 и PD2 - HIGH
*/

/** Пины для включения конкретного разряда (установить LOW чтобы включился нужный разряд) */

#define position_0_Port		GPIOD
#define position_0_Pin		6
#define position_1_Port		GPIOD
#define position_1_Pin		4
#define position_2_Port		GPIOD
#define position_2_Pin		2

/** Пины для включения конкретного сегмента цифры (установить HIGH чтобы включился нужный сегмент) */

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

// Сегмент E	PA9
#define segmentE_Port	GPIOA
#define segmentE_Pin	9

// Сегмент F	PC9
#define segmentF_Port	GPIOC
#define segmentF_Pin	9

// Сегмент G	PC7
#define segmentG_Port	GPIOC
#define segmentG_Pin	7


/** Набор сегментов для конкретной цифры/знака
1	BC
2	ABDEF
3	ABCDE
4	BCDG
5	ACDEG
6	ACDEFG
7	ABC
8	ABCDEFG
9	ABCDEG
0	ABCEFG
-	D
*/

/** Алгоритм отображения 3-значного числа
В цикле:
отобразить 0 цифру;
задержка 1мс;
отобразить 1 цифру;
задержка 1мс;
отобразить 2 цифру;
задержка 1мс;
*/