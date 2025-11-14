/**
  * @file    gpio.h
  * @brief   Файл содержит прототипы функций GPIO
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GPIO_H__
#define __GPIO_H__

/* Includes ------------------------------------------------------------------*/
#include "CMSIS/stm32f4xx.h"

/* Инициализация портов GPIO в режиме ввода/вывода и установка/изменение
   логического уровня*/

/**
	! Функция GPIO_set_HIGH инициализирует порт GPIO как вывод и устанавливает
		на нём высокий логический уровень.
	- port - порт GPIO
	- pin - пин GPIO
*/
void GPIO_set_HIGH(GPIO_TypeDef* port, int pin);

/**
	! Функция GPIO_set_LOW инициализирует порт GPIO как вывод и устанавливает
		на нём низкий логический уровень.
	- port - порт GPIO
	- pin - пин GPIO
*/
void GPIO_set_LOW(GPIO_TypeDef* port, int pin);

/**
	! Функция GPIO_toggle_Pin инициализирует порт GPIO как вывод и меняет на нём
		значение логического уровня на противоположное.
	- port - порт GPIO
	- pin - пин GPIO
*/
void GPIO_toggle_Pin(GPIO_TypeDef* port, int pin);

/**
	! Функция GPIO_Button_Enable инициализирует порт GPIO как ввод, к которому
		подключена кнопка.
	- port - порт GPIO
	- pin - пин GPIO
*/
void GPIO_Button_Enable(GPIO_TypeDef* port, int pin);

/* Иницализация портов GPIO в режиме альтернативной функции *******************/
/**
	! Функция GPIO_Enable_I2C инициализирует порт GPIO в режиме альтернативной
		функции AF4 (I2C).
	- port - порт GPIO
	- pin - пин GPIO
*/
void GPIO_Enable_I2C(GPIO_TypeDef* port, int pin);

/**
	! Функция GPIO_Enable_SPI инициализирует порт GPIO в режиме альтернативной
		функции AF5 или AF6 (SPI) в зависимости от выбранного модуля SPI.
	- SPIx - модуль SPI
	- port - порт GPIO
	- pin - пин GPIO
*/
void GPIO_Enable_SPI(SPI_TypeDef* SPIx, GPIO_TypeDef* port, int pin);

#endif /*__GPIO_H__ */