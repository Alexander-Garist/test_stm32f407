#include <stdio.h>
#include "CMSIS/stm32f4xx.h"
#include "systick.h"
#include "gpio.h"
#include "soft_SWD.h"
#include "LED.h"

#define READED_BYTES    0x1000   // 2 КБ прочитать
#define SENDED_BYTES    0x1000   // 2 КБ отправить
#define MEMORY_ADDRESS  0x20004000

//// Вывести в терминал массив
//static void print_readed_data(uint8_t* readed_data, uint32_t size)
//{
//    for (uint32_t i = 0; i < size; i++)
//    {
//        printf("%02X  ", readed_data[i]);
//        if ((i - 7) % 8 == 0) printf("  ");
//        if ((i - 15) % 16 == 0) printf("\n");
//    }
//}

// Заполнить массив последовательными числами
static void fill_sended_buffer(uint8_t* sended_data, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++)
    {
        sended_data[i] += (uint8_t)i;
    }
}

// Проверить совпадают ли записанные данные с прочитанными (0 - совпадают, 1 - есть несовпадение)
static uint8_t compare_sended_readed(uint8_t* sended_data, uint8_t* readed_data, uint32_t size)
{
    uint8_t result = 0;
    for (uint32_t i = 0; i < size; i++)
    {
        if (sended_data[i] != readed_data[i])
        {
            result = 1;
            break;
        }
    }
    return result;
}

int main()
{
    Clock_Config_168MHz_HSI();  // Разгон процессора до 168 МГц
    SysTick_Init();             // Инициализация системного таймера

    // Объявление буфера отправляемых данных и принимаемых данных
    uint8_t sended_data[SENDED_BYTES] = { 0 };
    uint8_t readed_data[READED_BYTES] = { 0 };

    SoftSWD_Init();             // Инициализация программного SWD
    SoftSWD_Sync_Target();      // Синхронизация хоста с таргетом

	while (1)
	{
        //SoftSWD_Reset_Target();                         // Аппаратный сброс таргета для перезагрузки
        uint32_t idcode = SoftSWD_Get_IDCODE();         // Чтение IDCODE таргета

        fill_sended_buffer(sended_data, SENDED_BYTES);  // Заполнение отправляемого буфера последовательными числами

        // Сначала чтение из RAM по указанному адресу
        SoftSWD_ReadMemory(MEMORY_ADDRESS, readed_data, READED_BYTES);

        // Затем запись по указанному адресу
        SoftSWD_Write_RAM(MEMORY_ADDRESS, sended_data, SENDED_BYTES);

        // Повторное чтение для проверки успешности записи
        SoftSWD_ReadMemory(MEMORY_ADDRESS, readed_data, READED_BYTES);

        // Вывод в терминал прочитанных байт
        //print_readed_data(readed_data, READED_BYTES);



        /** Светодиоды:
        *   синий: IDCODE прочитан правильно
        *   зеленый: данные записанные в память и прочитанные из этой же памяти совпадают
        *   красный: ошибка в чтении IDCODE
        *   красный + синий: IDCODE считан правильно, проверка записи и чтения в память не прошла
        */
        if (idcode != 0x2BA01477) GPIO_set_HIGH(GPIOD, 14);
        else
        {
            GPIO_set_HIGH(GPIOD, 15);
            if (compare_sended_readed(sended_data, readed_data, READED_BYTES))
            { GPIO_set_HIGH(GPIOD, 14); }
            else GPIO_set_HIGH(GPIOD, 12);
        }
        delay_ms(1000);
        LED_turnOFF_4_LED();
	}
}