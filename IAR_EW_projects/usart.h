/**
  * @file    usart.h
  * @brief   Файл содержит прототипы функций UART/USART
  */

/** Define to prevent recursive inclusion *****************************************************************************/
#ifndef __USART_H__
#define __USART_H__

/** Includes **********************************************************************************************************/
#include "CMSIS/stm32f4xx.h"

/** Defines ***********************************************************************************************************/
// Внешние переменные для приема команд
extern volatile char usart_rx_buffer[64];
extern volatile uint8_t usart_rx_index;
extern volatile uint8_t usart_command_ready;
/** Functions *********************************************************************************************************/

	/**
	! Включение и настройка модуля UART/USART.
	- USARTx - выбранный модуль UART/USART.
	- GPIO_port_Tx - порт GPIO передатчика.
	- GPIO_pin_Tx - пин GPIO передатчика.
	- GPIO_port_Rx - порт GPIO приемника.
	- GPIO_pin_Rx - пин GPIO приемника.
	- baudrate - желаемая скорость передачи (бит/с).
	*/
void USART_Enable(
	USART_TypeDef*	USARTx,
	GPIO_TypeDef*	GPIO_port_Tx,
	int				GPIO_pin_Tx,
	GPIO_TypeDef*	GPIO_port_Rx,
	int				GPIO_pin_Rx,
	uint32_t		baudrate
);

/*********************************** Функции отправки по UART/USART ***************************************************/

	/**
	! Отправка одного символа.
	- USARTx - выбранный модуль UART/USART.
	- symbol - отправляемый символ.
	*/
void USART_Send_Char(USART_TypeDef* USARTx, char symbol);

	/**
	! Отправка строки.
	- USARTx - выбранный модуль UART/USART.
	- str - отправляемая строка (указатель на константный массив символов).
	*/
void USART_Send_String(USART_TypeDef* USARTx, const char* str);

	/**
	! Отправка числа.
	- USARTx - выбранный модуль UART/USART.
	- number - отправляемое число.
	*/
void USART_Send_Number(USART_TypeDef* USARTx, uint32_t number);

/********************************** Функции приема по UART/USART ******************************************************/

	/**
	! Прием одного символа без ожидания.
	- USARTx - выбранный модуль UART/USART.
	- symbol - принимаемый символ.
	return: 1 если символ принят, 0 если нет данных
	*/
uint8_t USART_Receive_Char(USART_TypeDef* USARTx, char* symbol);

	/**
	! Проверка наличия принятых данных.
	return: 1 если данные приняты, 0 если нет.
	*/
uint8_t USART_Is_Data_Received(void);

	/**
	! Очистка буфера приема
	*/
void USART_Clear_Buffer(void);

/****************************** Обработчики прерываний UART/USART *****************************************************/

	/**
	! Обработчик прерывания USART1
	*/
void USART1_IRQHandler(void);

	/**
	! Обработчик прерывания USART2
	*/
void USART2_IRQHandler(void);

	/**
	! Обработчик прерывания USART3
	*/
void USART3_IRQHandler(void);

	/**
	! Обработчик прерывания UART4
	*/
void UART4_IRQHandler(void);

	/**
	! Обработчик прерывания UART5
	*/
void UART5_IRQHandler(void);

	/**
	! Обработчик прерывания USART6
	*/
void USART6_IRQHandler(void);

#endif /*__USART_H__ */