/**
  * @file    FM25Q08B.c
  * @brief   Файл содержит реализации функций выполнения встроенных команд FM25Q08B.
  */

/** Includes **********************************************************************************************************/
#include <string.h>
#include "FM25Q08B.h"
#include "systick.h"

	/**
	! Статическая функция ожидания завершения операции, т.е. пока бит WIP не станет 0.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	*/
static void FM25Q08B_Wait_End_Operation(SPI_TypeDef* SPIx)
{
    uint8_t STATUS_REGISTER_1 = FM25Q08B_Read_Status_Register_1(SPIx);
    while (STATUS_REGISTER_1 & 0x1)
    {
        STATUS_REGISTER_1 = FM25Q08B_Read_Status_Register_1(SPIx);
    }
}

/********************************** Пользовательские функции для flash памяти *****************************************/

	/**
	! Программный сброс flash памяти.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	*/
void FM25Q08B_Reset(SPI_TypeDef* SPIx)
{
    uint8_t transmitted_data[2] = {FLASH_CMD_ENABLE_RESET, FLASH_CMD_RESET};				// Отправляемые данные: 2 встроенные команды
    SPI2_CS_LOW();																			// CS_LOW() начинает операцию
    SPI_Transmit(SPIx, transmitted_data, 2);												// Отправка встроенных команд по SPI
    SPI2_CS_HIGH();																			// CS_HIGH() заканчивает операцию
    delay_ms(1);																			// Блокирующая задержка 1 мс (по даташиту ~30 мкс требуется для сброса чипа, в это время команды не принимаются)
}

/**************************** Считывание идентификаторов модуля FM25Q08B **********************************************/

	/**
	! Считать Unique_ID чипа памяти.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	- Unique_ID - указатель на массив, в который запишется Unique_ID модуля памяти.
	*/
void FM25Q08B_Read_Unique_ID(SPI_TypeDef* SPIx, uint8_t* Unique_ID)
{
    uint8_t transmitted_data[5] = { FLASH_CMD_READ_UNIQUE_ID, 0x11, 0x22, 0x66, 0x88 };     // Отправляемые данные: встроенная команда + 4 байта мусора ( п. 11.33 стр 51)
    SPI2_CS_LOW();																			// CS_LOW() начинает операцию
    SPI_Transmit(SPIx, transmitted_data, 5);												// Отправка команды по SPI
    SPI_Receive(SPIx, Unique_ID, 8);														// Прием уникального ID по SPI
    SPI2_CS_HIGH();																			// CS_HIGH() заканчивает операцию
}

	/**
	! Считать JEDEC_ID чипа памяти.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	return: JEDEC_ID.
	*/
uint32_t FM25Q08B_Read_JEDEC_ID(SPI_TypeDef* SPIx)
{
    uint8_t transmitted_data = FLASH_CMD_READ_JEDEC_ID;									// Отправляемые данные: встроенная команда ( п. 11.34 стр 52)
    uint8_t received_data[3];															// Принимаемые данные: 3-байтный Manufacturer ID / Memory type ID / Capacity ID
    SPI2_CS_LOW();																		// CS_LOW() начинает операцию
    SPI_Transmit(SPIx, &transmitted_data, 1);											// В случае ошибки передачи команды завершается работа функции со значением  -1
    SPI_Receive(SPIx, received_data, 3);												// В случае ошибки чтения значения идентификатора завершается работа функции со значением  -1
    SPI2_CS_HIGH();																		// CS_HIGH() заканчивает операцию

	// Возвращаемое значение: Manufacturer ID / Memory type ID / Capacity ID
    return ((received_data[0] << 16)|(received_data[1] << 8)|received_data[2]);
}

	/**
	! Считать Manufacturer_ID чипа памяти.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	return: Manufacturer_ID.
	*/
uint16_t FM25Q08B_Read_Manufacturer_ID(SPI_TypeDef* SPIx)
{
    uint8_t transmitted_data[4] = { FLASH_CMD_READ_MANUF_DEVICE_ID, 0x00, 0x00, 0x00 }; // Отправляемые данные: встроенная команда + 24-битный адрес 000000h( п. 11.30 стр 48)
    uint8_t received_data[2];															// Принимаемые данные: 2-байтный Manufacturer/ Device ID
    SPI2_CS_LOW();																		// CS_LOW() начинает операцию
    if (SPI_Transmit(SPIx, transmitted_data, 4) != SPI_OK) return -1;					// В случае ошибки передачи команды завершается работа функции со значением  -1
    if (SPI_Receive(SPIx, received_data, 2) != SPI_OK) return -1;						// В случае ошибки чтения значения идентификатора завершается работа функции со значением  -1
    SPI2_CS_HIGH();																		// CS_HIGH() заканчивает операцию

	// Возвращаемое значение: Manufacturer ID / Device ID
    return ((received_data[0] << 8)|received_data[1]);
}

