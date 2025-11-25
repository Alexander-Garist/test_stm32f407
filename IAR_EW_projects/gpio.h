/**
  * @file    gpio.h
  * @brief   Файл содержит прототипы функций GPIO
  */

/** Define to prevent recursive inclusion *****************************************************************************/
#ifndef __GPIO_H__
#define __GPIO_H__

/** Includes **********************************************************************************************************/
#include "CMSIS/stm32f4xx.h"

/** Functions *********************************************************************************************************/

	/**
	! Функция GPIO_set_HIGH инициализирует порт GPIO как вывод и устанавливает
		на нём высокий логический уровень.
	- GPIO_port - порт GPIO
	- GPIO_pin - пин GPIO
	*/
void GPIO_set_HIGH(
	GPIO_TypeDef*	GPIO_port,
	int				GPIO_pin
);

	/**
	! Функция GPIO_set_LOW инициализирует порт GPIO как вывод и устанавливает
		на нём низкий логический уровень.
	- GPIO_port - порт GPIO
	- GPIO_pin - пин GPIO
	*/
void GPIO_set_LOW(
	GPIO_TypeDef*	GPIO_port,
	int				GPIO_pin
);

	/**
	! Функция GPIO_toggle_Pin инициализирует порт GPIO как вывод и меняет на нём
		значение логического уровня на противоположное.
	- GPIO_port - порт GPIO
	- GPIO_pin - пин GPIO
	*/
void GPIO_toggle_Pin(
	GPIO_TypeDef*	GPIO_port,
	int				GPIO_pin
);

	/**
	! Функция GPIO_Button_Enable инициализирует порт GPIO как ввод, к которому
		подключена кнопка.
	- GPIO_port - порт GPIO
	- GPIO_pin - пин GPIO
	*/
void GPIO_Button_Enable(
	GPIO_TypeDef*	GPIO_port,
	int				GPIO_pin
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
	int				GPIO_pin
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
	int				GPIO_pin
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
	GPIO_TypeDef*	GPIO_port_Rx,
	int				GPIO_pin_Tx,
	int				GPIO_pin_Rx
);

#endif /*__GPIO_H__ */