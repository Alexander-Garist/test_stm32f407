#include "spi.h"

#define SPI_SR_FREE (0 << 7)
static SPI_Status_t SPI_Wait_Set_Flag_SR(SPI_TypeDef* SPIx, uint16_t flag, uint32_t timeout)       //Ожидание установки флага состояния SR
{
    uint32_t start_time = get_current_time();
    while(!(SPIx->SR & flag))
    {
        if(is_time_passed(start_time, timeout)) return SPI_FLAG_TIMEOUT;
    }
    return SPI_OK;
}
static SPI_Status_t SPI_Wait_Clear_Flag_SR(SPI_TypeDef* SPIx, uint16_t flag, uint32_t timeout)       //Ожидание очистки флага состояния SR
{
    uint32_t start_time = get_current_time();
    while((SPIx->SR & flag))
    {
        if(is_time_passed(start_time, timeout)) return SPI_FLAG_TIMEOUT;
    }
    return SPI_OK;
}


static void SPI_RCC_Enable(SPI_TypeDef* SPIx)           //Включение тактирования выбранного SPI
{
    if(SPIx == SPI1) RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    if(SPIx == SPI2) RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    if(SPIx == SPI3) RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;
}
static void SPI_Init(SPI_TypeDef* SPIx)                 //Инициализация выбранного SPI
{
    SPIx->CR1 = 0;      //Сброс всех битов CR1 в состояние RESET, т.е. все нули

    SPIx->CR1 &= ~SPI_CR1_DFF;      //Data Frame Format: 0 - 8 bit, 1 - 16 bit
    SPIx->CR1 &= ~SPI_CR1_RXONLY;   //0 - full duplex, 1 - receive only
    SPIx->CR1 |= SPI_CR1_SSM;       //software slave management
    SPIx->CR1 |= SPI_CR1_SSI;       //internal slave select
    SPIx->CR1 &= ~SPI_CR1_LSBFIRST; //MSB first

    SPIx->CR1 &= ~SPI_CR1_BR;       //Baud rate: 000 - Fpclk/2

    SPIx->CR1 |= SPI_CR1_MSTR;      //master mode
    SPIx->CR1 &= ~SPI_CR1_CPOL;     //Clock polarity 0
    SPIx->CR1 &= ~SPI_CR1_CPHA;     //Clock phase 0

    SPIx->CR1 |= SPI_CR1_SPE;       //enable
}

void SPI_Enable_Pin(SPI_TypeDef* SPIx)                  //Включение выбранного SPI (тактирование и настройка регистров)
{
    SPI_RCC_Enable(SPIx);   //Включение тактирования SPIx
    SPI_Init(SPIx);         //Настройка регистров SPIx
}



SPI_Status_t SPI_Transmit(SPI_TypeDef* SPIx, uint8_t* data, uint32_t size)      //отправить по SPI
{
    for(uint32_t i = 0; i < size; i++)
    {   //TXE==1 означает что буфер передатчика освободился и можно в него внести новый байт данных
        if(SPI_Wait_Set_Flag_SR(SPIx, SPI_SR_TXE, 10) != SPI_OK) return SPI_ERROR_WRITE;    //ожидание установки флага SPI_SR_TXE с таймаутом 10 мс
		SPIx->DR = data[i];                                                             //запись байта данных в регистр данных
    }
	if(SPI_Wait_Set_Flag_SR(SPIx, SPI_SR_TXE, 10) != SPI_OK) return SPI_ERROR_WRITE;        //ожидание установки флага SPI_SR_TXE с таймаутом 10 мс

    //BSY==1 означает что шина SPI занята либо буфер передатчика еще не освободился
    //значит BSY==0 означает что буфер передатчика свободен => все данные отправлены
    //SPI_SR_BSY это 1 в 7 бите регистра SR
    //SPI_SR_FREE это 0 в 7 бите регитсра SR
    if(SPI_Wait_Clear_Flag_SR(SPIx, SPI_SR_BSY, 10) != SPI_OK) return SPI_ERROR_WRITE;       //ожидание очистки флага SPI_SR_BSY с таймаутом 10 мс

	(void)SPIx->DR;
	(void)SPIx->SR;

    return SPI_OK;
}

SPI_Status_t SPI_Receive(SPI_TypeDef* SPIx, uint8_t* data, uint32_t size)       //принять по SPI
{
    while(size)
	{
		SPIx->DR = 0;    //запуск обмена
        //RXNE==1 означает что в буфере приемника появился байт данных
        if(SPI_Wait_Set_Flag_SR(SPIx, SPI_SR_RXNE, 10) != SPI_OK) return SPI_ERROR_READ;   //ожидание установки флага SPI_SR_RXNE с таймаутом 10 мс
		*data++ = (SPIx->DR);                                                           //чтение байта данных из регистра в массив data
		size--;
	}
    return SPI_OK;
}



/////////////////////////доработать статусы ошибок
/*SPI_Status_t SPI_Transmit(SPI_TypeDef* SPIx, uint8_t* data, uint32_t size)      //отправить по SPI
{
    for(uint32_t i = 0; i < size; i++)
    {
        //Wait until TXE is set
		while(!(SPIx->SR & (SPI_SR_TXE))){}

		//Write the data to the data register
		SPIx->DR = data[i];
    }

	//Wait until TXE is set
	while(!(SPIx->SR & (SPI_SR_TXE))){}

	//Wait for BUSY flag to reset
	while((SPIx->SR & (SPI_SR_BSY))){}

	//Clear OVR flag
	(void)SPIx->DR;
	(void)SPIx->SR;

    return SPI_OK;
}
*/
/*SPI_Status_t SPI_Receive(SPI_TypeDef* SPIx, uint8_t* data, uint32_t size)       //принять по SPI
{
    while(size)
	{
		SPIx->DR =1;    //Send dummy data

		//Wait for RXNE flag to be set
		while(!(SPIx->SR & (SPI_SR_RXNE))){}

		//Read data from data register
		*data++ = (SPIx->DR);
		size--;
	}
    return SPI_OK;
}*/