/************************** Считать STATUS REGISTER 1 и STATUS REGISTER 2 модуля FM25Q08B *****************************/

	/**
	! Считать STATUS REGISTER 1 (S7-S0).
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	return: STATUS REGISTER 1.
	*/
uint8_t FM25Q08B_Read_Status_Register_1(SPI_TypeDef* SPIx)
{
    uint8_t transmitted_data = FLASH_CMD_READ_STATUS_REG_1;					// Отправляемые данные: встроенная команда
    uint8_t STATUS_REGISTER_1 = 0;											// Начальное значение регистра 0
    SPI2_CS_LOW();															// CS_LOW() начинает операцию
    if (SPI_Transmit(SPIx, &transmitted_data, 1) != SPI_OK) return -1;		// В случае ошибки передачи команды завершается работа функции со значением регистра -1
    if (SPI_Receive(SPIx, &STATUS_REGISTER_1, 1) != SPI_OK) return -1;		// В случае ошибки чтения значения регистра завершается работа функции со значением регистра -1
    SPI2_CS_HIGH();															// CS_HIGH() заканчивает операцию

    return STATUS_REGISTER_1;
}

	/**
	! Считать STATUS REGISTER 2 (S15-S8).
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	return: STATUS REGISTER 2.
	*/
uint8_t FM25Q08B_Read_Status_Register_2(SPI_TypeDef* SPIx)
{
    uint8_t transmitted_data = FLASH_CMD_READ_STATUS_REG_2;					// Отправляемые данные: встроенная команда
    uint8_t STATUS_REGISTER_2 = 0;											// Начальное значение регистра 0
    SPI2_CS_LOW();															// CS_LOW() начинает операцию
    if (SPI_Transmit(SPIx, &transmitted_data, 1) != SPI_OK) return -1;		// В случае ошибки передачи команды завершается работа функции со значением регистра -1
    if (SPI_Receive(SPIx, &STATUS_REGISTER_2, 1) != SPI_OK) return -1;		// В случае ошибки чтения значения регистра завершается работа функции со значением регистра -1
    SPI2_CS_HIGH();															// CS_HIGH() заканчивает операцию

    return STATUS_REGISTER_2;
}

//===Разрешить/запретить запись в память чипа===============================================================================================================================

	/**
	! Функция FM25Q08B_Write_Enable разрешает запись в память, нужно вызывать перед каждой операцией записи в память
		или очистки памяти.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	return: статус выполнения команды.
	*/
FM25Q08B_Status_t FM25Q08B_Write_Enable(SPI_TypeDef* SPIx)
{
    uint8_t transmitted_data = FLASH_CMD_WRITE_ENABLE;										// Отправляемые данные: встроенная команда
    SPI2_CS_LOW();																			// CS_LOW() начинает операцию
    if (SPI_Transmit(SPIx, &transmitted_data, 1) != SPI_OK) return FM25Q08B_ERROR_WRITE;	// В случае ошибки передачи команды завершается работа функции со статусом ошибки
    SPI2_CS_HIGH();																			// CS_HIGH() заканчивает операцию

	// В случае успешного завершения статус FM25Q08B_OK
    return FM25Q08B_OK;
}

	/**
	! Функция FM25Q08B_Write_Disable запрещает запись в память.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	return: статус выполнения команды.
	*/
FM25Q08B_Status_t FM25Q08B_Write_Disable(SPI_TypeDef* SPIx)
{
    uint8_t transmitted_data = FLASH_CMD_WRITE_DISABLE;										// Отправляемые данные: встроенная команда
    SPI2_CS_LOW();																			// CS_LOW() начинает операцию
    if (SPI_Transmit(SPIx, &transmitted_data, 1) != SPI_OK) return FM25Q08B_ERROR_WRITE;	// В случае ошибки передачи команды завершается работа функции со статусом ошибки
    SPI2_CS_HIGH();																			// CS_HIGH() заканчивает операцию

	// В случае успешного завершения статус FM25Q08B_OK
    return FM25Q08B_OK;
}

