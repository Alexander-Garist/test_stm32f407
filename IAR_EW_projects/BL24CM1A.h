/**
  * @file    BL24CM1A.h
  * @brief   Библиотека для EEPROM BL24CM1A. Содержит информацию об
		объеме памяти EEPROM и функции чтения/записи по I2C.
  */

/** Define to prevent recursive inclusion *************************************/
#ifndef __BL24CM1A_H__
#define __BL24CM1A_H__

/** Includes ******************************************************************/
#include "i2c.h"

/** Defines *******************************************************************/
#define PAGE_LENGTH		(256)       //Объем одной страницы в байтах
#define PAGE_AMOUNT		(512)       //Количество страниц памяти
#define ALL_MEMORY		(131072)    //Суммарный объем памяти в байтах

/** Functions *****************************************************************/

	/**
	! Функция чтения из EEPROM BL24CM1A
	- I2Cx - выбранный модуль I2C (I2C1, I2C2, I2C3), к которому подключена
		EEPROM
	- device_addr - адрес подключенной EEPROM, по которому к ней обращается МК
	- reg_addr - адрес в памяти EEPROM, с которого начнется чтение данных
	- data - массив, в который данные записываются из памяти EEPROM
	- size - объем данных (кол-во байт), которые нужно считать из EEPROM
	return: статус выполнения чтения из EEPROM (если успешно, то I2C_OK)
	*/
I2C_Status_t BL24CM1A_Read(
	I2C_TypeDef *	I2Cx,
	uint8_t			device_addr,
	uint32_t		reg_addr,
	uint8_t *		data,
	uint16_t		size
);

	/**
	! Функция записи в EEPROM BL24CM1A
	- I2Cx - выбранный модуль I2C (I2C1, I2C2, I2C3), к которому подключена
		EEPROM
	- device_addr - адрес подключенной EEPROM, по которому к ней обращается МК
	- reg_addr - адрес в памяти EEPROM, с которого начнется запись данных
	- data - массив, из которого данные записываются в память EEPROM
	- size - объем данных (кол-во байт), которые нужно записать в EEPROM
	return: статус выполнения записи в EEPROM (если успешно, то I2C_OK)
	*/
I2C_Status_t BL24CM1A_Write(
	I2C_TypeDef *	I2Cx,
	uint8_t			device_addr,
	uint32_t		reg_addr,
	uint8_t *		data,
	uint16_t		size);

#endif /* __BL24CM1A_H__ */