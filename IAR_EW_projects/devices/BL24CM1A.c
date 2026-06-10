/**
  * @file    BL24CM1A.c
  * @brief   Файл содержит реализации функций чтения/записи BL24CM1A по I2C.
  */

/** Includes **********************************************************************************************************/
#include <string.h>
#include <stdio.h>
#include "BL24CM1A.h"
#include "systick.h"

/******************************* Пользовательские функции для EEPROM BL24CM1A *****************************************/

// Запись в EEPROM BL24CM1A
I2C_Status_t BL24CM1A_Write(I2C_TypeDef* I2Cx, uint8_t device_addr, uint32_t reg_addr, uint8_t* data, uint16_t size)
{
    I2C_Status_t status;
    uint8_t write_buffer[258];          	// Буфер записи 256 + 2 байта адреса
    uint16_t bytes_remaining = size;    	// Количество оставшихся незаписанных байт
    uint16_t data_offset = 0;           	// Смещение в массиве данных
    uint32_t current_address = reg_addr;	// Текущий адрес в памяти EEPROM

    // Вычисление адреса устройства
    uint8_t current_device_addr = device_addr;
    if (current_address > 0xFFFF) {			// Если нужно обратиться к адресу из второй половины памяти EEPROM
        current_device_addr |= 0x01;		// Бит B16 в device_addr должен стать 1
    }

	// Основной цикл записи
    while (bytes_remaining > 0)
    {
        // 1. Вычисление сколько байт можно записать на текущей странице
        uint16_t bytes_on_page = 256 - (current_address % 256);
        uint16_t bytes_to_write = (bytes_remaining < bytes_on_page) ? bytes_remaining : bytes_on_page;

        // 2. Подготовка буфера записи (адрес + данные)
        write_buffer[0] = (current_address >> 8) & 0xFF;					// Старший байт адреса
        write_buffer[1] = current_address & 0xFF;							// Младший байт адреса
        memcpy(&write_buffer[2], &data[data_offset], bytes_to_write);		// Копирование данных в буфер для отправки

        // 3. Отправка по I2C
        status = I2C_Write(I2Cx, current_device_addr, write_buffer, bytes_to_write + 2);

        if (status != I2C_OK)
        {
            //printf("Write failed at address 0x%05lX\n", current_address);		// Отладочный вывод
            return status;
        }

        // 4. Обновление счетчиков памяти и индексов в массиве
        bytes_remaining -= bytes_to_write;
        data_offset += bytes_to_write;
        current_address += bytes_to_write;

        // 5. Нужно ли сменить B16 в device_addr
        if ((current_address & 0xFFFF) == 0)
        {
            current_device_addr = device_addr | ((current_address >> 16) & 0x01);
        }

        // 6. Задержка только если есть еще данные
        if (bytes_remaining > 0) delay_ms(5);	// Задержка для внутренней записи EEPROM
    }

	// В случае успешного завершения работы функции статус I2C_OK
    return I2C_OK;
}

// Чтение из EEPROM BL24CM1A
I2C_Status_t BL24CM1A_Read(I2C_TypeDef* I2Cx, uint8_t device_addr, uint32_t reg_addr, uint8_t* data, uint16_t size)
{
    I2C_Status_t status;
    uint8_t addr_buffer[2];                     // Буфер отправляемых данных 2 байта для адреса в памяти EEPROM
    if (data == NULL) return I2C_DATA_NULL;

    // 1. Подготовка адреса памяти (2 байта)
    if (reg_addr > 0xFFFF) device_addr |= (1 << 0);
    addr_buffer[0] = (reg_addr >> 8) & 0xFF;
    addr_buffer[1] = reg_addr & 0xFF;

    uint16_t current_address = reg_addr;        // Текущий адрес в памяти EEPROM
    uint16_t bytes_remaining = size;            // Количество байт, которые осталось прочитать
    uint16_t data_offset = 0;                   // Сдвиг в массиве data

    while (bytes_remaining > 0) {
        // 2. Вычисление сколько байт можно прочитать с текущей страницы
        uint16_t bytes_on_page = 256 - (current_address % 256); // До конца страницы
        uint16_t bytes_to_read = (bytes_remaining < bytes_on_page) ? bytes_remaining : bytes_on_page;

        // 3. Обновление адреса для текущего блока
        addr_buffer[0] = (current_address >> 8) & 0xFF;
        addr_buffer[1] = current_address & 0xFF;

        // 4. Фаза записи: отправка адреса памяти
        status = I2C_Write(I2Cx, device_addr, addr_buffer, 2);
        if (status != I2C_OK)
        {
            //printf("Write address failed at 0x%04X\n", current_address);		// Отладочный вывод
            return status;
        }

        // 5. Короткая задержка между операциями
        delay_ms(1);

        // 6. Фаза чтения: чтение данных используя общую функцию
        status = I2C_Read(I2Cx, device_addr, data + data_offset, bytes_to_read);
        if (status != I2C_OK)
        {
            //printf("Read data failed at 0x%04X\n", current_address);			// Отладочный вывод
            return status;
        }

        // 7. Обновление счетчиков
        bytes_remaining -= bytes_to_read;
        data_offset += bytes_to_read;
        current_address += bytes_to_read;

        // 8. Задержка между страницами
        if (bytes_remaining > 0) delay_ms(5);
    }
    return I2C_OK;
}