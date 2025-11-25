/**
  * @file    usart.c
  * @brief   Файл содержит реализации функций UART
  */

/** Includes **********************************************************************************************************/
#include <stdio.h>
#include "usart.h"
#include "gpio.h"
#include "systick.h"

/** Defines ***********************************************************************************************************/
#define freq_APB1	16000000
#define freq_APB2	16000000


/***************************************** Статические функции ********************************************************/

// Включение тактирования модуля UART/USART
static void USART_RCC_Enable(USART_TypeDef* USARTx)
{
	if (USARTx == USART1) RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
	if (USARTx == USART2) RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
	if (USARTx == USART3) RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
	if (USARTx == UART4) RCC->APB1ENR |= RCC_APB1ENR_UART4EN;
	if (USARTx == UART5) RCC->APB1ENR |= RCC_APB1ENR_UART5EN;
	if (USARTx == USART6) RCC->APB2ENR |= RCC_APB2ENR_USART6EN;
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
				| USART_CR1_RE			// Receiver enable
				| USART_CR1_RXNEIE;		// Разрешение прерываний по приемнику (т.е. если приемник не пустой, вызывается обработчик прерывания соответствующего модуля USART)

    USARTx->CR1 |= USART_CR1_UE;		// USART enable
}

// Разрешение обработки прерываний USART и установка высокого приоритета соответствующего прерывания
static void USART_EnableIRQ(USART_TypeDef* USARTx)
{
    switch((uint32_t)USARTx) {
        case ((uint32_t)USART1):
            NVIC_EnableIRQ(USART1_IRQn);
			NVIC_SetPriority(USART1_IRQn, 0);
            break;

        case ((uint32_t)USART2):
            NVIC_EnableIRQ(USART2_IRQn);
    		NVIC_SetPriority(USART2_IRQn, 0);
            break;

        case ((uint32_t)USART3):
            NVIC_EnableIRQ(USART3_IRQn);
			NVIC_SetPriority(USART3_IRQn, 0);
            break;

        case ((uint32_t)UART4):
            NVIC_EnableIRQ(UART4_IRQn);
			NVIC_SetPriority(UART4_IRQn, 0);
            break;

        case ((uint32_t)UART5):
            NVIC_EnableIRQ(UART5_IRQn);
			NVIC_SetPriority(UART5_IRQn, 0);
            break;

        case ((uint32_t)USART6):
			NVIC_EnableIRQ(USART6_IRQn);
			NVIC_SetPriority(USART6_IRQn, 0);
            break;
    }
}

/*************************************** Публичные функции ************************************************************/

// Включение и настройка модуля USARTx ( USART1 // USART2 // USART3 // UART4 // UART5 // USART6)
void USART_Enable(USART_TypeDef* USARTx, GPIO_TypeDef* GPIO_port_Tx, int GPIO_pin_Tx, GPIO_TypeDef* GPIO_port_Rx, int GPIO_pin_Rx , uint32_t baudrate)
{
	// Включение тактирования USARTx
	USART_RCC_Enable(USARTx);

	// Включение тактирования GPIO, настройка заданных пинов приемника и передатчика в режиме AF (регистры MODER и AFR)
	GPIO_Enable_USART(USARTx, GPIO_port_Tx, GPIO_port_Rx, GPIO_pin_Tx, GPIO_pin_Rx);

	// Настройка регистра BRR и включение модуля USARTx
	USART_Init(USARTx, baudrate);

	// Разрешение обработки прерываний USART и установка высокого приоритета соответствующего прерывания
    USART_EnableIRQ(USARTx);
}

