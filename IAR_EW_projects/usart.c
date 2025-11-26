/**
  * @file    usart.c
  * @brief   Файл содержит реализации функций UART
  */

/** Includes **********************************************************************************************************/
#include "usart.h"
#include "gpio.h"
#include "systick.h"
#include "string.h"

/** Defines ***********************************************************************************************************/
// Частоты шин APB1 APB2
#define freq_APB1	16000000
#define freq_APB2	16000000

// Размер буфера приемника
#define USART_BUFFER_SIZE	256

/************************** Переменные ********************************************************************************/

uint8_t USART_Rx_Buffer[USART_BUFFER_SIZE];	// Буфер приемника на 256 байт
uint8_t Buffer_Index = 0;					// Индекс позиции в буфере
uint8_t Command_Is_Received = 0;			// Флаг, показывающий, что команда принята целиком

/***************************************** Статические функции ********************************************************/

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

	// Включение USARTx: TX, RX и прерывание по приему
    USARTx->CR1 = USART_CR1_TE 			// Transmitter enable
				| USART_CR1_RE;			// Receiver enable

    USARTx->CR1 |= USART_CR1_UE;		// USART enable
}

/*************************************** Публичные функции ************************************************************/

/********************* Включение, инициализация модуля UART/USART, разрешение прерываний USART ************************/

// Включение и настройка модуля USARTx с использованием структуры инициализации
void USART_Enable_with_struct(USART_Init_Struct* Init_Struct)
{
	// Включение тактирования USARTx
	USART_RCC_Enable(Init_Struct->USARTx);

	// Включение тактирования GPIO, настройка заданных пинов приемника и передатчика в режиме AF (регистры MODER и AFR)
	GPIO_Enable_USART(
		Init_Struct->USARTx,
		Init_Struct->GPIO_port_Tx,
		Init_Struct->GPIO_port_Rx,
		Init_Struct->GPIO_pin_Tx,
		Init_Struct->GPIO_pin_Rx
	);

	// Настройка регистра BRR и включение модуля USARTx
	USART_Init(Init_Struct->USARTx, Init_Struct->baudrate);
}

// Включение и настройка модуля USARTx ( USART1 // USART2 // USART3 // UART4 // UART5 // USART6)
void USART_Enable(USART_TypeDef* USARTx, GPIO_TypeDef* GPIO_port_Tx, int GPIO_pin_Tx, GPIO_TypeDef* GPIO_port_Rx, int GPIO_pin_Rx , uint32_t baudrate)
{
	// Включение тактирования USARTx
	USART_RCC_Enable(USARTx);

	// Включение тактирования GPIO, настройка заданных пинов приемника и передатчика в режиме AF (регистры MODER и AFR)
	GPIO_Enable_USART(USARTx, GPIO_port_Tx, GPIO_port_Rx, GPIO_pin_Tx, GPIO_pin_Rx);

	// Настройка регистра BRR и включение модуля USARTx
	USART_Init(USARTx, baudrate);
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
	// Разрешение прерываний по приемнику (обработчик прерывания модуля USARTx вызывается если приемник не пустой)
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























// отправка одного символа
void USART_Send_Char(USART_TypeDef* USARTx, char c)
{
    while (!(USARTx->SR & USART_SR_TXE));	// Ожидание пока буфер передатчика освободится
    USARTx->DR = c;							// Отправка 1 символа
}

// посимвольная отправка строки
void USART_Send_String(USART_TypeDef* USARTx, const char* str)
{
    while (*str)
    {
        USART_Send_Char(USARTx, *str++);
    }
}

// отправка числа как строки цифр
void USART_Send_Number(USART_TypeDef* USARTx, uint32_t num)
{
    char buffer[12];
    char *p = buffer + 11;
    *p = '\0';

    if (num == 0)
    {
        USART_Send_Char(USARTx, '0');
        return;
    }

    while (num > 0)
    {
        *--p = '0' + (num % 10);
        num /= 10;
    }

    USART_Send_String(USARTx, p);
}



void USART_Receive_Char(USART_TypeDef* USARTx, char* c)
{
	// ожидание появления символа в приемнике
	while (!(USART3->SR & USART_SR_RXNE)){}

	// запись символа из приемника
	*c = (char)(USARTx->DR & 0xFF);
}




/***************************************** Обработчики прерываний *****************************************************/
// сделать нормальный обработчик прерывания USART3
void USART3_IRQHandler(void)
{

}
