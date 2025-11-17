/**
  * @file    FM25Q08B.h
  * @brief   Библиотека для модуля памяти FM25Q08B. Содержит информацию о модуле памяти, встроенные команды
		и функции чтения/записи по SPI.
  */

/** Define to prevent recursive inclusion *****************************************************************************/
#ifndef __FM25Q08B_H__
#define __FM25Q08B_H__

/** Includes **********************************************************************************************************/
#include "spi.h"

/** Defines ***********************************************************************************************************/

// Информация о модуле памяти
#define FLASH_START_ADDRESS     0x000000    //начальный адрес памяти FM25Q08B
#define FLASH_PAGE_SIZE         256         //максимальный объем данных, которые можно записать в память за 1 операцию (1 страница)
#define FLASH_SECTOR_SIZE       4096        //Размер сектора 4KB = 4096 байт
#define FLASH_BLOCK_32KB_SIZE   32768       //Размер блока 32KB = 32768 байт
#define FLASH_BLOCK_64KB_SIZE   65536       //Размер блока 64KB = 65536 байт
#define FLASH_ALL_SIZE          1048576     //Размер всей памяти 1024KB = 1048576 байт

/** Встроенные команды FM25Q08B ***************************************************************************************/

// Разрешить/запретить запись в память
#define FLASH_CMD_WRITE_ENABLE          0x06
#define FLASH_CMD_WRITE_DISABLE         0x04

// Чтение статусных регистров
#define FLASH_CMD_READ_STATUS_REG_1     0x05
#define FLASH_CMD_READ_STATUS_REG_2     0x35

// Запись в статусные регистры
#define FLASH_CMD_WRITE_STATUS_REG      0x01

// Команды стирания памяти
#define FLASH_CMD_SECTOR_ERASE_4KB      0x20
#define FLASH_CMD_BLOCK_ERASE_32KB      0x52
#define FLASH_CMD_BLOCK_ERASE_64KB      0xD8
#define FLASH_CMD_CHIP_ERASE            0xC7

// Включение/выключение энергосберегающего режима
#define FLASH_CMD_POWER_DOWN            0xB9
#define FLASH_CMD_RELEASE_POWER_DOWN    0xAB

// Команды чтения/записи
#define FLASH_CMD_READ_DATA             0x03
#define FLASH_CMD_PAGE_PROGRAM          0x02

// Команды чтения идентификаторов модуля памяти
#define FLASH_CMD_READ_UNIQUE_ID        0x4B	// 8 байтный уникальный серийный номер
#define FLASH_CMD_READ_JEDEC_ID         0x9F	// 3 байтный ID
#define FLASH_CMD_READ_MANUF_DEVICE_ID  0x90	// 2 байтный ID

// Команды для программного сброса модуля
#define FLASH_CMD_ENABLE_RESET          0x66//FM25Q08B_Reset(SPI_TypeDef* SPIx);
#define FLASH_CMD_RESET                 0x99//FM25Q08B_Reset(SPI_TypeDef* SPIx);

/******************************* Перечисление статусов выполнения команл FM25Q08B *************************************/
typedef enum
{
    FM25Q08B_OK =                   0,  //Функция успешно отработала
    FM25Q08B_ERROR_WRITE =          1,  //Ошибка передачи данных
    FM25Q08B_ERROR_READ =           2,  //Ошибка приема данных
    FM25Q08B_ERROR_ERASE =          3,  //Ошибка стирания памяти
    FM25Q08B_ERROR_WRITE_PAGE =     4,  //Ошибка попытки записать больше данных, чем одна страница
    FM25Q08B_ERROR_WRITE_MEMORY =   5   //Ошибка попытки записать больше данных, чем может поместиться в объем памяти FM25Q08B
}FM25Q08B_Status_t;

/********************************** Пользовательские функции для flash памяти *****************************************/

	/**
	! Программный сброс flash памяти.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	*/
void FM25Q08B_Reset(SPI_TypeDef* SPIx);

/******************************** Считать идентификаторы модуля FM25Q08B **********************************************/

	/**
	! Считать Unique_ID чипа памяти.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	- Unique_ID - указатель на массив, в который запишется Unique_ID модуля памяти.
	*/
void FM25Q08B_Read_Unique_ID(
	SPI_TypeDef *	SPIx,
	uint8_t *		Unique_ID
);

	/**
	! Считать JEDEC_ID чипа памяти.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	return: JEDEC_ID.
	*/
