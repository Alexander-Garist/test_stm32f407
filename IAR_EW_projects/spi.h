#ifndef SPI_H
#define SPI_H

#include "CMSIS/stm32f4xx.h"
#include "systick.h"
// Определение выводов SPI1  (для SPI1 и SPI2 используется AF5)
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

// Определение выводов SPI3 (для SPI3 используется AF6)
#define SPI3_CS_PORT      GPIOA
#define SPI3_CS_PIN       15
#define SPI3_SCK_PORT     GPIOB
#define SPI3_SCK_PIN      3
#define SPI3_MISO_PORT    GPIOB
#define SPI3_MISO_PIN     4
#define SPI3_MOSI_PORT    GPIOB
#define SPI3_MOSI_PIN     5

// Макросы для управления CS, CS_LOW() начинает операцию, CS_HIGH() заканчивает
#define SPI1_CS_LOW()   (SPI1_CS_PORT->BSRR = (1 << (SPI1_CS_PIN + 16)))
#define SPI1_CS_HIGH()  (SPI1_CS_PORT->BSRR = (1 << SPI1_CS_PIN))

#define SPI2_CS_LOW()   (SPI2_CS_PORT->BSRR = (1 << (SPI2_CS_PIN + 16)))
#define SPI2_CS_HIGH()  (SPI2_CS_PORT->BSRR = (1 << SPI2_CS_PIN))

#define SPI3_CS_LOW()   (SPI3_CS_PORT->BSRR = (1 << (SPI3_CS_PIN + 16))
#define SPI3_CS_HIGH()  (SPI3_CS_PORT->BSRR = (1 << SPI3_CS_PIN))

typedef enum    //возвращаемые статусы команд SPI
{
    SPI_OK =                0,  //Функция успешно отработала
    SPI_DATA_NULL =         1,  //Ошибка массива данных для записи/чтения
    SPI_FLAG_TIMEOUT =      2,  //Ошибка ожидания установки флага
    SPI_ERROR_WRITE =       3,  //Ошибка передачи данных
    SPI_ERROR_READ =        4   //Ошибка приема данных
}SPI_Status_t;

void SPI_Enable_Pin(SPI_TypeDef* SPIx);                                         //Включение выбранного SPI (тактирование и настройка регистров)

//Прием/передача по SPI
SPI_Status_t SPI_Transmit(SPI_TypeDef* SPIx, uint8_t* data, uint32_t size);     //Отправить по SPI
SPI_Status_t SPI_Receive(SPI_TypeDef* SPIx, uint8_t* data, uint32_t size);      //Принять по SPI

#endif

