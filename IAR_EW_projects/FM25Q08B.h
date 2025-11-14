//Библиотека для модуля памяти FM25Q08B
//содержит информацию о модуле памяти, встроенные команды и функции чтения/записи по SPI
#ifndef FM25Q08B_H
#define FM25Q08B_H
#include <stdint.h>

#include "spi.h"

#define FLASH_START_ADDRESS     0x000000    //начальный адрес памяти FM25Q08B
#define FLASH_PAGE_SIZE         256         //максимальный объем данных, которые можно записать в память за 1 операцию (1 страница)
#define FLASH_SECTOR_SIZE       4096        //Размер сектора 4KB = 4096 байт
#define FLASH_BLOCK_32KB_SIZE   32768       //Размер блока 32KB = 32768 байт
#define FLASH_BLOCK_64KB_SIZE   65536       //Размер блока 64KB = 65536 байт
#define FLASH_ALL_SIZE          1048576     //Размер всей памяти 1024KB = 1048576 байт

// Команды для FM25Q08B
#define FLASH_CMD_WRITE_ENABLE          0x06
#define FLASH_CMD_WRITE_DISABLE         0x04

#define FLASH_CMD_READ_STATUS_REG_1     0x05
#define FLASH_CMD_READ_STATUS_REG_2     0x35

#define FLASH_CMD_WRITE_STATUS_REG      0x01

#define FLASH_CMD_SECTOR_ERASE_4KB      0x20
#define FLASH_CMD_BLOCK_ERASE_32KB      0x52
#define FLASH_CMD_BLOCK_ERASE_64KB      0xD8
#define FLASH_CMD_CHIP_ERASE            0xC7

#define FLASH_CMD_POWER_DOWN            0xB9
#define FLASH_CMD_RELEASE_POWER_DOWN    0xAB


#define FLASH_CMD_READ_DATA             0x03
#define FLASH_CMD_PAGE_PROGRAM          0x02

#define FLASH_CMD_READ_UNIQUE_ID        0x4B// 8 байтный уникальный серийный номер
#define FLASH_CMD_READ_JEDEC_ID         0x9F// 3 байта
#define FLASH_CMD_READ_MANUF_DEVICE_ID  0x90// 2 байта

#define FLASH_CMD_ENABLE_RESET          0x66//FM25Q08B_Reset(SPI_TypeDef* SPIx);
#define FLASH_CMD_RESET                 0x99//FM25Q08B_Reset(SPI_TypeDef* SPIx);

typedef enum    //возвращаемые статусы команд FM25Q08B
{
    FM25Q08B_OK =                   0,  //Функция успешно отработала
    FM25Q08B_ERROR_WRITE =          1,  //Ошибка передачи данных
    FM25Q08B_ERROR_READ =           2,  //Ошибка приема данных
    FM25Q08B_ERROR_ERASE =          3,  //Ошибка стирания памяти
    FM25Q08B_ERROR_WRITE_PAGE =     4,  //Ошибка попытки записать больше данных, чем одна страница
    FM25Q08B_ERROR_WRITE_MEMORY =   5   //Ошибка попытки записать больше данных, чем может поместиться в объем памяти FM25Q08B
}FM25Q08B_Status_t;

//========Пользовательские функции для flash памяти=======
void FM25Q08B_Reset(SPI_TypeDef* SPIx);                                         //Программный сброс flash памяти

void FM25Q08B_Read_Unique_ID(SPI_TypeDef* SPIx, uint8_t* Unique_ID);            //Считать Unique_ID чипа памяти
uint32_t FM25Q08B_Read_JEDEC_ID(SPI_TypeDef* SPIx);                             //Считать JEDEC_ID чипа памяти
uint16_t FM25Q08B_Read_Manufacturer_ID(SPI_TypeDef* SPIx);                      //Считать Manufacturer_ID чипа памяти

uint8_t FM25Q08B_Read_Status_Register_1(SPI_TypeDef* SPIx);                     //Считать STATUS REGISTER 1 (S7-S0)
uint8_t FM25Q08B_Read_Status_Register_2(SPI_TypeDef* SPIx);                     //Считать STATUS REGISTER 2 (S15-S8)

FM25Q08B_Status_t FM25Q08B_Write_Enable(SPI_TypeDef* SPIx);                                  //Разрешить запись, нужно вызывать перед каждой операцией записи в память/очистки памяти
FM25Q08B_Status_t FM25Q08B_Write_Disable(SPI_TypeDef* SPIx);                                 //Запретить запись

FM25Q08B_Status_t FM25Q08B_Sector_Erase_4KB(SPI_TypeDef* SPIx, uint32_t memory_addr);        //Очистить сектор памяти 4KB, memory_addr - начальный адрес сектора
FM25Q08B_Status_t FM25Q08B_Block_Erase_32KB(SPI_TypeDef* SPIx, uint32_t memory_addr);        //Очистить блок памяти 32KB, memory_addr - начальный адрес блока
FM25Q08B_Status_t FM25Q08B_Block_Erase_64KB(SPI_TypeDef* SPIx, uint32_t memory_addr);        //Очистить блок памяти 64KB, memory_addr - начальный адрес блока
FM25Q08B_Status_t FM25Q08B_Chip_Erase(SPI_TypeDef* SPIx);                                    //Очистить всю память чипа 1024KB

//осталось дописать функции
//FLASH_CMD_WRITE_STATUS_REG
//FLASH_CMD_POWER_DOWN
//FLASH_CMD_RELEASE_POWER_DOWN

FM25Q08B_Status_t FM25Q08B_Read(SPI_TypeDef* SPIx, uint32_t memory_addr, uint8_t* data, uint32_t size);      //Чтение из памяти FM25Q08B по указанному адресу
FM25Q08B_Status_t FM25Q08B_Write(SPI_TypeDef* SPIx, uint32_t memory_addr, uint8_t* data, uint32_t size);     //Запись в память FM25Q08B по указанному адресу
#endif

