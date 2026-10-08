#include <stdio.h>
#include <string.h>
#include <math.h>
#include "stm32f4xx.h"
#include "systick.h"
#include "gpio.h"
#include "i2c.h"
#include "exti.h"
#include "usart.h"
#include "button.h"

#include "ov2640.h"
#include "flash.h"

uint8_t camera_packed_buffer[CAM_FRAME_BYTES / 32];  // 400 * 300 / 8 = 15000 байт
uint32_t lines_processed = 0;

uint8_t camera_restart = 0;         // Сброс и повторная инициализация камеры (если камера ослепла после смены освещения ее достаточно сбросить и она все видит)
uint8_t save_frame_to_FLASH = 0;    // Флаг, по которому образец запишется в Flash память

/** Вариант для отладки
*       Сохранить в бинарный файл значения яркости всех пикселей */
void DEBUG_Save_File(uint8_t *buffer, uint32_t size, const char* filename)
{
    GPIO_set_HIGH(GPIOD, 13);
    FILE *f = fopen(filename, "wb");

    if (f != NULL)
    {
        fwrite(buffer, 1, size, f);
        fclose(f);
    }
    GPIO_set_LOW(GPIOD, 13);
}



int main(void)
{
    Clock_Config_168MHz_HSI();  // Тактовая частота процессора 168 МГц
    SysTick_Init();             // Включение системного таймера

    EXTI_Enable_Pin(EXTI_PortA, 0, EXTI_TRIGGER_FALLING);   // Включение обработки прерываний по нажатию кнопки


    // Настройка выводов, подключенных к камере
    // Входы синхронизации
    GPIO_Camera_Input_Enable(GPIOB, 5);    // VSYNC
    GPIO_Camera_Input_Enable(GPIOC, 9);     // HREF
    GPIO_Camera_Input_Enable(GPIOC, 10);    // DCLK

    // Входы данных
    GPIO_Camera_Input_Enable(GPIOE, 8);   // D0
    GPIO_Camera_Input_Enable(GPIOE, 9);   // D1
    GPIO_Camera_Input_Enable(GPIOE, 10);  // D2
    GPIO_Camera_Input_Enable(GPIOE, 11);  // D3
    GPIO_Camera_Input_Enable(GPIOE, 12);  // D4
    GPIO_Camera_Input_Enable(GPIOE, 13);  // D5
    GPIO_Camera_Input_Enable(GPIOE, 14);  // D6
    GPIO_Camera_Input_Enable(GPIOE, 15);  // D7

    // Инициализация I2C2
    I2C_Enable(I2C2);
    GPIO_Enable_I2C(GPIOB, 10); // SCL
    GPIO_Enable_I2C(GPIOB, 11); // SDA

    // Сброс и инициализация OV2640
    ov2640_Init(0x30);

    while(1)
    {
        GPIO_set_HIGH(GPIOD, 12);

        ov2640_count_pixels_in_frame();
        int rec = ov2640_capture_and_process(camera_packed_buffer, CAM_WIDTH, CAM_HEIGHT);   // захват кадра с обработкой на месте

        // По нажатию кнопки либо произойдет отправка текущего кадра на ПК, либо запись образца кадра в Flash память
        if (Interrupt_EXTI0_Occured)
        {
////            /** Запись образца снимка */
////            if (save_frame_to_FLASH)
////            {
////                GPIO_set_HIGH(GPIOD, 12);
////
////                //Save_To_Flash(FLASH_SECTOR_11_START_ADDRESS, camera_packed_buffer, CAM_FRAME_BYTES / 8);      // Записать образец в Flash
////                //DEBUG_Save_File(camera_packed_buffer, CAM_FRAME_BYTES / 8, "ov2640_frame_example.bin");       // ДЛЯ ОТЛАДКИ отправить кадр образца на ПК
////                save_frame_to_FLASH = 0;                                                                        // Сбросить флаг записи образца
////
////                GPIO_set_LOW(GPIOD, 12);
////            }

            /** Получение обычного текущего снимка */
            DEBUG_Save_File(camera_packed_buffer, CAM_FRAME_BYTES / 32, "ov2640_frame_binarized_packed.bin");
            Interrupt_EXTI0_Occured = 0;
        }

        if (camera_restart)
        {
            ov2640_Init(0x30);
            camera_restart = 0;
        }

        GPIO_set_LOW(GPIOD, 12);
        delay_ms(500);
    }
}