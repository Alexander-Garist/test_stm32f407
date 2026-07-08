// flash.c
#include <stdint.h>
#include "stm32f4xx.h"
#include "flash.h"


#define FLASH_SAMPLE_ADDR   0x080E0000 // Начало Сектора 11 (128 КБ)
#define PACKED_FRAME_SIZE   60000      // Размер нашего упакованного кадра 800х600

// 1. Функция разблокировки периферии Flash памяти STM32
void Flash_Unlock(void)
{
    if (FLASH->CR & FLASH_CR_LOCK)
    {
        FLASH->KEYR = 0x45670123; // Ключ 1 для разблокировки по даташиту
        FLASH->KEYR = 0xCDEF89AB; // Ключ 2 для разблокировки по даташиту
    }
}

// 2. Функция блокировки Flash памяти обратно
void Flash_Lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

// 3. Функция стирания Сектора 11 (Очистка памяти перед записью образца)
void Flash_Erase_Sample_Sector(void)
{
    Flash_Unlock();

    // Ждем окончания предыдущих операций (флаг BSY в регистре SR)
    while (FLASH->SR & FLASH_SR_BSY);

    // Настраиваем стирание Сектора 11 (биты SNB = 1011 = 11)
    FLASH->CR &= ~FLASH_CR_SNB;
    FLASH->CR |= (11 << 3) | FLASH_CR_SER;
    FLASH->CR |= FLASH_CR_STRT; // Запуск стирания

    while (FLASH->SR & FLASH_SR_BSY); // Ждем окончания (~1-2 секунды)

    FLASH->CR &= ~FLASH_CR_SER; // Сбрасываем флаг стирания секторов
    Flash_Lock();
}

// 4. Функция сохранения упакованного буфера из ОЗУ во Flash память
void Save_Sample_To_Flash(uint8_t *packed_buffer)
{
    // Сначала полностью очищаем сектор от старого образца
    Flash_Erase_Sample_Sector();

    Flash_Unlock();

    // Настраиваем программирование по 1 байту (Parallelism x8, PSIZE = 00)
    FLASH->CR &= ~FLASH_CR_PSIZE;
    FLASH->CR |= FLASH_CR_PG;

    uint32_t dest_addr = FLASH_SAMPLE_ADDR;

    for (uint32_t i = 0; i < PACKED_FRAME_SIZE; i++)
    {
        while (FLASH->SR & FLASH_SR_BSY);

        // Записываем байт напрямую по физическому адресу Flash памяти
        *(volatile uint8_t*)dest_addr = packed_buffer[i];
        dest_addr++;
    }

    while (FLASH->SR & FLASH_SR_BSY);
    FLASH->CR &= ~FLASH_CR_PG; // Выключаем режим программирования
    Flash_Lock();
}
