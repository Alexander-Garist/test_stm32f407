#include <stdio.h>
#include <string.h>
#include <math.h>
#include "stm32f4xx.h"
#include "systick.h"
#include "gpio.h"
#include "i2c.h"
#include "exti.h"
#include "usart.h"

#include "ov2640.h"
//#include "image_processing.h"
#include "flash.h"

uint8_t camera_packed_buffer[CAM_FRAME_BYTES / 8];  // 800 * 600 / 8 = 60000 байт

uint32_t lines_processed = 0;

uint8_t camera_restart = 0;             // Сброс и повторная инициализация камеры (если камера ослепла после смены освещения ее достаточно сбросить и она все видит)
uint32_t frame_counter = 0;

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
    GPIO_Camera_Input_Enable(GPIOD, 11);    // VSYNC
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
    ov2640_Init_Master_Mode(0x30);

    //ov2640_Init_Slave_Mode(0x30);
/**********************************************************************************************************************/

    while(1)
    {
        frame_counter++;

        ov2640_count_pixels_in_frame_Master_Mode();
        int rec = ov2640_capture_and_process_Master_Mode(camera_packed_buffer, CAM_WIDTH, CAM_HEIGHT, 1);

        if (Interrupt_EXTI0_Occured)
        {
            DEBUG_Save_File(camera_packed_buffer, CAM_FRAME_BYTES / 8, "ov2640_frame_binarized_packed.bin");
            Interrupt_EXTI0_Occured = 0;
        }

        delay_ms(1000);

        if (camera_restart || frame_counter == 5)
        {
            ov2640_Init_Master_Mode(0x30);
            //ov2640_Init_Slave_Mode(0x30);
            camera_restart = 0;
            frame_counter = 0;
        }
    }
}
