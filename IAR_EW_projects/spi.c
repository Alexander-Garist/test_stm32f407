/**
  * @file    spi.c
  * @brief   Файл содержит реализации функций SPI
  */

/** Includes ******************************************************************/
#include "spi.h"
#include "systick.h"

/** Static Functions **********************************************************/

	/**
	! Статическая функция SPI_Wait_Set_Flag_SR реализует ожидание установки
		флага состояния SR с заданным пределом времени ожидания.
	- SPIx - выбранный модуль SPI (SPI1, SPI2, SPI3)
	- SPI_flag - маска флага, который должен установиться
	- SPI_timeout - максимальное время ожидания установки флага
	return: статус выполнения установки флага (если успешно установился в
		пределах времени SPI_timeout, то SPI_OK)
	*/
static SPI_Status_t SPI_Wait_Set_Flag_SR(SPI_TypeDef* SPIx, uint16_t SPI_flag, uint32_t SPI_timeout)
{
    uint32_t start_time = get_current_ms();
    while (!(SPIx->SR & SPI_flag))   // Ожидание пока флаг не установлен
    {
        if (is_time_passed_ms(start_time, SPI_timeout)) return SPI_FLAG_TIMEOUT;
    }
    return SPI_OK;
}

	/**
	! Статическая функция SPI_Wait_Set_Flag_SR реализует ожидание сброса
		флага состояния SR с заданным пределом времени ожидания.
	- SPIx - выбранный модуль SPI (SPI1, SPI2, SPI3)
	- SPI_flag - маска флага, который должен сброситься
	- SPI_timeout - максимальное время ожидания сброса флага
	return: статус выполнения сброса флага (если успешно сброшен в
		пределах времени SPI_timeout, то SPI_OK)
	*/
static SPI_Status_t SPI_Wait_Clear_Flag_SR(SPI_TypeDef* SPIx, uint16_t SPI_flag, uint32_t SPI_timeout)
{
    uint32_t start_time = get_current_ms();
    while ((SPIx->SR & SPI_flag))    // Ожидание пока флаг установлен
    {
        if (is_time_passed_ms(start_time, SPI_timeout)) return SPI_FLAG_TIMEOUT;
    }
    return SPI_OK;
}

	/**
	! Статическая функция SPI_RCC_Enable включает тактирование выбранного
		модуля SPI.
	- SPIx - выбранный модуль SPI (SPI1, SPI2, SPI3)
	*/
static void SPI_RCC_Enable(SPI_TypeDef* SPIx)
{
    if (SPIx == SPI1) RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    if (SPIx == SPI2) RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    if (SPIx == SPI3) RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;
}

	/**
	! Инициализация выбранного модуля SPI, настройка регистров SPI.
	- SPIx - выбранный модуль SPI (SPI1, SPI2, SPI3)
	*/
static void SPI_Init(SPI_TypeDef* SPIx)
{
    SPIx->CR1 = 0;      // Сброс всех битов CR1 в состояние RESET, т.е. все нули

    SPIx->CR1 &= ~SPI_CR1_DFF;      // Data Frame Format: 0 - 8 bit, 1 - 16 bit
    SPIx->CR1 &= ~SPI_CR1_RXONLY;   // 0 - full duplex, 1 - receive only
    SPIx->CR1 |= SPI_CR1_SSM;       // Software slave management
    SPIx->CR1 |= SPI_CR1_SSI;       // Internal slave select
    SPIx->CR1 &= ~SPI_CR1_LSBFIRST; // MSB first
    SPIx->CR1 &= ~SPI_CR1_BR;       // Baud rate: 000 - Fpclk/2
    SPIx->CR1 |= SPI_CR1_MSTR;      // Master mode
    SPIx->CR1 &= ~SPI_CR1_CPOL;     // Clock polarity 0
    SPIx->CR1 &= ~SPI_CR1_CPHA;     // Clock phase 0

	// Enable
    SPIx->CR1 |= SPI_CR1_SPE;
}

/** Functions *****************************************************************/

	/**
	! Включение выбранного модуля SPI (тактирование и настройка регистров)
	- SPIx - модуль SPI (SPI1, SPI2, SPI3)
	*/
void SPI_Enable_Pin(SPI_TypeDef* SPIx)
{
    SPI_RCC_Enable(SPIx);   //Включение тактирования SPIx
    SPI_Init(SPIx);         //Настройка регистров SPIx
}

	/**
	! Функция отправки данных по шине SPI
	- SPIx - модуль SPI (SPI1, SPI2, SPI3)
	- SPI_data - указатель на массив отправляемых данных
	- SPI_size - объем передаваемых данных в байтах
	return: статус выполнения отправки данных (если отправка успешна, вернет SPI_OK)
	*/
SPI_Status_t SPI_Transmit(SPI_TypeDef* SPIx, uint8_t* SPI_data, uint32_t SPI_size)
{
    for (uint32_t i = 0; i < SPI_size; i++)
    {
		// TXE==1 означает что буфер передатчика освободился и можно в него внести новый байт данных
        if (SPI_Wait_Set_Flag_SR(SPIx, SPI_SR_TXE, 10) != SPI_OK) return SPI_ERROR_WRITE;	// Ожидание установки флага SPI_SR_TXE с таймаутом 10 мс
		SPIx->DR = SPI_data[i];																	// Запись байта данных в регистр данных
    }
	if (SPI_Wait_Set_Flag_SR(SPIx, SPI_SR_TXE, 10) != SPI_OK) return SPI_ERROR_WRITE;		//  Ожидание установки флага SPI_SR_TXE с таймаутом 10 мс

    // BSY==1 означает что шина SPI занята либо буфер передатчика еще не освободился
    // значит BSY==0 означает что буфер передатчика свободен => все данные отправлены
    // SPI_SR_BSY это 1 в 7 бите регистра SR
    if (SPI_Wait_Clear_Flag_SR(SPIx, SPI_SR_BSY, 10) != SPI_OK) return SPI_ERROR_WRITE;		// Ожидание сброса флага SPI_SR_BSY с таймаутом 10 мс
	(void)SPIx->DR;
	(void)SPIx->SR;

	// В случае успешного завершения работы функции статус SPI_OK
    return SPI_OK;
}

	/**
	! Функция приема данных по шине SPI
	- SPIx - модуль SPI (SPI1, SPI2, SPI3)
	- SPI_data - указатель на массив, в который запишутся принятые данные
	- SPI_size - объем принимаемых данных в байтах
	return: статус выполнения приема данных (если прием успешен, вернет SPI_OK)
	*/
SPI_Status_t SPI_Receive(SPI_TypeDef* SPIx, uint8_t* SPI_data, uint32_t SPI_size)
{
    while (SPI_size)
	{
		SPIx->DR = 0;    // Запуск обмена

        // RXNE==1 означает что в буфере приемника появился байт данных
        if (SPI_Wait_Set_Flag_SR(SPIx, SPI_SR_RXNE, 10) != SPI_OK) return SPI_ERROR_READ;	// Ожидание установки флага SPI_SR_RXNE с таймаутом 10 мс
		*SPI_data++ = (SPIx->DR);															// Чтение байта данных из регистра в массив SPI_data
		SPI_size--;
	}

	// В случае успешного завершения работы функции статус SPI_OK
    return SPI_OK;
}