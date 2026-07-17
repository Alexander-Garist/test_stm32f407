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
#include "image_processing.h"
#include "flash.h"

//uint8_t camera_packed_buffer[CAM_FRAME_BYTES / 8];  // 800 * 600 / 8 = 60000 байт


uint8_t camera_frame_fragment0[CAM_FRAME_BYTES / 5];
uint8_t camera_frame_fragment1[CAM_FRAME_BYTES / 5];
uint8_t camera_frame_fragment2[CAM_FRAME_BYTES / 5];
uint8_t camera_frame_fragment3[CAM_FRAME_BYTES / 5];
uint8_t camera_frame_fragment4[CAM_FRAME_BYTES / 5];



uint32_t lines_processed = 0;

uint8_t camera_restart = 0;         // Сброс и повторная инициализация камеры (если камера ослепла после смены освещения ее достаточно сбросить и она все видит)
uint8_t save_frame_to_FLASH = 0;    // Флаг, по которому образец запишется в Flash память

float comparison_result = 0.0f; // результат сравнения текущего кадра с образцом в памяти


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

/** Вариант для релиза
*       Сохранить в бинарный файл значения яркости всех пикселей */
void RELEASE_Save_File(uint8_t *buffer, uint32_t size, const char* filename)
{
    USART_Transmit(USART2, (char*)buffer, size);
}



int main(void)
{
    {   // Настройка STM32
    Clock_Config_168MHz_HSI();  // Тактовая частота процессора 168 МГц
    SysTick_Init();             // Включение системного таймера

    EXTI_Enable_Pin(EXTI_PortA, 0, EXTI_TRIGGER_FALLING);   // Включение обработки прерываний по нажатию кнопки
    }

    {   // Настройка выводов, подключенных к камере
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
    }

    {   // Инициализация I2C2
    I2C_Enable(I2C2);
    GPIO_Enable_I2C(GPIOB, 10); // SCL
    GPIO_Enable_I2C(GPIOB, 11); // SDA
    }
/********************************************** USART2 для передачи бинарных данных в рабочем режиме ******************/
    {   // Инициализация USART2 для передачи кадра на ПК
    USART_Init_Struct usart2_init;

    usart2_init.USARTx = USART2;
    usart2_init.GPIO_port_Tx = GPIOA;
    usart2_init.GPIO_pin_Tx = 2;
    usart2_init.GPIO_port_Rx = GPIOA;
    usart2_init.GPIO_pin_Rx = 3;
    usart2_init.baudrate = BRR_115200;

    USART_Enable(&usart2_init);
    }
/**********************************************************************************************************************/
    // Сброс и инициализация OV2640
    ov2640_Init(0x30);
/**********************************************************************************************************************/

/*    // БПФ 64 точки
    while (1)
    {
        double complex signal[64];      // Сигнал, который нужно разложить на спектр
        double complex spectrum[64];    // Спектр исходного сигнала

        // Генерируем тестовый сигнал: гармоника на частоте k=3
        // Формула: x(n) = sin(2*pi*3*n/32)
        for (int n = 0; n < 64; ++n)
        {
            double angle = 2.0 * M_PI * 3.0 * n / 64.0;
            signal[n] = 1+sin(angle) + 0.0 * I;
        }

        fft64(signal, spectrum);

        printf("--- Точный спектр 64-точечного БПФ ---\n");
        for (int k = 0; k < 64; ++k)
        {
            double re = creal(spectrum[k]);
            double im = cimag(spectrum[k]);

            // Убираем шумы окружения float чисел (все что меньше 1e-5 приравниваем к 0)
            if (fabs(re) < 1e-5) re = 0.0;
            if (fabs(im) < 1e-5) im = 0.0;

            printf("X[%2d] = %8.4f %s %8.4fj\n", k, re, (im >= 0 ? "+" : "-"), fabs(im));
        }
    }
*/

    while(1)
    {
        ov2640_count_pixels_in_frame();
        //int rec = ov2640_capture_and_process(camera_packed_buffer, CAM_WIDTH, CAM_HEIGHT, 1);   // захват кадра с обработкой на месте
        int result0 = ov2640_capture_fragment(camera_frame_fragment0, CAM_WIDTH, CAM_HEIGHT);
//        int result1 = ov2640_capture_fragment(camera_frame_fragment1, CAM_WIDTH, CAM_HEIGHT);
//        int result2 = ov2640_capture_fragment(camera_frame_fragment2, CAM_WIDTH, CAM_HEIGHT);
//        int result3 = ov2640_capture_fragment(camera_frame_fragment3, CAM_WIDTH, CAM_HEIGHT);


        // По нажатию кнопки либо произойдет отправка текущего кадра на ПК, либо запись образца кадра в Flash память
        if (Interrupt_EXTI0_Occured)
        {
            /** Запись образца снимка */
            if (save_frame_to_FLASH)
            {
                GPIO_set_HIGH(GPIOD, 12);

                //Save_To_Flash(FLASH_SECTOR_11_START_ADDRESS, camera_packed_buffer, CAM_FRAME_BYTES / 8);    // Записать образец в Flash
                //DEBUG_Save_File(camera_packed_buffer, CAM_FRAME_BYTES / 8, "ov2640_frame_example.bin");     // ДЛЯ ОТЛАДКИ отправить кадр образца на ПК
                save_frame_to_FLASH = 0;                                                                    // Сбросить флаг записи образца

                GPIO_set_LOW(GPIOD, 12);
            }
            /** Получение обычного текущего снимка */
            //DEBUG_Save_File(camera_packed_buffer, CAM_FRAME_BYTES / 8, "ov2640_frame_binarized_packed.bin");
            Interrupt_EXTI0_Occured = 0;
        }

        delay_ms(1000);

        if (camera_restart)
        {
            ov2640_Init(0x30);
            camera_restart = 0;
        }
    }
}


//while(1)
//    {
//        ov2640_count_pixels_in_frame();
//        int rec = ov2640_capture_and_process(camera_packed_buffer, CAM_WIDTH, CAM_HEIGHT, 1);   // захват кадра с обработкой на месте
//
//        // По нажатию кнопки либо произойдет отправка текущего кадра на ПК, либо запись образца кадра в Flash память
//        if (Interrupt_EXTI0_Occured)
//        {
//            /** Запись образца снимка */
//            if (save_frame_to_FLASH)
//            {
//                GPIO_set_HIGH(GPIOD, 12);
//
//                Save_To_Flash(FLASH_SECTOR_11_START_ADDRESS, camera_packed_buffer, CAM_FRAME_BYTES / 8);    // Записать образец в Flash
//                DEBUG_Save_File(camera_packed_buffer, CAM_FRAME_BYTES / 8, "ov2640_frame_example.bin");     // ДЛЯ ОТЛАДКИ отправить кадр образца на ПК
//                save_frame_to_FLASH = 0;                                                                    // Сбросить флаг записи образца
//
//                GPIO_set_LOW(GPIOD, 12);
//            }
//            /** Получение обычного текущего снимка */
//            DEBUG_Save_File(camera_packed_buffer, CAM_FRAME_BYTES / 8, "ov2640_frame_binarized_packed.bin");
//            Interrupt_EXTI0_Occured = 0;
//        }
//
//        delay_ms(1000);
//
//        if (camera_restart)
//        {
//            ov2640_Init(0x30);
//            camera_restart = 0;
//        }
//    }











