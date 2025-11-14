#include <string.h>
#include "FM25Q08B.h"

static void FM25Q08B_Wait_End_Operation(SPI_TypeDef* SPIx)                      //Ожидание завершения операции записи/стирания, т.е. ожидание пока WIP не станет 0
{
    uint8_t STATUS_REGISTER_1 = FM25Q08B_Read_Status_Register_1(SPIx);
    while(STATUS_REGISTER_1 & 0x1)
    {
        STATUS_REGISTER_1 = FM25Q08B_Read_Status_Register_1(SPIx);
    }
}

//===Программный сброс чипа=================================================================================================================================================
void FM25Q08B_Reset(SPI_TypeDef* SPIx)                                          //Программный сброс flash памяти
{
    uint8_t transmitted_data[2] = {FLASH_CMD_ENABLE_RESET, FLASH_CMD_RESET};
    SPI2_CS_LOW();
    SPI_Transmit(SPIx, transmitted_data, 2);
    SPI2_CS_HIGH();
    delay_ms(10);
}

//===Считать идентификатор чипа=============================================================================================================================================
void FM25Q08B_Read_Unique_ID(SPI_TypeDef* SPIx, uint8_t* Unique_ID)             //Считать Unique_ID чипа памяти
{
    uint8_t transmitted_data[5] = { FLASH_CMD_READ_UNIQUE_ID, 0x11, 0x22, 0x66, 0x88 };     //11.33 стр 51 команда + 4 байта мусора

    SPI2_CS_LOW();
    SPI_Transmit(SPIx, transmitted_data, 5);
    SPI_Receive(SPIx, Unique_ID, 8);
    SPI2_CS_HIGH();
}
uint32_t FM25Q08B_Read_JEDEC_ID(SPI_TypeDef* SPIx)                              //Считать JEDEC_ID чипа памяти
{
    uint8_t transmitted_data = FLASH_CMD_READ_JEDEC_ID;                         //11.34 стр 52
    uint8_t received_data[3];

    SPI2_CS_LOW();
    SPI_Transmit(SPIx, &transmitted_data, 1);
    SPI_Receive(SPIx, received_data, 3);

    SPI2_CS_HIGH();
    return ((received_data[0] << 16)|(received_data[1] << 8)|received_data[2]);  //manufacturer ID / memory type ID / capacity id
}
uint16_t FM25Q08B_Read_Manufacturer_ID(SPI_TypeDef* SPIx)                       //Считать Manufacturer_ID чипа памяти
{
    uint8_t transmitted_data[4] = { FLASH_CMD_READ_MANUF_DEVICE_ID, 0x00, 0x00, 0x00 }; //11.30 стр 48
    uint8_t received_data[2];

    SPI2_CS_LOW();
    if(SPI_Transmit(SPIx, transmitted_data, 4) != SPI_OK) return -1;
    if(SPI_Receive(SPIx, received_data, 2) != SPI_OK) return -1;
    SPI2_CS_HIGH();

    return ((received_data[0] << 8)|received_data[1]);  //manufacturer ID / device ID
}

//===Считать регистры статуса чипа==========================================================================================================================================
uint8_t FM25Q08B_Read_Status_Register_1(SPI_TypeDef* SPIx)                      //Считать STATUS REGISTER 1 (S7-S0)
{
    uint8_t transmitted_data = FLASH_CMD_READ_STATUS_REG_1;
    uint8_t STATUS_REGISTER_1 = 0;

    SPI2_CS_LOW();
    if(SPI_Transmit(SPIx, &transmitted_data, 1) != SPI_OK) return -1;
    if(SPI_Receive(SPIx, &STATUS_REGISTER_1, 1) != SPI_OK) return -1;
    SPI2_CS_HIGH();

    return STATUS_REGISTER_1;
}
uint8_t FM25Q08B_Read_Status_Register_2(SPI_TypeDef* SPIx)                      //Считать STATUS REGISTER 2 (S15-S8)
{
    uint8_t transmitted_data = FLASH_CMD_READ_STATUS_REG_2;
    uint8_t STATUS_REGISTER_2 = 0;

    SPI2_CS_LOW();
    if(SPI_Transmit(SPIx, &transmitted_data, 1) != SPI_OK) return -1;
    if(SPI_Receive(SPIx, &STATUS_REGISTER_2, 1) != SPI_OK) return -1;
    SPI2_CS_HIGH();

    return STATUS_REGISTER_2;
}

