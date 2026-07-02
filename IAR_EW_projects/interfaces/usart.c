/**
  * @file    usart.c
  * @brief   Файл содержит реализации функций UART/USART
  */

/** Includes **********************************************************************************************************/
#include <stdio.h>
#include "usart.h"
#include "gpio.h"
#include "systick.h"
#include "string.h"

// Частоты шин APB1 APB2
static uint32_t freq_APB1, freq_APB2;

/***************************************** Статические функции ********************************************************/
// Определить делители частот APB1 APB2
static void get_prescalers()
{
    uint32_t ppre1, ppre2;

    uint32_t PPRE1 = RCC->CFGR; // 32 бита регистра
    PPRE1 >>= 10;               // Сдвиг на 10 бит вправо
    PPRE1 &= 0x7;               // Наложение маски 0b0111, теперь это значение PPRE1

    uint32_t PPRE2 = RCC->CFGR; // 32 бита регистра
    PPRE2 >>= 13;               // Сдвиг на 13 бит вправо
    PPRE2 &= 0x7;               // Наложение маски 0b0111, теперь это значение PPRE2

    switch (PPRE1)
    {
        case 0x4:   ppre1 = 2; break;
        case 0x5:   ppre1 = 4; break;
        case 0x6:   ppre1 = 8; break;
        case 0x7:   ppre1 = 16; break;
        default:    ppre1 = 1; break;
    }

    switch (PPRE2)
    {
        case 0x4:   ppre2 = 2; break;
        case 0x5:   ppre2 = 4; break;
        case 0x6:   ppre2 = 8; break;
        case 0x7:   ppre2 = 16; break;
        default:    ppre2 = 1; break;
    }

    freq_APB1 = SystemCoreClock / ppre1;
    freq_APB2 = SystemCoreClock / ppre2;
}

// Включение тактирования модуля UART/USART
static void USART_RCC_Enable(USART_TypeDef* USARTx)
{
	if (USARTx == USART1)	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
	if (USARTx == USART2)	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
	if (USARTx == USART3)	RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
	if (USARTx == UART4)	RCC->APB1ENR |= RCC_APB1ENR_UART4EN;
	if (USARTx == UART5)	RCC->APB1ENR |= RCC_APB1ENR_UART5EN;
	if (USARTx == USART6)	RCC->APB2ENR |= RCC_APB2ENR_USART6EN;
}

// Настройка регистра BRR и включение модуля USARTx
static void USART_Init(USART_TypeDef* USARTx, uint32_t baudrate)
{
    get_prescalers();

	/** Настройка регистра BRR (Baud rate register)
	*	APB1 для USART 2/3 UART 4/5
	*	APB2 для USART 1/6
	*/
	switch ((uint32_t)USARTx)
	{
		case ((uint32_t)USART1):	USARTx->BRR = freq_APB2 / baudrate;		break;
		case ((uint32_t)USART2):	USARTx->BRR = freq_APB1 / baudrate; 	break;
		case ((uint32_t)USART3):	USARTx->BRR = freq_APB1 / baudrate; 	break;
		case ((uint32_t)UART4):		USARTx->BRR = freq_APB1 / baudrate; 	break;
		case ((uint32_t)UART5):		USARTx->BRR = freq_APB1 / baudrate; 	break;
		case ((uint32_t)USART6):	USARTx->BRR = freq_APB2 / baudrate; 	break;
	}

	// Включение USARTx: TX, RX
    USARTx->CR1 |= USART_CR1_TE 			// Transmitter enable
				| USART_CR1_RE;			// Receiver enable

    USARTx->CR1 |= USART_CR1_UE;		// USART enable
}

/*************************************** Публичные функции ************************************************************/

/********************* Включение, инициализация модуля UART/USART, разрешение прерываний USART ************************/

// Включение и настройка модуля USARTx с использованием структуры инициализации
void USART_Enable(USART_Init_Struct* Init_Struct)
{
	// Включение тактирования USARTx
	USART_RCC_Enable(Init_Struct->USARTx);

	// Включение тактирования GPIO, настройка заданных пинов приемника и передатчика в режиме AF (регистры MODER и AFR)
	GPIO_Enable_USART(
		Init_Struct->USARTx,
		Init_Struct->GPIO_port_Tx,
		Init_Struct->GPIO_pin_Tx,
		Init_Struct->GPIO_port_Rx,
		Init_Struct->GPIO_pin_Rx
	);

	// Настройка регистра BRR и включение модуля USARTx
	USART_Init(Init_Struct->USARTx, Init_Struct->baudrate);
}

