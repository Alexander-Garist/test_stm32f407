#include <stdio.h>
#include <string.h>

#include "BL24CM1A.h"

I2C_Status_t BL24CM1A_Write(I2C_TypeDef* I2Cx, uint8_t device_addr, uint32_t reg_addr, uint8_t* data, uint16_t size)       //Новая версия записи в EEPROM, использует I2C_Write(I2Cx, device_addr, data, size)
{
    I2C_Status_t status;
    uint8_t write_buffer[258];          //Буфер записи 256 + 2 байта адреса
    uint16_t bytes_remaining = size;    //Количество оставшихся незаписанных байт
    uint16_t data_offset = 0;           //Смещение в массиве данных
    uint32_t current_address = reg_addr;//Текущий адрес в памяти EEPROM

    // Вычисление адреса устройства
    uint8_t current_device_addr = device_addr;
    if(current_address > 0xFFFF) {      //Если нужно обратиться к адресу из второй половины памяти EEPROM
        current_device_addr |= 0x01;    // Бит B16 в device_addr должен стать 1
    }

    while(bytes_remaining > 0)
    {
        // 1. Сколько байт можно записать на текущей странице
        uint16_t bytes_on_page = 256 - (current_address % 256);
        uint16_t bytes_to_write = (bytes_remaining < bytes_on_page) ? bytes_remaining : bytes_on_page;
        // 2. Подготовка буфера записи (адрес + данные)
        write_buffer[0] = (current_address >> 8) & 0xFF; // Старший байт адреса
        write_buffer[1] = current_address & 0xFF;        // Младший байт адреса
        // Копирование данных
        memcpy(&write_buffer[2], &data[data_offset], bytes_to_write);

        // 3. Отправка по I2C
        status = I2C_Write(I2Cx, current_device_addr, write_buffer, bytes_to_write + 2);

        if(status != I2C_OK)
        {
            //printf("Write failed at address 0x%05lX\n", current_address);
            return status;
        }
        // 4. Обновление счетчиков памяти и индексов в массиве
        bytes_remaining -= bytes_to_write;
        data_offset += bytes_to_write;
        current_address += bytes_to_write;
        // 5. Нужно ли сменить B16 в device_addr
        if((current_address & 0xFFFF) == 0)
        {
            current_device_addr = device_addr | ((current_address >> 16) & 0x01);
        }
        // 6. Задержка только если есть еще данные
        if(bytes_remaining > 0) delay_ms(5); // Задержка для внутренней записи EEPROM
    }
    return I2C_OK;
}