//===Разрешить/запретить запись в память чипа===============================================================================================================================
FM25Q08B_Status_t FM25Q08B_Write_Enable(SPI_TypeDef* SPIx)                      //Разрешить запись, нужно вызывать перед каждой операцией записи в память/очистки памяти
{
    uint8_t transmitted_data = FLASH_CMD_WRITE_ENABLE;

    SPI2_CS_LOW();
    if(SPI_Transmit(SPIx, &transmitted_data, 1) != SPI_OK) return FM25Q08B_ERROR_WRITE;
    SPI2_CS_HIGH();

    return FM25Q08B_OK;
}
FM25Q08B_Status_t FM25Q08B_Write_Disable(SPI_TypeDef* SPIx)                     //Запретить запись
{
    uint8_t transmitted_data = FLASH_CMD_WRITE_DISABLE;

    SPI2_CS_LOW();
    if(SPI_Transmit(SPIx, &transmitted_data, 1) != SPI_OK) return FM25Q08B_ERROR_WRITE;
    SPI2_CS_HIGH();

    return FM25Q08B_OK;
}

//===Очистить память чипа (сектор/блок/весь чип)============================================================================================================================
FM25Q08B_Status_t FM25Q08B_Sector_Erase_4KB(SPI_TypeDef* SPIx, uint32_t memory_addr)         //Очистить сектор памяти 4KB, memory_addr - начальный адрес сектора
{
    if(FM25Q08B_Write_Enable(SPIx) != FM25Q08B_OK) return FM25Q08B_ERROR_ERASE; //Перед любой операцией очистки памяти вызывается инструкция Write_Enable

    uint8_t transmitted_data[4];        //код команды и 3 байта начального адреса сектора
    transmitted_data[0] = FLASH_CMD_SECTOR_ERASE_4KB;
    transmitted_data[1] = (memory_addr >> 16) & 0xFF;
    transmitted_data[2] = (memory_addr >> 8) & 0xFF;
    transmitted_data[3] = memory_addr & 0xFF;

    SPI2_CS_LOW();
    if(SPI_Transmit(SPIx, transmitted_data, 4) != SPI_OK) return FM25Q08B_ERROR_ERASE;
    SPI2_CS_HIGH();
    FM25Q08B_Wait_End_Operation(SPIx);

    return FM25Q08B_OK;
}
FM25Q08B_Status_t FM25Q08B_Block_Erase_32KB(SPI_TypeDef* SPIx, uint32_t memory_addr)         //Очистить блок памяти 32KB, memory_addr - начальный адрес блока
{
    if(FM25Q08B_Write_Enable(SPIx) != FM25Q08B_OK) return FM25Q08B_ERROR_ERASE; //Перед любой операцией очистки памяти вызывается инструкция Write_Enable

    uint8_t transmitted_data[4];//команда и 3 байта адреса
    transmitted_data[0] = FLASH_CMD_BLOCK_ERASE_32KB;
    transmitted_data[1] = (memory_addr >> 16) & 0xFF;
    transmitted_data[2] = (memory_addr >> 8) & 0xFF;
    transmitted_data[3] = memory_addr & 0xFF;

    SPI2_CS_LOW();
    if(SPI_Transmit(SPIx, transmitted_data, 4) != SPI_OK) return FM25Q08B_ERROR_ERASE;
    SPI2_CS_HIGH();
    FM25Q08B_Wait_End_Operation(SPIx);

    return FM25Q08B_OK;
}
FM25Q08B_Status_t FM25Q08B_Block_Erase_64KB(SPI_TypeDef* SPIx, uint32_t memory_addr)         //Очистить блок памяти 64KB, memory_addr - начальный адрес блока
{
    if(FM25Q08B_Write_Enable(SPIx) != FM25Q08B_OK) return FM25Q08B_ERROR_ERASE; //Перед любой операцией очистки памяти вызывается инструкция Write_Enable

    uint8_t transmitted_data[4];//команда и 3 байта адреса
    transmitted_data[0] = FLASH_CMD_BLOCK_ERASE_64KB;
    transmitted_data[1] = (memory_addr >> 16) & 0xFF;
    transmitted_data[2] = (memory_addr >> 8) & 0xFF;
    transmitted_data[3] = memory_addr & 0xFF;

    SPI2_CS_LOW();
    if(SPI_Transmit(SPIx, transmitted_data, 4) != SPI_OK) return FM25Q08B_ERROR_ERASE;
    SPI2_CS_HIGH();
    FM25Q08B_Wait_End_Operation(SPIx);

    return FM25Q08B_OK;
}
FM25Q08B_Status_t FM25Q08B_Chip_Erase(SPI_TypeDef* SPIx)                                     //Очистить всю память чипа 1024KB
{
    if(FM25Q08B_Write_Enable(SPIx) != FM25Q08B_OK) return FM25Q08B_ERROR_ERASE; //Перед любой операцией очистки памяти вызывается инструкция Write_Enable

    uint8_t transmitted_data = FLASH_CMD_CHIP_ERASE;

    SPI2_CS_LOW();
    if(SPI_Transmit(SPIx, &transmitted_data, 1) != SPI_OK) return FM25Q08B_ERROR_ERASE;
    SPI2_CS_HIGH();
    FM25Q08B_Wait_End_Operation(SPIx);

    return FM25Q08B_OK;
}