uint32_t FM25Q08B_Read_JEDEC_ID(SPI_TypeDef* SPIx);

	/**
	! Считать Manufacturer_ID чипа памяти.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	return: Manufacturer_ID.
	*/
uint16_t FM25Q08B_Read_Manufacturer_ID(SPI_TypeDef* SPIx);

/************************** Считать STATUS REGISTER 1 и STATUS REGISTER 2 модуля FM25Q08B *****************************/

	/**
	! Считать STATUS REGISTER 1 (S7-S0).
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	return: STATUS REGISTER 1.
	*/
uint8_t FM25Q08B_Read_Status_Register_1(SPI_TypeDef* SPIx);

	/**
	! Считать STATUS REGISTER 2 (S15-S8).
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	return: STATUS REGISTER 2.
	*/
uint8_t FM25Q08B_Read_Status_Register_2(SPI_TypeDef* SPIx);

/**************************************** Разрешить/запретить запись в память *****************************************/

	/**
	! Функция FM25Q08B_Write_Enable разрешает запись в память, нужно вызывать перед каждой операцией записи в память
		или очистки памяти.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	return: статус выполнения команды.
	*/
FM25Q08B_Status_t FM25Q08B_Write_Enable(SPI_TypeDef* SPIx);

	/**
	! Функция FM25Q08B_Write_Disable запрещает запись в память.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	return: статус выполнения команды.
	*/
FM25Q08B_Status_t FM25Q08B_Write_Disable(SPI_TypeDef* SPIx);

/**************************************** Функции стирания памяти *****************************************************/

	/**
	! Функция FM25Q08B_Sector_Erase_4KB очищает сектор памяти 4KB.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	- memory_addr - начальный адрес сектора.
	return: статус выполнения команды.
	*/
FM25Q08B_Status_t FM25Q08B_Sector_Erase_4KB(
	SPI_TypeDef *	SPIx,
	uint32_t		memory_addr
);

	/**
	! Функция FM25Q08B_Block_Erase_32KB очищает сектор памяти 32KB.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	- memory_addr - начальный адрес блока.
	return: статус выполнения команды.
	*/
FM25Q08B_Status_t FM25Q08B_Block_Erase_32KB(
	SPI_TypeDef *	SPIx,
	uint32_t		memory_addr
);

	/**
	! Функция FM25Q08B_Block_Erase_64KB очищает сектор памяти 64KB.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	- memory_addr - начальный адрес блока.
	return: статус выполнения команды.
	*/
FM25Q08B_Status_t FM25Q08B_Block_Erase_64KB(
	SPI_TypeDef *	SPIx,
	uint32_t		memory_addr
);

	/**
	! Функция FM25Q08B_Chip_Erase очищает всю память чипа 1024KB.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	return: статус выполнения команды.
	*/
FM25Q08B_Status_t FM25Q08B_Chip_Erase(SPI_TypeDef* SPIx);

/*********************************** to do ************************************/
// дописать функции
// FLASH_CMD_WRITE_STATUS_REG
// FLASH_CMD_POWER_DOWN
// FLASH_CMD_RELEASE_POWER_DOWN

/*************************************** Функции чтения/записи FM25Q08B ***********************************************/
	/**
	! Чтение из памяти FM25Q08B по указанному адресу
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	- memory_addr - адрес в памяти FM25Q08B, начиная с которого нужно считать данные.
	- data - указатель на массив, в который данные считываются из FM25Q08B.
	- size - количество считанных байт данных.
	return: статус выполнения чтения из FM25Q08B.
	*/
FM25Q08B_Status_t FM25Q08B_Read(
	SPI_TypeDef *	SPIx,
	uint32_t		memory_addr,
	uint8_t *		data,
	uint32_t		size
);

	/**
	! Запись в память FM25Q08B по указанному адресу
	- SPIx - модуль SPI (SPI1, SPI2, SPI3)
	- memory_addr - адрес в памяти FM25Q08B, начиная с которого нужно записать данные.
	- data - указатель на массив, данные из которого записываются в FM25Q08B.
	- size - количество записанных байт данных.
	return: статус выполнения записи в FM25Q08B.
	*/
FM25Q08B_Status_t FM25Q08B_Write(
	SPI_TypeDef *	SPIx,
	uint32_t		memory_addr,
	uint8_t *		data,
	uint32_t		size
);

#endif /* __FM25Q08B_H__ */