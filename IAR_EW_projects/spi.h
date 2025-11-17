/**
  * @file    spi.h
  * @brief   Файл содержит прототипы функций SPI
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPI_H__
#define __SPI_H__

#include "CMSIS/stm32f4xx.h"
#include "systick.h"
// Определение выводов SPI1
#define SPI1_CS_PORT      GPIOA
#define SPI1_CS_PIN       4
#define SPI1_SCK_PORT     GPIOA
#define SPI1_SCK_PIN      5
#define SPI1_MISO_PORT    GPIOA
#define SPI1_MISO_PIN     6
#define SPI1_MOSI_PORT    GPIOA
#define SPI1_MOSI_PIN     7

// Определение выводов SPI2
#define SPI2_CS_PORT      GPIOB
#define SPI2_CS_PIN       12
#define SPI2_SCK_PORT     GPIOB
#define SPI2_SCK_PIN      13
#define SPI2_MISO_PORT    GPIOB
#define SPI2_MISO_PIN     14
#define SPI2_MOSI_PORT    GPIOB
#define SPI2_MOSI_PIN     15

// Определение выводов SPI3
#define SPI3_CS_PORT      GPIOA
#define SPI3_CS_PIN       15
#define SPI3_SCK_PORT     GPIOB
#define SPI3_SCK_PIN      3
#define SPI3_MISO_PORT    GPIOB
#define SPI3_MISO_PIN     4
#define SPI3_MOSI_PORT    GPIOB
#define SPI3_MOSI_PIN     5

/****************** Макросы для управления CS *********************************/
/********** CS_LOW() начинает операцию, CS_HIGH() заканчивает *****************/
//Для SPI1
#define SPI1_CS_LOW()   (SPI1_CS_PORT->BSRR = (1 << (SPI1_CS_PIN + 16)))
#define SPI1_CS_HIGH()  (SPI1_CS_PORT->BSRR = (1 << SPI1_CS_PIN))
//Для SPI2
#define SPI2_CS_LOW()   (SPI2_CS_PORT->BSRR = (1 << (SPI2_CS_PIN + 16)))
#define SPI2_CS_HIGH()  (SPI2_CS_PORT->BSRR = (1 << SPI2_CS_PIN))
//Для SPI3
#define SPI3_CS_LOW()   (SPI3_CS_PORT->BSRR = (1 << (SPI3_CS_PIN + 16))
#define SPI3_CS_HIGH()  (SPI3_CS_PORT->BSRR = (1 << SPI3_CS_PIN))

/************* Перечисление статусов выполнения функций SPI *******************/
typedef enum
{
    SPI_OK =                0,  //Функция успешно отработала
    SPI_DATA_NULL =         1,  //Ошибка массива данных для записи/чтения
    SPI_FLAG_TIMEOUT =      2,  //Ошибка ожидания установки флага
    SPI_ERROR_WRITE =       3,  //Ошибка передачи данных
    SPI_ERROR_READ =        4   //Ошибка приема данных
}SPI_Status_t;

/********************** Глобальные функции ************************************/

	/**
	! Включение выбранного модуля SPI (тактирование и настройка регистров)
	- SPIx - модуль SPI (SPI1, SPI2, SPI3)
	*/
void SPI_Enable_Pin(SPI_TypeDef* SPIx);

	/**
	! Функция отправки данных по шине SPI
	- SPIx - модуль SPI (SPI1, SPI2, SPI3)
	- data - указатель на массив отправляемых данных
	- size - объем передаваемых данных в байтах
	+ статус выполнения отправки данных (если отправка успешна, вернет SPI_OK)
	*/
SPI_Status_t SPI_Transmit(
	SPI_TypeDef *	SPIx,
	uint8_t *		data,
	uint32_t		size
);

	/**
	! Функция приема данных по шине SPI
	- SPIx - модуль SPI (SPI1, SPI2, SPI3)
	- data - указатель на массив, в который запишутся принятые данные
	- size - объем принимаемых данных в байтах
	+ статус выполнения приема данных (если прием успешен, вернет SPI_OK)
	*/
SPI_Status_t SPI_Receive(
	SPI_TypeDef* SPIx,
	uint8_t* data,
	uint32_t size
);

#endif /*__SPI_H__ */