I2C_Status_t BL24CM1A_Read(I2C_TypeDef* I2Cx, uint8_t device_addr, uint32_t reg_addr, uint8_t* data, uint16_t size)       //Новая версия чтения из EEPROM, использует I2C_Read(I2Cx, device_addr, data, size)
{
    I2C_Status_t status;
    uint8_t addr_buffer[2];                     //2 байта адреса для фазы записи
    if (data == NULL) return I2C_DATA_NULL;
    // 1. Подготовка адреса памяти (2 байта)
    if(reg_addr > 0xFFFF) device_addr |= (1 << 0);
    addr_buffer[0] = (reg_addr >> 8) & 0xFF;
    addr_buffer[1] = reg_addr & 0xFF;

    uint16_t current_address = reg_addr;        //Текущий адрес в памяти EEPROM
    uint16_t bytes_remaining = size;            //Количество байт, которые осталось прочитать
    uint16_t data_offset = 0;                   //Сдвиг в массиве data

    while (bytes_remaining > 0) {
        // 2. Расчет сколько байт можно прочитать с текущей страницы
        uint16_t bytes_on_page = 256 - (current_address % 256); // До конца страницы
        uint16_t bytes_to_read = (bytes_remaining < bytes_on_page) ? bytes_remaining : bytes_on_page;       //Если осталось прочитать <= байт на странице, то читать все; если нет - то прочитать до конца страницы
        // 3. Обновление адреса для текущего блока
        addr_buffer[0] = (current_address >> 8) & 0xFF;
        addr_buffer[1] = current_address & 0xFF;
        // 4. Фаза записи: отправка адреса памяти
        status = I2C_Write(I2Cx, device_addr, addr_buffer, 2);
        if (status != I2C_OK)
        {
            //printf("Write address failed at 0x%04X\n", current_address);
            return status;
        }
        // 5. Короткая задержка между операциями
        delay_ms(1);
        // 6. Фаза чтения: чтение данных используя общую функцию
        status = I2C_Read(I2Cx, device_addr, data + data_offset, bytes_to_read);
        if (status != I2C_OK)
        {
            //printf("Read data failed at 0x%04X\n", current_address);
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





//===Старые версии===
/*I2C_Status BL24CM1A_Write(I2C_TypeDef* I2Cx, uint8_t device_addr, uint16_t reg_addr, uint8_t* data, uint16_t size)      // I2C_Write(I2Cx, device_addr, data, size) но 16-битный адрес памяти
{
    //reg_addr поместить в data
    uint8_t* data_with_addr = data;
    Add_ADDR_to_data(reg_addr, data, size, data_with_addr);   //теперь data_with_addr хранит 2 байта адреса в памяти EEPROM, куда надо вписать size байт данных

    uint16_t page_number = reg_addr / 256;                                      //Номер страницы памяти, на которой начнется чтение

    uint16_t Max_Page_Address = page_number * 0x100 + 0xff;                     //Последний адрес этой страницы

    uint16_t current_address_on_page = reg_addr;                                //Текущий адрес в памяти страницы EEPROM, обнуляется при переходе на следующую страницу
    uint16_t Rest_Memory_Current_Page = Max_Page_Address - reg_addr + 1;        //Сколько байт можно записать на текущей странице

    uint16_t curr_position_inData = 0;                                          //Текущий индекс в массиве данных, меняется при переходе на следующую страницу, чтобы на новой странице не писать старые данные повторно


    while(size > Rest_Memory_Current_Page)          //Пока объем полезных данных больше, чем поместится на страницу, запись будет идти постранично
    {
        if(I2C_Write(I2Cx, device_addr, data_with_addr + curr_position_inData, Rest_Memory_Current_Page + 2) != I2C_OK) return I2C_ERROR_WRITE;    //На текущей странице записать сколько влезет, максимум - 1 страница
        size -= Rest_Memory_Current_Page;                                       //Количество передаваемой информации уменьшается, чтобы не дублировать данные
        curr_position_inData += Rest_Memory_Current_Page;                       //Сдвиг индекса в массиве данных
        curr_position_inData += 2;

        //current_address_on_page переходит на следующую страницу на начальный адрес
        page_number++;                                  //Переход на новую страницу
        current_address_on_page = page_number * 0x100;  //Перемещение текущего адреса на начало новой страницы

        //current_address_on_page нужно снова впихнуть в data
         Add_ADDR_to_data(current_address_on_page, data + curr_position_inData, size, data_with_addr + curr_position_inData);

        //Остаток на новой странице равен длине всей страницы
        Rest_Memory_Current_Page = PAGE_LENGTH;
        delay_ms(10);
    }
    if(size <= Rest_Memory_Current_Page)
    {
        if(I2C_Write(I2Cx, device_addr, data_with_addr + curr_position_inData, size + 2) != I2C_OK) return I2C_ERROR_WRITE;     //Количество байт на 2 больше, чем полезные данные, т.к. первые 2 байта адрес в памяти
    }
    return I2C_OK;
}
*/
/*I2C_Status BL24CM1A_Read(I2C_TypeDef* I2Cx, uint8_t device_addr, uint16_t reg_addr, uint8_t* data, uint16_t size)       // I2C_Read(I2Cx, device_addr, data, size) но 16-битный адрес памяти
{
    I2C_Status status;
    uint8_t addr_buffer[2];                     //2 байта адреса для фазы записи
    if (data == NULL) return I2C_DATA_NULL;
    // 1. Подготовка адреса памяти (2 байта)
    addr_buffer[0] = (reg_addr >> 8) & 0xFF;    // Старший байт адреса
    addr_buffer[1] = reg_addr & 0xFF;           // Младший байт адреса
    //номер страницы не нужен, убрать в ф-и write
    //uint16_t page_number = reg_addr / 256;    // Номер страницы (0x100 = 256)
    uint16_t current_address = reg_addr;        //Текущий адрес в памяти EEPROM
    uint16_t bytes_remaining = size;            //Количество байт, которые осталось прочитать
    uint16_t data_offset = 0;                   //Сдвиг в массиве data

    while (bytes_remaining > 0) {
        // 2. Расчет сколько байт можно прочитать с текущей страницы
        uint16_t bytes_on_page = 256 - (current_address % 256); // До конца страницы
        uint16_t bytes_to_read = (bytes_remaining < bytes_on_page) ? bytes_remaining : bytes_on_page;       //Если осталось прочитать <= байт на странице, то читать все; если нет - то прочитать до конца страницы
        // 3. Обновление адреса для текущего блока
        addr_buffer[0] = (current_address >> 8) & 0xFF;
        addr_buffer[1] = current_address & 0xFF;
        // 4. Фаза записи: отправка адреса памяти
        status = I2C_Write(I2Cx, device_addr, addr_buffer, 2);
        if (status != I2C_OK) {
            printf("Write address failed at 0x%04X\n", current_address);
            return status;
        }
        // 5. Короткая задержка между операциями
        delay_ms(1);
        // 6. Фаза чтения: чтение данных используя общую функцию
        status = I2C_Read(I2Cx, device_addr, data + data_offset, bytes_to_read);
        if (status != I2C_OK) {
            printf("Read data failed at 0x%04X\n", current_address);
            return status;
        }
        // 7. Обновление счетчиков
        bytes_remaining -= bytes_to_read;
        data_offset += bytes_to_read;
        current_address += bytes_to_read;
        // 8. Задержка между страницами
        if (bytes_remaining > 0) {
            delay_ms(5);
        }
    }
    return I2C_OK;
}
*/

/*I2C_Status BL24CM1A_Write(I2C_TypeDef* port, uint8_t device_addr, uint16_t reg_addr, uint8_t* data, uint16_t size)        //Старая версия записи в EEPROM с использованием I2C_Write(..., reg_addr,...)
{
    uint16_t page_number = reg_addr / 256;                                      //Номер страницы памяти, на которой начнется чтение

    uint16_t Max_Page_Address = page_number * 0x100 + 0xff;                     //Последний адрес этой страницы

    uint16_t current_address_on_page = reg_addr;                                //Текущий адрес в памяти страницы EEPROM, обнуляется при переходе на следующую страницу
    uint16_t Rest_Memory_Current_Page = Max_Page_Address - reg_addr + 1;        //Сколько байт можно записать на текущей странице

    uint16_t curr_position_inData = 0;                                          //Текущий индекс в массиве данных, меняется при переходе на следующую страницу, чтобы на новой странице не писать старые данные повторно

    if(size <= Rest_Memory_Current_Page)
    {
        if(I2C_Write(port, device_addr, current_address_on_page, data + curr_position_inData, size) != I2C_OK) return I2C_ERROR_WRITE;
        return I2C_OK;
    }
    while(size > Rest_Memory_Current_Page)          //Пока объем данных больше, чем поместится на страницу, запись будет идти постранично
    {
        if(I2C_Write(port, device_addr, current_address_on_page, data + curr_position_inData, Rest_Memory_Current_Page) != I2C_OK) return I2C_ERROR_WRITE;    //На текущей странице записать сколько влезет, максимум - 1 страница
        size -= Rest_Memory_Current_Page;                                       //Количество передаваемой информации уменьшается, чтобы не дублировать данные
        curr_position_inData += Rest_Memory_Current_Page;                       //Сдвиг индекса в массиве данных

        //current_address_on_page переходит на следующую страницу на начальный адрес
        page_number++;                                  //Переход на новую страницу
        current_address_on_page = page_number * 0x100;  //Перемещение текущего адреса на начало новой страницы

        //Остаток на новой странице равен длине всей страницы
        Rest_Memory_Current_Page = PAGE_LENGTH;
        delay_ms(10);
        //while докрутится до тех пор пока size не влезет в одну страницу, тогда сработает if(size <= Rest_Memory_Current_Page) и допишется остаток данных
        if(size <= Rest_Memory_Current_Page)
        {
            if(I2C_Write(port, device_addr, current_address_on_page, data + curr_position_inData, size) != I2C_OK) return I2C_ERROR_WRITE;
        }
    }
    return I2C_OK;
}
*/
/*I2C_Status BL24CM1A_Read(I2C_TypeDef* port, uint8_t device_addr, uint16_t reg_addr, uint8_t* data, uint16_t size)         //Старая версия чтения из EEPROM с использованием I2C_Read(..., reg_addr,...)
{
    uint16_t page_number = reg_addr / 256;                                      //Номер страницы памяти, на которой начнется чтение
    uint16_t Max_Page_Address = page_number * 0x100 + 0xff;                     //Последний адрес текущей страницы

    uint16_t current_address_on_page = reg_addr;                                //Текущий адрес в памяти страницы EEPROM, обнуляется при переходе на следующую страницу
    uint16_t Rest_Memory_Current_Page = Max_Page_Address - reg_addr + 1;        //Количество байт, которые нужно считать с текущей страницы

    uint16_t curr_position_inData = 0;                                          //Текущий индекс в массиве данных, меняется при переходе на следующую страницу, чтобы данные с новой страницы не затерли данные с предыдущей страницы

    if(size <= Rest_Memory_Current_Page)
    {
        if(I2C_Read(port, device_addr, current_address_on_page, data + curr_position_inData, size) != I2C_OK) return I2C_ERROR_READ;
        return I2C_OK;
    }
    while(size > Rest_Memory_Current_Page)                                      //Если объем данных больше, чем есть на странице
    {
        if(I2C_Read(port, device_addr, current_address_on_page, data + curr_position_inData, Rest_Memory_Current_Page) != I2C_OK) return I2C_ERROR_READ;        //С текущей страницы считать стольь

        size -= Rest_Memory_Current_Page;                                                                   //количество принимаемой информации уменьшается, чтобы не дублировать данные
        curr_position_inData += Rest_Memory_Current_Page;

        //current_address_on_page нужно перенести на следующую страницу на начальный адрес
        page_number++;                                                          //Переход на новую страницу
        current_address_on_page = page_number * 0x100;                          //Перемещение текущего адреса на начало новой страницы
        //Остаток на новой странице равен длине всей страницы
        Rest_Memory_Current_Page = PAGE_LENGTH;
        delay_ms(10);
        //while докрутится до тех пор пока size не влезет в одну страницу, тогда сработает if(size <= Rest_Memory_Current_Page) и считается остаток данных
        if(size <= Rest_Memory_Current_Page)
        {
            if(I2C_Read(port, device_addr, current_address_on_page, data + curr_position_inData, size) != I2C_OK) return I2C_ERROR_READ;
        }
    }
    return I2C_OK;
}
*/
