// flash.c
#include <stdint.h>
#include "stm32f4xx.h"
#include "flash.h"

// Определить номер сектора Flash по адресу памяти
static uint8_t get_sector_number(uint32_t memory_address)
{
    uint8_t sector_number = 0;
    if ((memory_address >= FLASH_SECTOR_0_START_ADDRESS) && ((memory_address < FLASH_SECTOR_1_START_ADDRESS)))  sector_number = 0;
    if ((memory_address >= FLASH_SECTOR_1_START_ADDRESS) && ((memory_address < FLASH_SECTOR_2_START_ADDRESS)))  sector_number = 1;
    if ((memory_address >= FLASH_SECTOR_2_START_ADDRESS) && ((memory_address < FLASH_SECTOR_3_START_ADDRESS)))  sector_number = 2;
    if ((memory_address >= FLASH_SECTOR_3_START_ADDRESS) && ((memory_address < FLASH_SECTOR_4_START_ADDRESS)))  sector_number = 3;
    if ((memory_address >= FLASH_SECTOR_4_START_ADDRESS) && ((memory_address < FLASH_SECTOR_5_START_ADDRESS)))  sector_number = 4;
    if ((memory_address >= FLASH_SECTOR_5_START_ADDRESS) && ((memory_address < FLASH_SECTOR_6_START_ADDRESS)))  sector_number = 5;
    if ((memory_address >= FLASH_SECTOR_6_START_ADDRESS) && ((memory_address < FLASH_SECTOR_7_START_ADDRESS)))  sector_number = 6;
    if ((memory_address >= FLASH_SECTOR_7_START_ADDRESS) && ((memory_address < FLASH_SECTOR_8_START_ADDRESS)))  sector_number = 7;
    if ((memory_address >= FLASH_SECTOR_8_START_ADDRESS) && ((memory_address < FLASH_SECTOR_9_START_ADDRESS)))  sector_number = 8;
    if ((memory_address >= FLASH_SECTOR_9_START_ADDRESS) && ((memory_address < FLASH_SECTOR_10_START_ADDRESS))) sector_number = 9;
    if ((memory_address >= FLASH_SECTOR_10_START_ADDRESS) && ((memory_address < FLASH_SECTOR_11_START_ADDRESS)))sector_number = 10;
    if ((memory_address >= FLASH_SECTOR_11_START_ADDRESS) && ((memory_address <= FLASH_END_ADDRESS)))           sector_number = 11;

    return sector_number;
}

// Определить размер сектора по номеру
static uint32_t get_sector_size(uint8_t sector_number)
{
    uint32_t sector_size = 0x0;

    switch (sector_number)
    {
        case 0: sector_size = FLASH_SECTOR_0_SIZE;  break;
        case 1: sector_size = FLASH_SECTOR_1_SIZE;  break;
        case 2: sector_size = FLASH_SECTOR_2_SIZE;  break;
        case 3: sector_size = FLASH_SECTOR_3_SIZE;  break;
        case 4: sector_size = FLASH_SECTOR_4_SIZE;  break;
        case 5: sector_size = FLASH_SECTOR_5_SIZE;  break;
        case 6: sector_size = FLASH_SECTOR_6_SIZE;  break;
        case 7: sector_size = FLASH_SECTOR_7_SIZE;  break;
        case 8: sector_size = FLASH_SECTOR_8_SIZE;  break;
        case 9: sector_size = FLASH_SECTOR_9_SIZE;  break;
        case 10: sector_size = FLASH_SECTOR_10_SIZE;  break;
        case 11: sector_size = FLASH_SECTOR_11_SIZE;  break;
    }
    return sector_size;
}

// Разблокировать FLASH
static void Flash_Unlock(void)
{
    if (FLASH->CR & FLASH_CR_LOCK)
    {
        FLASH->KEYR = 0x45670123; // Ключ 1 для разблокировки (по даташиту стр 84)
        FLASH->KEYR = 0xCDEF89AB; // Ключ 2 для разблокировки
    }
}

// Заблокировать FLASH
static void Flash_Lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;     // (по даташиту стр 85)
}

// Стирание 1 сектора по номеру
static void Flash_Erase_Sector(uint8_t sector_number)
{
    // разблокировать flash
    Flash_Unlock();

    // ожидание окончания предыдущих операций (флаг BSY в регистре SR)
    while (FLASH->SR & FLASH_SR_BSY);

    // стирание сектора (биты SNB == sector_number)           по даташиту стр 105
    FLASH->CR &= ~FLASH_CR_SNB;
    FLASH->CR |= (sector_number << 3) | FLASH_CR_SER;   // установка бита Sector Erase и Sector Number
    FLASH->CR |= FLASH_CR_STRT;                         // запуск операции стирания сектора

    while (FLASH->SR & FLASH_SR_BSY);                   // ожидание завершения стирания сектора
    FLASH->CR &= ~FLASH_CR_SER;                         // сброс флага стирания секторов

    // заблокировать flash после записи в нее
    Flash_Lock();
}






// Определить сколько и каких секторов нужно стереть + стереть требуемую память
void Erase_Memory(uint32_t memory_address, uint32_t size)
{
    uint32_t required_memory_size = size;                           // Требуемый объем памяти
    uint8_t sector_to_erase = get_sector_number(memory_address);    // Определить первый сектор, который будет стерт

    while (required_memory_size > 0)
    {
        // В случае если вся память поместится в 1 стираемый сектор
        if (required_memory_size <= get_sector_size(sector_to_erase))
        {
            Flash_Erase_Sector(sector_to_erase);
            required_memory_size = 0;
            break;
        }
        else
        {
            Flash_Erase_Sector(sector_to_erase);
            required_memory_size -= get_sector_size(sector_to_erase);
            sector_to_erase++;
        }
    }
}

// Запись данных из ОЗУ во Flash память по адресу
void Save_To_Flash(uint32_t memory_address, uint8_t *data, uint32_t size)
{
    // перед записью во flash ее нужно стереть
    Erase_Memory(memory_address, size);

    Flash_Unlock();

    // запись памяти по 1 байту (Parallelism x8, PSIZE = 00)
    FLASH->CR &= ~FLASH_CR_PSIZE;
    FLASH->CR |= FLASH_CR_PG;

    for (uint32_t index = 0; index < size; index++)
    {
        while (FLASH->SR & FLASH_SR_BSY);

        *(volatile uint8_t*)memory_address = data[index];
        memory_address++;
    }

    while (FLASH->SR & FLASH_SR_BSY);
    FLASH->CR &= ~FLASH_CR_PG; // выключить режим программирования памяти
    Flash_Lock();
}
