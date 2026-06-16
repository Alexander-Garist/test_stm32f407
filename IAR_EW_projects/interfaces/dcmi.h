#ifndef __DCMI_H__
#define __DCMI_H__

/**
Пин камеры     Провод             Пин платы STM32
1 VCC 		-> красный		->      3.3V
2 GND 		-> коричневый	->      GND
3 SCL 		-> белый		->      I2C2_SCL		(PB10 AF4)
4 SDA 		-> серый		->      I2C2_SDA		(PB11 AF4)
5 VSYNC 	-> черный		->      DCMI_VSYNC	(PB7 AF13)
6 HREF 		-> фиолетовый	->      DCMI_HSYNC	(PA4 AF13)
7 PCLK 		-> синий		->      DCMI_PIXCLK	(PA6 AF13)
8 MCLK 		-> НЕ ПОДКЛЮЧЕН
9 D7 		-> черный		->      DCMI_D7		(PB9 AF13)
10 D6 		-> белый		->      DCMI_D6		(PB8 AF13)
11 D5 		-> серый		->      DCMI_D5		(PB6 AF13)
12 D4 		-> коричневый	->      DCMI_D4		(PC11 AF13)
13 D3 		-> оранжевый	->      DCMI_D3		(PC9 AF13)
14 D2 		-> красный		->      DCMI_D2		(PC8 AF13)
15 D1 		-> зеленый		->      DCMI_D1		(PA10 AF13)
16 D0 		-> желтый		->      DCMI_D0		(PC6 AF13)
17 RESET 	-> оранжевый	->      GPIO output (PB1)
18 PWDN 	-> фиолетовый	->      GPIO output (PB0)



Данные упаковываются в 32-битное слово в регистре DCMI_DR.
Запрос DMA генерируется каждый раз когда DCMI_DR получает полное 32-битное слово.
DMA активируется когда DCMI_CR.CAPTURE == 1

*/

#include "stm32f4xx.h"

#define CAM_WIDTH        160
#define CAM_HEIGHT       120
#define CAM_FRAME_BYTES  (CAM_WIDTH * CAM_HEIGHT)

// Прототипы функций
void DCMI_Init(void);
void DCMI_StartCapture(uint32_t *buffer);
uint8_t DCMI_IsFrameReady(void);
void DCMI_ClearFrameStatus(void);


#endif /* __DCMI_H__ */
