#ifndef GPIO_H
#define GPIO_H

#include "CMSIS/stm32f4xx.h"
//пользовательские функции, можно использовать в других модулях
void GPIO_set_HIGH(GPIO_TypeDef* port, int pin);                                //Установить высокий уровень на выходе GPIO
void GPIO_set_LOW(GPIO_TypeDef* port, int pin);                                 //Установить низкий уровень на выходе GPIO
void GPIO_toggle_Pin(GPIO_TypeDef* port, int pin);                              //Переключить состояние вывода GPIO (HIGH/LOW)

void GPIO_Button_Enable(GPIO_TypeDef* port, int pin);                           //Определить пин как вход, к которому подключена кнопка

void GPIO_Enable_I2C(GPIO_TypeDef* port, int pin);                              //Включить пин в режиме альтернативной функции I2C
void GPIO_Enable_SPI(SPI_TypeDef* SPIx, GPIO_TypeDef* port, int pin);           //Включить пин в режиме альтернативной функции SPI
#endif