/**************************************** Функции стирания памяти *****************************************************/

	/**
	! Функция FM25Q08B_Sector_Erase_4KB очищает сектор памяти 4KB.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	- memory_addr - начальный адрес сектора.
	return: статус выполнения команды.
	*/
FM25Q08B_Status_t FM25Q08B_Sector_Erase_4KB(SPI_TypeDef* SPIx, uint32_t memory_addr)
{
	// Перед любой операцией очистки памяти вызывается инструкция Write_Enable
    if (FM25Q08B_Write_Enable(SPIx) != FM25Q08B_OK) return FM25Q08B_ERROR_ERASE;

	// Формирование отправляемых данных: встроенная команда + 3 байта адреса в памяти
    uint8_t transmitted_data[4];
    transmitted_data[0] = FLASH_CMD_SECTOR_ERASE_4KB;	// 0 байт: встроенная команда
    transmitted_data[1] = (memory_addr >> 16) & 0xFF;	// 1 байт: 0 байт адреса памяти
    transmitted_data[2] = (memory_addr >> 8) & 0xFF;	// 2 байт: 1 байт адреса памяти
    transmitted_data[3] = memory_addr & 0xFF;			// 3 байт: 2 байт адреса памяти

    SPI2_CS_LOW();																			// CS_LOW() начинает операцию
    if (SPI_Transmit(SPIx, transmitted_data, 4) != SPI_OK) return FM25Q08B_ERROR_ERASE;		// В случае ошибки передачи команды завершается работа функции со статусом ошибки
    SPI2_CS_HIGH();																			// CS_HIGH() заканчивает операцию
    FM25Q08B_Wait_End_Operation(SPIx);														// Ожидание завершения операции

	// В случае успешного завершения статус FM25Q08B_OK
    return FM25Q08B_OK;
}

	/**
	! Функция FM25Q08B_Block_Erase_32KB очищает сектор памяти 32KB.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	- memory_addr - начальный адрес блока.
	return: статус выполнения команды.
	*/
FM25Q08B_Status_t FM25Q08B_Block_Erase_32KB(SPI_TypeDef* SPIx, uint32_t memory_addr)         //Очистить блок памяти 32KB, memory_addr - начальный адрес блока
{
	// Перед любой операцией очистки памяти вызывается инструкция Write_Enable
    if (FM25Q08B_Write_Enable(SPIx) != FM25Q08B_OK) return FM25Q08B_ERROR_ERASE;

	// Формирование отправляемых данных: встроенная команда + 3 байта адреса в памяти
    uint8_t transmitted_data[4];
    transmitted_data[0] = FLASH_CMD_BLOCK_ERASE_32KB;	// 0 байт: встроенная команда
    transmitted_data[1] = (memory_addr >> 16) & 0xFF;	// 1 байт: 0 байт адреса памяти
    transmitted_data[2] = (memory_addr >> 8) & 0xFF;	// 2 байт: 1 байт адреса памяти
    transmitted_data[3] = memory_addr & 0xFF;			// 3 байт: 2 байт адреса памяти

    SPI2_CS_LOW();																			// CS_LOW() начинает операцию
    if (SPI_Transmit(SPIx, transmitted_data, 4) != SPI_OK) return FM25Q08B_ERROR_ERASE;		// В случае ошибки передачи команды завершается работа функции со статусом ошибки
    SPI2_CS_HIGH();																			// CS_HIGH() заканчивает операцию
    FM25Q08B_Wait_End_Operation(SPIx);														// Ожидание завершения операции

	// В случае успешного завершения статус FM25Q08B_OK
    return FM25Q08B_OK;
}

	/**
	! Функция FM25Q08B_Block_Erase_64KB очищает сектор памяти 64KB.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	- memory_addr - начальный адрес блока.
	return: статус выполнения команды.
	*/
