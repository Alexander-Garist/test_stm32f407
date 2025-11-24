/**
  * @file    uart.c
  * @brief   Файл содержит реализации функций UART
  */

/** Includes **********************************************************************************************************/
#include "uart.h"


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

static void USART_Init(USART_TypeDef* USARTx)
{
	// настройка регистров

}

// включение модуля USARTx ( USART1 // USART2 // USART3 // UART4 // UART5 // USART6)
void USART_Enable(USART_TypeDef* USARTx)
{
	USART_RCC_Enable(USARTx);
	USART_Init(USARTx);
}