//===Чтение/запись==========================================================================================================================================================
FM25Q08B_Status_t FM25Q08B_Read(SPI_TypeDef* SPIx, uint32_t memory_addr, uint8_t* data, uint32_t size)              //Чтение из памяти FM25Q08B по указанному адресу
{
    if (data == NULL) return FM25Q08B_ERROR_READ;
    uint8_t transmitted_data[4];
    transmitted_data[0] = FLASH_CMD_READ_DATA;
    transmitted_data[1] = (memory_addr >> 16) & 0xFF;
    transmitted_data[2] = (memory_addr >> 8) & 0xFF;
    transmitted_data[3] = memory_addr & 0xFF;

    SPI2_CS_LOW();
    if(SPI_Transmit(SPIx, transmitted_data, 4) != SPI_OK) return FM25Q08B_ERROR_WRITE;
    if(SPI_Receive(SPIx, data, size) != SPI_OK) return FM25Q08B_ERROR_READ;
    SPI2_CS_HIGH();

    return FM25Q08B_OK;
}
static FM25Q08B_Status_t FM25Q08B_Write_Page(SPI_TypeDef* SPIx, uint32_t memory_addr, uint8_t* data, uint32_t size) //Запись одной страницы в память FM25Q08B по указанному адресу
{
    if(size > FLASH_PAGE_SIZE) return FM25Q08B_ERROR_WRITE_PAGE;
    FM25Q08B_Write_Enable(SPIx);        //Перед любой операцией записи вызывается инструкция Write_Enable

    uint8_t transmitted_data[260];//команда и 3 байта адреса
    transmitted_data[0] = FLASH_CMD_PAGE_PROGRAM;
    transmitted_data[1] = (memory_addr >> 16) & 0xFF;
    transmitted_data[2] = (memory_addr >> 8) & 0xFF;
    transmitted_data[3] = memory_addr & 0xFF;

    for(uint16_t i = 4; i < size + 4; i++)
    {
        transmitted_data[i] = data[i - 4];
    }

    SPI2_CS_LOW();
    if(SPI_Transmit(SPIx, transmitted_data, size + 4) != SPI_OK) return FM25Q08B_ERROR_WRITE;
    SPI2_CS_HIGH();
    FM25Q08B_Wait_End_Operation(SPIx);

    return FM25Q08B_OK;
}
FM25Q08B_Status_t FM25Q08B_Write(SPI_TypeDef* SPIx, uint32_t memory_addr, uint8_t* data, uint32_t size)             //Запись в память FM25Q08B по указанному адресу
{
    FM25Q08B_Status_t status;
    if(size <= FLASH_PAGE_SIZE)
    {
        status = FM25Q08B_Write_Page(SPIx, memory_addr, data, size); //если объем записи не превышает 1 страницу, то обычная запись одной страницы
        return status;
    }
    if(size > FLASH_PAGE_SIZE)
    {
        uint32_t current_address = memory_addr;     //Текущий адрес в памяти EEPROM
        uint32_t bytes_remaining = size;            //Количество оставшихся незаписанных байт
        uint16_t data_offset = 0;                   //Смещение в массиве данных

        while(bytes_remaining > 0)
        {
            // Сколько байт можно записать на текущей странице
            uint16_t bytes_on_page = 256 - (current_address % 256);
            uint16_t bytes_to_write = (bytes_remaining < bytes_on_page) ? bytes_remaining : bytes_on_page;

            status = FM25Q08B_Write_Page(SPIx, current_address, data + data_offset, bytes_to_write);    //отправка по SPI столько байт, сколько есть места до конца страницы

            bytes_remaining -= bytes_to_write;
            data_offset += bytes_to_write;
            current_address += bytes_to_write;
        }
    }

    return status;
}