/**
  * @file    AD9833.c
  * @brief   Файл содержит реализации функций AD9833
  */

/** Includes **********************************************************************************************************/
#include "AD9833.h"
#include "systick.h"
#include "gpio.h"

/** Functions *********************************************************************************************************/

/***************************************** Static functions ***********************************************************/

	/**
	! Запись 16-битного слова в AD9833.
	- SPIx - SPI модуль (SPI1, SPI2, SPI3).
	- data - 16-битные данные для записи.
	*/
static void AD9833_Write(SPI_TypeDef* SPIx, uint16_t data)
{
    uint8_t spi_data[2];

    // Преобразование 16-битного слова в два байта (старший первый)
    spi_data[0] = (data >> 8) & 0xFF;
    spi_data[1] = data & 0xFF;

    // Включение FSY (низкий уровень)
    GPIO_set_LOW(AD9833_FSY_PORT, AD9833_FSY_PIN);

    // Отправка данных через SPI
    SPI_Transmit(SPIx, spi_data, 2);

    // Выключение FSY (высокий уровень)
    GPIO_set_HIGH(AD9833_FSY_PORT, AD9833_FSY_PIN);
}

	/**
	! Запись значения в цифровой потенциометр MCP41010.
	- SPIx - SPI модуль (SPI1, SPI2, SPI3)
	- value - значение амплитуды (0-255)
	*/
static void MCP41010_Write(SPI_TypeDef* SPIx, uint8_t value)
{
    uint8_t spi_data[2];
    uint16_t command = MCP41010_CMD_WRITE | value;	// Значение амплитуды записывается во второй байт данных, в первом остается команда 0x11

    // Преобразование команды в два байта
    spi_data[0] = (command >> 8) & 0xFF;
    spi_data[1] = command & 0xFF;

    // Включение CS (низкий уровень)
    GPIO_set_LOW(MCP41010_CS_PORT, MCP41010_CS_PIN);

    // Отправка данных через SPI
    SPI_Transmit(SPIx, spi_data, 2);

    // Выключение CS (высокий уровень)
    GPIO_set_HIGH(MCP41010_CS_PORT, MCP41010_CS_PIN);
}

/***************************************** Public functions ***********************************************************/

	/**
	! Включение тактирования порта GPIO, инициализация выводов GPIO для управления генератора сигналов AD9833 и
		цифровым потенциометром MCP41010.
		Установка высокого уровня на выводах GPIO.
	*/
void AD9833_GPIO_Init(void)
{
	GPIO_set_HIGH(AD9833_FSY_PORT, AD9833_FSY_PIN);		// PA3 -> FSY
	GPIO_set_HIGH(MCP41010_CS_PORT, MCP41010_CS_PIN);	// PA4 -> CS
}

	/**
	! Инициализация модуля AD9833 с заданными параметрами.
	- SPIx - SPI модуль (SPI1, SPI2, SPI3).
	- frequency - частота выходного сигнала в Герцах.
	- amplitude - амплитуда выходного сигнала как часть от максимальной (например 128: 128/256 = 50% от максимальной).
	- mode - форма выходного сигнала (AD9833_SIN_MODE, AD9833_TRIANGLE_MODE, AD9833_SQUARE_MODE)
	*/
void AD9833_Module_Init(SPI_TypeDef* SPIx, uint32_t frequency, uint8_t amplitude, uint16_t mode)
{
    AD9833_GPIO_Init();
    AD9833_Reset(SPIx);

    // Настройка частоты выходного сигнала
    //AD9833_Write(SPIx, 0x2000); // Управляющий код 0010 0000 0000 0000 => далее 2 14-битных слова частоты
	AD9833_SetFrequency(SPIx, frequency);

    // Настройка амплитуды выходного сигнала
    AD9833_SetAmplitude(SPIx, amplitude);

	// Настройка формы выходного сигнала
	AD9833_SetOutputMode(SPIx, mode);

	// Включение выходного сигнала
	AD9833_EnableOutput(SPIx, 1);
}

	/**
	! Установка частоты выходного сигнала.
	- SPIx - SPI модуль (SPI1, SPI2, SPI3).
	- frequency - частота в Герцах.
	*/
void AD9833_SetFrequency(SPI_TypeDef* SPIx, uint32_t frequency)
{
    uint32_t freq_word;
    // Расчет значения частоты: FREQ = (f * 2^28) / MCLK
    freq_word = (uint32_t)((frequency * 268435456.0) / MCLK_FREQUENCY);

	AD9833_Write(SPIx, 0x2000); // Управляющий код 0010 0000 0000 0000 => далее 2 14-битных слова частоты
    // Запись в регистр частоты FREQ0 (14-битные фрагменты)
    AD9833_Write(SPIx, AD9833_FREQ0_REG | (freq_word & 0x3FFF));         // LSB
	AD9833_Write(SPIx, AD9833_FREQ0_REG | ((freq_word >> 14) & 0x3FFF)); // MSB
}

	/**
	! Установка амплитуды выходного сигнала.
	- SPIx - SPI модуль (SPI1, SPI2, SPI3).
	- amplitude - амплитуда (0-255).
	*/
void AD9833_SetAmplitude(SPI_TypeDef* SPIx, uint8_t amplitude)
{
    MCP41010_Write(SPIx, amplitude);
}

	/**
	! Установка формы выходного сигнала.
	- SPIx - SPI модуль (SPI1, SPI2, SPI3).
	- mode - режим (AD9833_SIN_MODE, AD9833_TRIANGLE_MODE, AD9833_SQUARE_MODE).
	*/
void AD9833_SetOutputMode(SPI_TypeDef* SPIx, uint16_t mode)
{
    AD9833_Write(SPIx, mode);
}

	/**
	! Сброс внутренних регистров AD9833. Регистры фазы, частоты и управления НЕ СБРАСЫВАЮТСЯ.
	- SPIx - SPI модуль (SPI1, SPI2, SPI3).
	*/
void AD9833_Reset(SPI_TypeDef* SPIx)
{
    AD9833_Write(SPIx, AD9833_RESET_CMD);
    delay_ms(10);
    AD9833_Write(SPIx, 0x0000); // Снять сброс
	delay_ms(10);
}

	/**
	! Включение/выключение выходного сигнала.
	- SPIx - SPI модуль (SPI1, SPI2, SPI3).
	- enable - 1 - включить, 0 - выключить.
	*/
void AD9833_EnableOutput(SPI_TypeDef* SPIx, uint8_t enable)
{
    if(enable) {
        AD9833_Write(SPIx, AD9833_FREQ0_REG);	// Включить выход
    } else {
        AD9833_Write(SPIx, AD9833_SLEEP1_CMD);	// Выключить выход (режим сна)
    }
}