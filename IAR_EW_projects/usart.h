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