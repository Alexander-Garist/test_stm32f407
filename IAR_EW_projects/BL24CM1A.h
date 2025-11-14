//Библиотека для EEPROM BL24CM1A
//содержит информацию об объеме памяти EEPROM и функции чтения/записи по I2C
#ifndef BL24CM1A_H
#define BL24CM1A_H

#include "i2c.h"

#define PAGE_LENGTH             (256)       //Объем одной страницы в байтах
#define PAGE_AMOUNT             (512)       //Количество страниц памяти
#define ALL_MEMORY              (131072)    //Суммарный объем памяти в байтах

//Функция чтения из EEPROM BL24CM1A
//port          - порт I2C, к которому подключена память
//device_addr   - адрес подключенной EEPROM, по которому к ней обращается МК
//reg_addr      - адрес в памяти EEPROM, с которого начнется чтение данных
//data          - массив, в который данные записываются из памяти EEPROM
//size          - объем данных (кол-во байт), которые нужно считать из EEPROM
I2C_Status_t BL24CM1A_Read(I2C_TypeDef* port, uint8_t device_addr, uint32_t reg_addr, uint8_t* data, uint16_t size);

//Функция записи в EEPROM BL24CM1A
//port          - порт I2C, к которому подключена память
//device_addr   - адрес подключенной EEPROM, по которому к ней обращается МК
//reg_addr      - адрес в памяти EEPROM, с которого начнется запись данных
//data          - массив, из которого данные записываются в память EEPROM
//size          - объем данных (кол-во байт), которые нужно записать в EEPROM
I2C_Status_t BL24CM1A_Write(I2C_TypeDef* port, uint8_t device_addr, uint32_t reg_addr, uint8_t* data, uint16_t size);
#endif
