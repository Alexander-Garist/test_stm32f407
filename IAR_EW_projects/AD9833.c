/**
  * @file    AD9833.c
  * @brief   Файл содержит реализации функций AD9833
  */

/** Includes ******************************************************************/
#include "AD9833.h"
#include "systick.h"
#include "gpio.h"

/**
  * @brief Инициализация GPIO для управления модулем AD9833
  */
void AD9833_GPIO_Init(void)
{
	// Включение тактирования, инициализация пинов как выход, установка высокого уровня на выходе
	GPIO_set_HIGH(AD9833_FSY_PORT, AD9833_FSY_PIN);		// PA3 -> FSY
	GPIO_set_HIGH(MCP41010_CS_PORT, MCP41010_CS_PIN);	// PA4 -> CS
}

/**
  ! Запись 16-битного слова в AD9833
  - SPIx - SPI модуль (SPI1, SPI2, SPI3)
  - data - 16-битные данные для записи
  */
static void AD9833_Write(SPI_TypeDef* SPIx, uint16_t data)
{
    uint8_t spi_data[2];
    // Преобразуем 16-битное слово в два байта (старший первый)
    spi_data[0] = (data >> 8) & 0xFF;
    spi_data[1] = data & 0xFF;
    // Активируем FSY (active low)
    AD9833_FSY_PORT->BSRR = (uint32_t)AD9833_FSY_PIN << 16;
    // Отправляем данные через SPI
    SPI_Transmit(SPIx, spi_data, 2);
    // Деактивируем FSY
    AD9833_FSY_PORT->BSRR = AD9833_FSY_PIN;
}

/**
  ! Запись значения в цифровой потенциометр MCP41010
  - SPIx - SPI модуль (SPI1, SPI2, SPI3)
  - value - значение амплитуды (0-255)
  */
static void MCP41010_Write(SPI_TypeDef* SPIx, uint8_t value)
{
    uint8_t spi_data[2];
    uint16_t command = MCP41010_CMD_WRITE | value;
    // Преобразуем команду в два байта
    spi_data[0] = (command >> 8) & 0xFF;
    spi_data[1] = command & 0xFF;
    // Активируем CS (active low)
    MCP41010_CS_PORT->BSRR = (uint32_t)MCP41010_CS_PIN << 16;
    // Отправляем данные через SPI
    SPI_Transmit(SPIx, spi_data, 2);
    // Деактивируем CS
    MCP41010_CS_PORT->BSRR = MCP41010_CS_PIN;
}

/**
  * @brief Инициализация модуля AD9833
  * @param SPIx: SPI модуль (SPI1, SPI2, SPI3)
  */
void AD9833_Module_Init(SPI_TypeDef* SPIx)
{
    AD9833_GPIO_Init();
    AD9833_Reset(SPIx);
    // Конфигурируем AD9833 для нормальной работы
    AD9833_Write(SPIx, 0x2100); // B28 bit set for double frequency writes
    // Устанавливаем среднюю амплитуду
    AD9833_SetAmplitude(SPIx, 128);
}

/**
  * @brief Установка частоты выходного сигнала
  * @param SPIx: SPI модуль (SPI1, SPI2, SPI3)
  * @param frequency: частота в Герцах
  */
void AD9833_SetFrequency(SPI_TypeDef* SPIx, uint32_t frequency)
{
    uint32_t freq_word;
    // Расчет значения частоты: FREQ = (f * 2^28) / MCLK
    freq_word = (uint32_t)((frequency * 268435456.0) / MCLK_FREQUENCY);
    // Запись в регистр частоты FREQ0 (14-битные фрагменты)
    AD9833_Write(SPIx, AD9833_FREQ0_REG | (freq_word & 0x3FFF));         // LSB
    AD9833_Write(SPIx, AD9833_FREQ0_REG | ((freq_word >> 14) & 0x3FFF)); // MSB
}

/**
  * @brief Установка амплитуды выходного сигнала
  * @param SPIx: SPI модуль (SPI1, SPI2, SPI3)
  * @param amplitude: амплитуда (0-255)
  */
void AD9833_SetAmplitude(SPI_TypeDef* SPIx, uint8_t amplitude)
{
    MCP41010_Write(SPIx, amplitude);
}

/**
  * @brief Установка формы выходного сигнала
  * @param SPIx: SPI модуль (SPI1, SPI2, SPI3)
  * @param mode: режим (AD9833_SINE_MODE, AD9833_TRIANGLE_MODE, AD9833_SQUARE_MODE)
  */
void AD9833_SetOutputMode(SPI_TypeDef* SPIx, uint16_t mode)
{
    AD9833_Write(SPIx, mode);
}

/**
  * @brief Сброс AD9833
  * @param SPIx: SPI модуль (SPI1, SPI2, SPI3)
  */
void AD9833_Reset(SPI_TypeDef* SPIx)
{
    AD9833_Write(SPIx, AD9833_RESET_CMD);
    delay_ms(10);
    AD9833_Write(SPIx, 0x0000); // Снять сброс
}

/**
  * @brief Включение/выключение выходного сигнала
  * @param SPIx: SPI модуль (SPI1, SPI2, SPI3)
  * @param enable: 1 - включить, 0 - выключить
  */
void AD9833_EnableOutput(SPI_TypeDef* SPIx, uint8_t enable)
{
    if(enable) {
        AD9833_Write(SPIx, AD9833_FREQ0_REG); // Включить выход
    } else {
        AD9833_Write(SPIx, AD9833_SLEEP1_CMD); // Выключить выход (режим сна)
    }
}