FM25Q08B_Status_t FM25Q08B_Block_Erase_64KB(SPI_TypeDef* SPIx, uint32_t memory_addr)
{
	// Перед любой операцией очистки памяти вызывается инструкция Write_Enable
    if(FM25Q08B_Write_Enable(SPIx) != FM25Q08B_OK) return FM25Q08B_ERROR_ERASE;

	// Формирование отправляемых данных: встроенная команда + 3 байта адреса в памяти
    uint8_t transmitted_data[4];
    transmitted_data[0] = FLASH_CMD_BLOCK_ERASE_64KB;	// 0 байт: встроенная команда
    transmitted_data[1] = (memory_addr >> 16) & 0xFF;	// 1 байт: 0 байт адреса памяти
    transmitted_data[2] = (memory_addr >> 8) & 0xFF;	// 2 байт: 1 байт адреса памяти
    transmitted_data[3] = memory_addr & 0xFF;			// 3 байт: 2 байт адреса памяти

    SPI2_CS_LOW();																			// CS_LOW() начинает операцию
    if (SPI_Transmit(SPIx, transmitted_data, 4) != SPI_OK) return FM25Q08B_ERROR_ERASE;		// В случае ошибки передачи команды завершается работа функции со статусом ошибки
    SPI2_CS_HIGH();																			// CS_HIGH() заканчивает операцию
    FM25Q08B_Wait_End_Operation(SPIx);														// Ожидание завершения операции

	// В случае успешного завершения статус FM25Q08B_OK
    return FM25Q08B_OK;
}

	/**
	! Функция FM25Q08B_Chip_Erase очищает всю память чипа 1024KB.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	return: статус выполнения команды.
	*/	/**
	! Функция FM25Q08B_Chip_Erase очищает всю память чипа 1024KB.
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	return: статус выполнения команды.
	*/
FM25Q08B_Status_t FM25Q08B_Chip_Erase(SPI_TypeDef* SPIx)
{
	// Перед любой операцией очистки памяти вызывается инструкция Write_Enable
    if (FM25Q08B_Write_Enable(SPIx) != FM25Q08B_OK) return FM25Q08B_ERROR_ERASE;

    uint8_t transmitted_data = FLASH_CMD_CHIP_ERASE;										// Отправка встроенной команды FM25Q08B
    SPI2_CS_LOW();																			// CS_LOW() начинает операцию
    if (SPI_Transmit(SPIx, &transmitted_data, 1) != SPI_OK) return FM25Q08B_ERROR_ERASE;	// В случае ошибки передачи команды завершается работа функции со статусом ошибки
    SPI2_CS_HIGH();																			// CS_HIGH() заканчивает операцию
    FM25Q08B_Wait_End_Operation(SPIx);														// Ожидание завершения операции

	// В случае успешного завершения статус FM25Q08B_OK
    return FM25Q08B_OK;
}

/*********************************************** Чтение/запись ********************************************************/

	/**
	! Чтение из памяти FM25Q08B по указанному адресу
	- SPIx - модуль SPI (SPI1, SPI2, SPI3).
	- memory_addr - адрес в памяти FM25Q08B, начиная с которого нужно считать данные.
	- data - указатель на массив, в который данные считываются из FM25Q08B.
	- size - количество считанных байт данных.
	return: статус выполнения чтения из FM25Q08B.
	*/
FM25Q08B_Status_t FM25Q08B_Read(SPI_TypeDef* SPIx, uint32_t memory_addr, uint8_t* data, uint32_t size)
{
    if (data == NULL) return FM25Q08B_ERROR_READ;

	// Формирование отправляемых данных: встроенная команда + 3 байта адреса в памяти
    uint8_t transmitted_data[4];
    transmitted_data[0] = FLASH_CMD_READ_DATA;			// 0 байт: встроенная команда
    transmitted_data[1] = (memory_addr >> 16) & 0xFF;	// 1 байт: 0 байт адреса памяти
    transmitted_data[2] = (memory_addr >> 8) & 0xFF;	// 2 байт: 1 байт адреса памяти
    transmitted_data[3] = memory_addr & 0xFF;			// 3 байт: 2 байт адреса памяти

    SPI2_CS_LOW();																			// CS_LOW() начинает операцию
    if (SPI_Transmit(SPIx, transmitted_data, 4) != SPI_OK) return FM25Q08B_ERROR_WRITE;		// В случае ошибки передачи команды завершается работа функции со статусом ошибки
    if (SPI_Receive(SPIx, data, size) != SPI_OK) return FM25Q08B_ERROR_READ;				// В случае ошибки приема данных завершается работа функции со статусом ошибки
    SPI2_CS_HIGH();																			// CS_HIGH() заканчивает операцию

	// В случае успешного завершения статус FM25Q08B_OK
    return FM25Q08B_OK;
}


	/**
	! Статическая функция записи в память FM25Q08B по указанному адресу 1 страницы данных (не более 256 байт)
	- SPIx - модуль SPI (SPI1, SPI2, SPI3)
	- memory_addr - адрес в памяти FM25Q08B, начиная с которого нужно записать данные.
	- data - указатель на массив, данные из которого записываются в FM25Q08B.
	- size - количество записанных байт данных.
	return: статус выполнения записи в FM25Q08B.
	*/
