#include <stdio.h>
#include "CMSIS/stm32f4xx.h"
#include "systick.h"
#include "gpio.h"
#include "LED.h"

#include "programmer_target_Flash.h"





#define FIRMWARE_SIZE           0x1180
#define PAGE_SIZE               0x800
#define TARGET_MEMORY_ADDRESS   0x08000000

/********************* В файл линкера добавлено размещение 2 прошивок в flash память программатора ********************/
extern uint8_t target_firm_RAM;
uint8_t* firm_address_RAM = &target_firm_RAM;

extern uint8_t target_firm_FLASH;
uint8_t* firm_address_FLASH = &target_firm_FLASH;
/**********************************************************************************************************************/

// заполнить максимум 1 страницу 0x400 (1024 bytes)
static void fill_sended_buffer(uint8_t* sended_data, uint32_t* sended_size)
{
    static uint8_t page_max_count = FIRMWARE_SIZE / PAGE_SIZE;
    static uint8_t page_number = 0;                             // Номер отправляемой страницы
    uint32_t address_offset = page_number * PAGE_SIZE;  // Сдвиг по адресу прошивки каждый раз на 1 страницу

    if (page_number > page_max_count) { GPIO_set_HIGH(GPIOD, 13); return; }

    if (FIRMWARE_SIZE - PAGE_SIZE < address_offset)
    {
        // Записывается последняя неполная страница прошивки с текущим сдвигом
        for (uint32_t i = 0; i < FIRMWARE_SIZE - address_offset; i++)
        {
            sended_data[i] = *(firm_address_FLASH + address_offset + i);
        }

        *sended_size = FIRMWARE_SIZE - address_offset;
        page_number++;
    }
    else
    {
        // Записывается 1 страница прошивки с текущим сдвигом
        for (uint32_t i = 0; i < PAGE_SIZE; i++)
        {
            sended_data[i] = *(firm_address_FLASH + address_offset + i);
        }
        *sended_size = PAGE_SIZE;
        page_number++;
    }
}

///////** Проверить совпадают ли записанные данные с прочитанными (0 - совпадают, 1 - есть несовпадение) */
//////static uint8_t compare_sended_readed(uint8_t* sended_data, uint8_t* readed_data, uint32_t size)
//////{
//////    uint8_t result = 0;
//////    for (uint32_t i = 0; i < size; i++)
//////    {
//////        if (sended_data[i] != readed_data[i])
//////        {
//////            result = 1;
//////            break;
//////        }
//////    }
//////    return result;
//////}



int main()
{
    Clock_Config_168MHz_HSI();  // Разгон процессора до 168 МГц
    SysTick_Init();             // Инициализация системного таймера
    SoftSWD_Init();             // Инициализация программного SWD

    // Объявление буфера отправляемых данных и принимаемых данных
    uint8_t sended_data[PAGE_SIZE] = { 0 };
    uint8_t readed_data[PAGE_SIZE] = { 0 };

    uint32_t sending_bytes = 0;
    uint32_t current_offset = 0;

	while (1)
	{
        uint32_t idcode =  Connect_Target_GetIDCODE();  // Подключение к таргету и чтение его IDCODE
        Target_Halt();                                  // Остановка ядра атргета
        Erase_Flash_All();                              // Стирание всей Flash памяти

//        Erase_Flash_size(TARGET_MEMORY_ADDRESS, FIRMWARE_SIZE);
//        SoftSWD_ReadMemory(TARGET_MEMORY_ADDRESS, readed_data, PAGE_SIZE);
//        SoftSWD_Reset_Target();

        while (current_offset <= FIRMWARE_SIZE)
        {
            // Заполнение отправляемого буфера байтами прошивки таргета
            fill_sended_buffer(sended_data, &sending_bytes);

            // Запись по указанному адресу
            Program_Flash(TARGET_MEMORY_ADDRESS + current_offset, sended_data, sending_bytes);

            // Чтение для проверки успешности записи
            SoftSWD_ReadMemory(TARGET_MEMORY_ADDRESS + current_offset, readed_data, sending_bytes);

            current_offset += PAGE_SIZE;
        }

        delay_ms(100);
        Target_Run();

        SoftSWD_Reset_Target();
        delay_ms(3000);


        /** Светодиоды:
        *   синий: IDCODE прочитан правильно
        *   зеленый: данные записанные в память и прочитанные из этой же памяти совпадают
        *   красный: ошибка в чтении IDCODE
        *   красный + синий: IDCODE считан правильно, проверка записи и чтения в память не прошла
        */
        if (idcode != 0x2BA01477) GPIO_set_HIGH(GPIOD, 14);
        else GPIO_set_HIGH(GPIOD, 15);
	}
}