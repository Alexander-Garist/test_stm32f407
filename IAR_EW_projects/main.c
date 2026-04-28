#include <stdio.h>
#include "CMSIS/stm32f4xx.h"
#include "systick.h"
#include "gpio.h"
#include "soft_SWD.h"

typedef enum
{
	TASK_OK = 0,
	TASK_ERROR = -1
}Task_Status_t;

int main()
{
    Clock_Config_168MHz_HSI();  // Разгон процессора до 168 МГц
    SysTick_Init();             // Инициализация системного таймера

    SoftSWD_Init();             // Инициализация программного SWD
    SoftSWD_Sync_Target();      // Синхронизация хоста с таргетом

	while (1)
	{
//        uint8_t send_buf[32] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xAA, 0xBB, 0xCC, 0xDD, 0xAA, 0xBB, 0xCC, 0xDD, 0xAA, 0xBB, 0xCC, 0xDD,0xAA, 0xBB, 0xCC, 0xDD, 0xAA, 0xBB, 0xCC, 0xDD, 0xAA, 0xBB, 0xCC, 0xDD, 0xAA, 0xBB, 0xCC, 0xDD};
//        SoftSWD_WriteMemory(0x20000100, send_buf, 32);    // читатет после записи 00 ??
//


        uint32_t idcode = SoftSWD_Get_IDCODE();

        #define READED_BYTES    0x100
        // Чтение из flash памяти таргета, прочитанные байты выводятся по порядку
        uint8_t mem_data[READED_BYTES];
        GPIO_set_HIGH(GPIOD, 12);
        SoftSWD_ReadMemory(0x08000000, mem_data, READED_BYTES);
        GPIO_set_LOW(GPIOD, 12);

//        for (uint32_t i = 0; i < READED_BYTES; i++)
//        {
//            printf("%02X  ", mem_data[i]);
//            if ((i - 7) % 8 == 0) printf("  ");
//            if ((i - 15) % 16 == 0) printf("\n");
//        }

        // горит синий: ACK == 0x1 OK и четность прочитанная и посчитанная совпали
        // горит красный: ACK != 0x1 что-то неправильно
        if (idcode == 0x2BA01477)GPIO_set_HIGH(GPIOD, 15);
        else GPIO_set_HIGH(GPIOD, 14);
        delay_ticks(500000);
        GPIO_set_LOW(GPIOD, 15);
        GPIO_set_LOW(GPIOD, 14);
	}
}