// Разрешение обработки прерываний USART и установка приоритета соответствующего прерывания
void USART_EnableIRQ(USART_TypeDef* USARTx, uint32_t priority)
{
	// Разрешение прерываний по приемнику (обработчик прерывания модуля USARTx вызывается если приемник не пустой)
	USARTx->CR1 |= USART_CR1_RXNEIE;

    switch((uint32_t)USARTx) {
        case ((uint32_t)USART1):
            NVIC_EnableIRQ(USART1_IRQn);
			NVIC_SetPriority(USART1_IRQn, priority);
            break;

        case ((uint32_t)USART2):
            NVIC_EnableIRQ(USART2_IRQn);
    		NVIC_SetPriority(USART2_IRQn, priority);
            break;

        case ((uint32_t)USART3):
            NVIC_EnableIRQ(USART3_IRQn);
			NVIC_SetPriority(USART3_IRQn, priority);
            break;

        case ((uint32_t)UART4):
            NVIC_EnableIRQ(UART4_IRQn);
			NVIC_SetPriority(UART4_IRQn, priority);
            break;

        case ((uint32_t)UART5):
            NVIC_EnableIRQ(UART5_IRQn);
			NVIC_SetPriority(UART5_IRQn, priority);
            break;

        case ((uint32_t)USART6):
			NVIC_EnableIRQ(USART6_IRQn);
			NVIC_SetPriority(USART6_IRQn, priority);
            break;
    }
}

// Запрет обработки прерываний USART
void USART_DisableIRQ(USART_TypeDef* USARTx)
{
	// Запрет прерываний по приемнику (обработчик прерывания модуля USARTx вызывается если приемник не пустой)
	USARTx->CR1 &= ~USART_CR1_RXNEIE;

    switch((uint32_t)USARTx) {
        case ((uint32_t)USART1):	NVIC_DisableIRQ(USART1_IRQn);	break;
        case ((uint32_t)USART2):    NVIC_DisableIRQ(USART2_IRQn);	break;
        case ((uint32_t)USART3):	NVIC_DisableIRQ(USART3_IRQn);	break;
        case ((uint32_t)UART4):		NVIC_DisableIRQ(UART4_IRQn);	break;
        case ((uint32_t)UART5):		NVIC_DisableIRQ(UART5_IRQn);	break;
        case ((uint32_t)USART6):	NVIC_DisableIRQ(USART6_IRQn);	break;
    }
}

/******************************** Прием/передача **********************************************************************/

// Функция побайтной отправки size байт данных по USART
USART_Status_t USART_Transmit(USART_TypeDef* USARTx, char* data, uint32_t size)
{
	// добавить таймаут отправки
	// если не отправлено до таймаута, выкинуть ошибку USART_ERROR_TRANSMIT
	for (uint32_t index = 0; index < size; index++)
	{
		while (!(USARTx->SR & USART_SR_TXE));	// Ожидание пока буфер передатчика освободится
		USARTx->DR = data[index];				// Отправка 1 символа
	}
	return USART_OK;
}

// Функция побайтной отправки size байт данных по USART
USART_Status_t USART_Transmit_UINT8(USART_TypeDef* USARTx, uint8_t* data, uint32_t size)
{
	// добавить таймаут отправки
	// если не отправлено до таймаута, выкинуть ошибку USART_ERROR_TRANSMIT
	for (uint32_t index = 0; index < size; index++)
	{
		while (!(USARTx->SR & USART_SR_TXE));	// Ожидание пока буфер передатчика освободится
		USARTx->DR = data[index];				// Отправка 1 символа
	}
	return USART_OK;
}


// Побайтный прием из USART до стопового байта
USART_Status_t USART_Receive(USART_TypeDef* USARTx, char* buffer, char STOP_BYTE)
{
	uint32_t received_bytes = 0;				// Количество принятых байт

	while (received_bytes < MAX_BUFFER_SIZE)	// За одну операцию чтения можно считать не более одного буфера
	{
		while (!(USARTx->SR & USART_SR_RXNE)){}				// Ожидание пока не придет в приемник ОДИН байт
		buffer[received_bytes] = USARTx->DR;				// Запись пришедшего байта в буфер
		if (buffer[received_bytes] == STOP_BYTE)
		{
			break;		// Конечный символ на случай если нужно принять менее 256 байт
		}
		received_bytes++;
	}

	return USART_OK;
}

/***************************** Отладочные функции *********************************************************************/

// Вывести текущее содержимое буфера USART
void USART_print_Buffer(char* buffer, uint32_t buffer_size)
{
    for (int i = 0; i < buffer_size; i++)
    {
        printf("%c", buffer[i]);
    }
    printf("\n\n");
}

// Очистить буфер
void USART_clear_Buffer(char* buffer)
{
    *buffer = '\0';
}

/***************************************** Обработчики прерываний *****************************************************/

// TODO: сделать нормальный обработчик прерывания USART3
void USART3_IRQHandler(void)
{

}