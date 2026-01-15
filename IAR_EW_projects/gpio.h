/**
  * @file    gpio.h
  * @brief   Файл содержит прототипы функций GPIO
  */

/** Define to prevent recursive inclusion *****************************************************************************/
#ifndef __GPIO_H__
#define __GPIO_H__

/** Includes **********************************************************************************************************/
#include "CMSIS/stm32f4xx.h"

/** Defines ***********************************************************************************************************/

// Маски регистра MODER
#define MODER_INPUT		0x0
#define MODER_OUTPUT	0x1
#define MODER_AF		0x2
#define MODER_ANALOG	0x3

// Маски регистра OTYPER
#define OTYPER_PUSH_PULL	0x0
#define OTYPER_OPEN_DRAIN	0x1

// Маски регистра OSPEEDR
#define OSPEEDR_LOW			0x0
#define OSPEEDR_MEDIUM		0x1
#define OSPEEDR_HIGH		0x2
#define OSPEEDR_VERY_HIGH	0x3

// Маски регистра PUPDR
#define PUPDR_NO_PUPD	0x0
#define PUPDR_PU		0x1
#define PUPDR_PD		0x2
#define PUPDR_RESERVED	0x3

// Маски регистра AFR
#define AFR_0		0x0
#define AFR_1		0x1
#define AFR_2		0x2
#define AFR_3		0x3
#define AFR_4		0x4
#define AFR_5		0x5
#define AFR_6		0x6
#define AFR_7		0x7
#define AFR_8		0x8
#define AFR_9		0x9
#define AFR_10		0xA
#define AFR_11		0xB
#define AFR_12		0xC
#define AFR_13		0xD
#define AFR_14		0xE
#define AFR_15		0xF

/** Functions *********************************************************************************************************/

	/**
	! Функция GPIO_set_HIGH инициализирует порт GPIO как вывод и устанавливает
		на нём высокий логический уровень.
	- GPIO_port - порт GPIO
	- GPIO_pin - пин GPIO
	*/
void GPIO_set_HIGH(
	GPIO_TypeDef*	GPIO_port,
	uint8_t			GPIO_pin
);

	/**
	! Функция GPIO_set_LOW инициализирует порт GPIO как вывод и устанавливает
		на нём низкий логический уровень.
	- GPIO_port - порт GPIO
	- GPIO_pin - пин GPIO
	*/
void GPIO_set_LOW(
	GPIO_TypeDef*	GPIO_port,
	uint8_t			GPIO_pin
);

	/**
	! Функция GPIO_toggle_Pin инициализирует порт GPIO как вывод и меняет на нём
		значение логического уровня на противоположное.
	- GPIO_port - порт GPIO
	- GPIO_pin - пин GPIO
	*/
void GPIO_toggle_Pin(
	GPIO_TypeDef*	GPIO_port,
	uint8_t			GPIO_pin
);

	/**
	! Функция GPIO_Button_Enable инициализирует порт GPIO как ввод, к которому
		подключена кнопка.
	- GPIO_port - порт GPIO
	- GPIO_pin - пин GPIO
	*/
void GPIO_Button_Enable(
	GPIO_TypeDef*	GPIO_port,
	uint8_t			GPIO_pin
);

/*********************** Иницализация портов GPIO в режиме альтернативной функции *************************************/
	/**
	! Функция GPIO_Enable_I2C инициализирует порт GPIO в режиме альтернативной
		функции AF4 (I2C).
	- GPIO_port - порт GPIO
	- GPIO_pin - пин GPIO
	*/
void GPIO_Enable_I2C(
	GPIO_TypeDef*	GPIO_port,
	uint8_t			GPIO_pin
);

	/**
	! Функция GPIO_Enable_SPI инициализирует порт GPIO в режиме альтернативной
		функции AF5 или AF6 (SPI) в зависимости от выбранного модуля SPI.
	- SPIx - модуль SPI
	- GPIO_port - порт GPIO
	- GPIO_pin - пин GPIO
	*/
void GPIO_Enable_SPI(
	SPI_TypeDef*	SPIx,
	GPIO_TypeDef*	GPIO_port,
	uint8_t			GPIO_pin
);

	/**
	! Функция GPIO_Enable_USART инициализирует порт GPIO в режиме альтернативной функции AF7 или AF8 (UART/USART) в
		зависимости от выбранного модуля UART/USART.
	- USARTx - модуль UART/USART.
	- GPIO_port_Tx - порт передатчика.
	- GPIO_port_Rx - порт приемника.
	- GPIO_pin_Tx - пин передатчика.
	- GPIO_pin_Rx - пин приемника.
	*/
void GPIO_Enable_USART(
	USART_TypeDef*	USARTx,
	GPIO_TypeDef*	GPIO_port_Tx,
	uint8_t			GPIO_pin_Tx,
	GPIO_TypeDef*	GPIO_port_Rx,
	uint8_t			GPIO_pin_Rx
);

#endif /*__GPIO_H__ */