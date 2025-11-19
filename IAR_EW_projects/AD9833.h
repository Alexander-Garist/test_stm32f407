/**
  * @file    AD9833.h
  * @brief   Файл содержит прототипы функций AD9833. По умолчанию для связи по SPI используется SPI1.
				Выводы GPIO для управления передачей по SPI STM32F407 -> AD9833 и STM32F407 -> MCP41010
				по умолчанию PA3 и PA4.
  */

/** Define to prevent recursive inclusion *****************************************************************************/
#ifndef __AD9833_H__
#define __AD9833_H__

/** Includes **********************************************************************************************************/
#include "CMSIS/stm32f4xx.h"
#include "spi.h"

/** Defines ***********************************************************************************************************/
// Сброс чипа
#define AD9833_RESET_CMD     0x0100		// 0000 0001 0000 0000

// Запись регистра частоты
#define AD9833_FREQ0_REG     0x4000		// 01[00 0000 0000 0000]	14 бит частоты
#define AD9833_FREQ1_REG     0x8000		// 10[00 0000 0000 0000]	14 бит частоы

// Запись регистра фазы
#define AD9833_PHASE0_REG    0xC000		// 1100 [0000 0000 0000]	12 бит фазы
#define AD9833_PHASE1_REG    0xE000		// 1110 [0000 0000 0000]	12 бит фазы

// Режимы низкого энергопотребления
#define AD9833_SLEEP_CMD	 0x00C0		// 0000 0000 1100 0000		Запрещено внутреннее тактирование + выключен DAC
#define AD9833_SLEEP1_CMD	 0x0080		// 0000 0000 1000 0000		Запрещено внутреннее тактирование (не действует MCLK)
#define AD9833_SLEEP12_CMD	 0x0040		// 0000 0000 0100 0000		Выключен DAC

// Выбор формы генерируемого сигнала
#define AD9833_SINE_MODE     0x0000		// 0000 0000 0000 0000  	синусоида
#define AD9833_TRIANGLE_MODE 0x0002		// 0000 0000 0000 0010  	треугольный
#define AD9833_SQUARE_MODE   0x0028		// 0000 0000 0010 1000  	прямоугольный

// MCP41010 Command Definitions
#define MCP41010_CMD_WRITE   0x1100
#define MCP41010_MAX_VALUE   256

// Pin Definitions
#define AD9833_FSY_PIN       3
#define AD9833_FSY_PORT      GPIOA
#define MCP41010_CS_PIN      4
#define MCP41010_CS_PORT     GPIOA

// External MCLK frequency from onboard oscillator (Hz)
#define MCLK_FREQUENCY 25000000

/** Function prototypes ***********************************************************************************************/

/* дописать описания прототипов функций */

void AD9833_GPIO_Init(void);
void AD9833_Module_Init(SPI_TypeDef* SPIx);
void AD9833_SetFrequency(SPI_TypeDef* SPIx, uint32_t frequency);
void AD9833_SetAmplitude(SPI_TypeDef* SPIx, uint8_t amplitude);
void AD9833_SetOutputMode(SPI_TypeDef* SPIx, uint16_t mode);
void AD9833_Reset(SPI_TypeDef* SPIx);
void AD9833_EnableOutput(SPI_TypeDef* SPIx, uint8_t enable);


#endif /*__AD9833_H__ */