static FM25Q08B_Status_t FM25Q08B_Write_Page(SPI_TypeDef* SPIx, uint32_t memory_addr, uint8_t* data, uint32_t size) //Запись одной страницы в память FM25Q08B по указанному адресу
{
    if (size > FLASH_PAGE_SIZE) return FM25Q08B_ERROR_WRITE_PAGE;	// Функция позволяет записать не более 1 страницы данных
    FM25Q08B_Write_Enable(SPIx);									// Перед любой операцией записи вызывается инструкция Write_Enable

	// Формирование отправляемых данных: встроенная команда + 3 байта адреса в памяти + не более 256 байт данных
    uint8_t transmitted_data[260];
    transmitted_data[0] = FLASH_CMD_PAGE_PROGRAM;		// 0 байт: встроенная команда
    transmitted_data[1] = (memory_addr >> 16) & 0xFF;	// 1 байт: 0 байт адреса памяти
    transmitted_data[2] = (memory_addr >> 8) & 0xFF;	// 2 байт: 1 байт адреса памяти
    transmitted_data[3] = memory_addr & 0xFF;			// 3 байт: 2 байт адреса памяти

    for (uint16_t i = 4; i < size + 4; i++)
    {
        transmitted_data[i] = data[i - 4];
    }

    SPI2_CS_LOW();																					// CS_LOW() начинает операцию
    if (SPI_Transmit(SPIx, transmitted_data, size + 4) != SPI_OK) return FM25Q08B_ERROR_WRITE;		// В случае ошибки передачи команды завершается работа функции со статусом ошибки
    SPI2_CS_HIGH();																					// CS_HIGH() заканчивает операцию
    FM25Q08B_Wait_End_Operation(SPIx);																// Ожидание завершения операции

	// В случае успешного завершения статус FM25Q08B_OK
    return FM25Q08B_OK;
}

	/**
	! Запись в память FM25Q08B по указанному адресу
	- SPIx - модуль SPI (SPI1, SPI2, SPI3)
	- memory_addr - адрес в памяти FM25Q08B, начиная с которого нужно записать данные.
	- data - указатель на массив, данные из которого записываются в FM25Q08B.
	- size - количество записанных байт данных.
	return: статус выполнения записи в FM25Q08B.
	*/
FM25Q08B_Status_t FM25Q08B_Write(SPI_TypeDef* SPIx, uint32_t memory_addr, uint8_t* data, uint32_t size)
{
    FM25Q08B_Status_t status;		// Возвращаемый статус выполнения функции

	// Если объем записи не превышает 1 страницу, то запись одной страницы с помощью статической функции FM25Q08B_Write_Page
    if (size <= FLASH_PAGE_SIZE)
    {
        status = FM25Q08B_Write_Page(SPIx, memory_addr, data, size);
        return status;
    }

	// Если объем записи превышает 1 страницу, то постраничная запись в цикле
    if (size > FLASH_PAGE_SIZE)
    {
        uint32_t current_address = memory_addr;     //Текущий адрес в памяти EEPROM
        uint32_t bytes_remaining = size;            //Количество оставшихся незаписанных байт
        uint16_t data_offset = 0;                   //Смещение в массиве данных

        while (bytes_remaining > 0)
        {
            // Определение сколько байт можно записать на текущей странице
            uint16_t bytes_on_page = 256 - (current_address % 256);
            uint16_t bytes_to_write = (bytes_remaining < bytes_on_page) ? bytes_remaining : bytes_on_page;

			// Отправка по SPI столько байт, сколько есть места до конца текущей страницы
            status = FM25Q08B_Write_Page(SPIx, current_address, data + data_offset, bytes_to_write);

			// Пересчет оставшихся байт данных, сдвига в массиве записываемых данных и сдвига адреса памяти FM25Q08B
            bytes_remaining -= bytes_to_write;
            data_offset += bytes_to_write;
            current_address += bytes_to_write;
        }
    }

	// Статус выполнения записи в FM25Q08B
    return status;
}