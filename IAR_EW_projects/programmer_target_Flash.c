/***********************************************************************************************************************
*   Программирование Flash памяти таргета через программный SWD
***********************************************************************************************************************/

#include "programmer_target_Flash.h"

/************************************************************************************************ СТАТИЧЕСКИЕ ФУНКЦИИ */

/** Разблокировка Flash-контроллера */
static void Unlock_Flash()
{
    SoftSWD_WriteRegister(AP, AP_TAR, FLASH_KEY);
    SoftSWD_WriteRegister(AP, AP_DRW, KEY1);
    SoftSWD_WriteRegister(AP, AP_DRW, KEY2);
}

/** Блокировка Flash-контроллера */
static void Lock_Flash()
{
    SoftSWD_WriteRegister(AP, AP_TAR, FLASH_CTRL);
    SoftSWD_WriteRegister(AP, AP_DRW, FLASH_CTRL_LOCK);
}

/** Ожидание завершения операции записи или стирания (аппаратного сброса флага FLASH_STS_BUSY) */
static void Wait_FLASH_STS_BUSY()
{
    uint32_t status;
    SoftSWD_WriteRegister(AP, AP_TAR, FLASH_STS);
    do
    {
        SoftSWD_ReadRegister(AP, AP_DRW);
        status = SoftSWD_ReadRegister(DP, DP_RDBUFF);
    } while (status & 0x01);
}

/** Упаковка 4 байт в одно 32-битное слово */
static uint32_t pack_words(uint8_t* buffer)
{
    uint32_t data = 0;
    data |= (uint32_t)buffer[0];
    data |= (uint32_t)buffer[1] << 8;
    data |= (uint32_t)buffer[2] << 16;
    data |= (uint32_t)buffer[3] << 24;
    return data;
}


/************************************************************************************************* ГЛОБАЛЬНЫЕ ФУНКЦИИ */

/** Стирает только необходимое для записи количество страниц */
void Erase_Flash_size(uint32_t start_address, uint32_t size)
{
    // 1. Включение питания и выбор порта MEM-AP (запись в CTRL/STAT и выбор AP 0, Bank 0)
    SoftSWD_set_MEM_AP();

    // 2. Настройка регистра CSW
    SoftSWD_WriteRegister(AP, AP_CSW, MEM_AP_DEFAULT);

    // 3. Разблокировка Flash-контроллера (Unlock)
    Unlock_Flash();

    // 4. Стирание необходимого количества страниц
    uint32_t num_pages = (size + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE;
    for (uint32_t p = 0; p < num_pages; p++)
    {
        uint32_t page_addr = start_address + (p * FLASH_PAGE_SIZE);

//        // Режим стирания страницы (PER)
//        SoftSWD_WriteRegister(AP, AP_TAR, FLASH_CTRL);
//        SoftSWD_WriteRegister(AP, AP_DRW, FLASH_CTRL_PER);

        // Адрес страницы
        SoftSWD_WriteRegister(AP, AP_TAR, FLASH_ADD);
        SoftSWD_WriteRegister(AP, AP_DRW, page_addr);

        // Запуск стирания (PER + START)
        SoftSWD_WriteRegister(AP, AP_TAR, FLASH_CTRL);
        SoftSWD_WriteRegister(AP, AP_DRW, FLASH_CTRL_PER | FLASH_CTRL_START);

        // Ожидание аппаратного сброса флага FLASH_STS_BUSY
        Wait_FLASH_STS_BUSY();
    }

    // 5. Блокировка Flash
    Lock_Flash();
}

/** Стирает всю Flash */
void Erase_Flash_All()
{
    // 1. Включение питания и выбор порта MEM-AP (запись в CTRL/STAT и выбор AP 0, Bank 0)
    SoftSWD_set_MEM_AP();

    // 2. Настройка регистра CSW
    SoftSWD_WriteRegister(AP, AP_CSW, MEM_AP_DEFAULT);

    // 3. Разблокировка Flash
    Unlock_Flash();

    // 4. Запуск полного стирания (MER + START)
    SoftSWD_WriteRegister(AP, AP_TAR, FLASH_CTRL);
    SoftSWD_WriteRegister(AP, AP_DRW, FLASH_CTRL_MER | FLASH_CTRL_START);

    // 5. Ожидание аппаратного сброса флага FLASH_STS_BUSY
    Wait_FLASH_STS_BUSY();

    // 6. Блокировка Flash
    Lock_Flash();
}

/** Запись в flash таргета */
void Program_Flash(uint32_t start_address, uint8_t* program_data, uint32_t program_size)
{
    // 1. Включение питания и выбор порта MEM-AP (запись в CTRL/STAT и выбор AP 0, Bank 0)
    SoftSWD_set_MEM_AP();

    // 2. Настройка регистра CSW
    SoftSWD_WriteRegister(AP, AP_CSW, MEM_AP_DEFAULT);

    // 3. Разблокировка Flash-контроллера
    Unlock_Flash();

    // 4 Режим программирования (PG)
    SoftSWD_WriteRegister(AP, AP_TAR, FLASH_CTRL);
    SoftSWD_WriteRegister(AP, AP_DRW, FLASH_CTRL_PG);

    // 5. Запись данных
    uint32_t operations = (program_size + 3) / 4;
    for (uint32_t i = 0; i < operations; i++)
    {
        uint32_t data_to_write = pack_words(program_data + (i * 4));
        uint32_t current_addr = start_address + (i * 4);

        // Запись 32-битного слова данных
        SoftSWD_WriteRegister(AP, AP_TAR, current_addr);
        SoftSWD_WriteRegister(AP, AP_DRW, data_to_write);

        // Ожидание аппаратного сброса флага FLASH_STS_BUSY
        Wait_FLASH_STS_BUSY();
    }

    // 6. Блокировка Flash
    Lock_Flash();
}
