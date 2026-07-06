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
#include "image_processing.h"

uint8_t camera_buffer[CAM_FRAME_BYTES]; // Массив яркостей пикселей 1 кадра
uint8_t frame_example[CAM_FRAME_BYTES];

uint32_t lines_processed = 0;           // В одном кадре будет 600 строк, из них принимаются только 250

uint8_t camera_restart = 0;             // Сброс и повторная инициализация камеры (если камера ослепла после смены освещения ее достаточно сбросить и она все видит)
uint32_t frame_counter = 0;

uint8_t get_high_contrast_frame = 1;    // Получить снимок с повышением контрастности
uint8_t get_binarized_frame = 1;        // Получить снимок после бинаризации

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
    GPIO_set_HIGH(GPIOD, 13);
    USART_Transmit(USART2, (char*)filename, strlen(filename));  // Отправка названия бинарного файла по USART2

    USART_Transmit(USART2, (char*)'\r', 1);
    USART_Transmit(USART2, (char*)'\n', 1);

    USART_Transmit(USART2, (char*)buffer, size);                // Отправка кадра по USART2
    GPIO_set_LOW(GPIOD, 13);
}

/**********************************************************************************************************************/
// Поиск среднего значения массива
static float get_average_value(uint8_t* buffer, uint32_t size)
{
    float average_value = 0.0f; // Среднее значение массива данных
    float sum = 0.0f;           // Сумма элементов массива

    for (size_t index = 0; index < size; index++)
    {
        sum += *(buffer + index);
    }

    average_value = (float)sum / (float)size; // Целая часть
    return average_value;
}

// Поиск отклонения от среднего
static float deviation_from_average(uint8_t value, float average)
{
    return (float)value - average;
}

// сумма квадратов отклонений
static float sum_squares_deviations(float* array_deviations)
{
    float sum = 0.0f;

    // массив отклонений заменяется массивом квадратов отклонений
    for (uint32_t index = 0; index < CAM_FRAME_BYTES; index++)
    {
        sum += array_deviations[index] * array_deviations[index];
    }

    return sum;
}
/**********************************************************************************************************************/


float deviation_X[CAM_FRAME_BYTES], deviation_Y[CAM_FRAME_BYTES];             // массивы отклонений от средних значений
float correlation_coefficient;



int main(void)
{
    Clock_Config_168MHz_HSI();  // Тактовая частота процессора 168 МГц
    SysTick_Init();             // Включение системного таймера

    EXTI_Enable_Pin(EXTI_PortA, 0, EXTI_TRIGGER_FALLING);   // Включение обработки прерываний по нажатию кнопки

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

    // Инициализация I2C2
    I2C_Enable(I2C2);
    GPIO_Enable_I2C(GPIOB, 10); // SCL
    GPIO_Enable_I2C(GPIOB, 11); // SDA
/********************************************** USART2 для передачи бинарных данных в рабочем режиме ******************/
    // USART2 для передачи кадра на ПК
    USART_Init_Struct usart2_init;

    usart2_init.USARTx = USART2;
    usart2_init.GPIO_port_Tx = GPIOA;
    usart2_init.GPIO_pin_Tx = 2;
    usart2_init.GPIO_port_Rx = GPIOA;
    usart2_init.GPIO_pin_Rx = 3;
    usart2_init.baudrate = BRR_115200;

    USART_Enable(&usart2_init);
/**********************************************************************************************************************/
    // Сброс, проверка ID и запись регистров конфигурации в OV2640
    ov2640_Reset();
    ov2640_Read_ID(0x30);
    ov2640_Init(0x30);
/**********************************************************************************************************************/
/*  // тест свертки
//
//    uint8_t array_X[] = {1,  5, 3,  5,  5,  6, 3, 2,  6, 7,  11, 15, 13, 10};   // исходный массив X
//    uint8_t array_Y[] = {2, 11, 6, 10, 10, 12, 6, 4, 12, 1, 22, 30, 26, 15};   // исходный массив Y
//
//    float average_X = get_average_value(array_X, 14);   // среднее значение исходного массива X
//    float average_Y = get_average_value(array_Y, 14);   // среднее значение исходного массива Y
//
//    float deviation_X[14], deviation_Y[14];             // массивы отклонений от средних значений
//
//    for (uint32_t index = 0; index < 14; index++)
//    {
//        deviation_X[index] = deviation_from_average(array_X[index], average_X);
//    }
//
//    for (uint32_t index = 0; index < 14; index++)
//    {
//        deviation_Y[index] = deviation_from_average(array_Y[index], average_Y);
//    }
//
//    // Произведение отклонений
//    float numerator = 0.0f; // числитель коэффициента корреляции
//
//    for (uint32_t index = 0; index < 14; index++)
//    {
//        numerator += deviation_X[index] * deviation_Y[index];
//    }
//
//    // подсчет сумм квадратов отклонений
//    float sum_squares_X, sum_squares_Y;
//
//    sum_squares_X = sum_squares_deviations(deviation_X);
//    sum_squares_Y = sum_squares_deviations(deviation_Y);
//
//    // знаменатель формулы
//
//   float denominator = sqrt(sum_squares_X * sum_squares_Y);
//
//
//   float correlation_coefficient = numerator / denominator;
*/

    // создание образца изображения
    ov2640_capture_snapshot(frame_example, CAM_WIDTH, CAM_HEIGHT);
    ov2640_capture_snapshot(frame_example, CAM_WIDTH, CAM_HEIGHT);
    ov2640_capture_snapshot(frame_example, CAM_WIDTH, CAM_HEIGHT);
    ov2640_capture_snapshot(frame_example, CAM_WIDTH, CAM_HEIGHT);
    ov2640_capture_snapshot(frame_example, CAM_WIDTH, CAM_HEIGHT);                  // Исходный снимок образца
    ImageProcessing_increase_image_contrast(frame_example, CAM_FRAME_BYTES);        // Образец после повышения контрастности
    ImageProcessing_binarize_adaptive_local(frame_example, CAM_WIDTH, CAM_HEIGHT);  // Образец после бинаризации
    DEBUG_Save_File(frame_example, CAM_FRAME_BYTES, "EXAMPLE.bin");

    while(1)
    {
        frame_counter++;
        ov2640_count_pixels_in_frame();
        int received_lines = ov2640_capture_snapshot(camera_buffer, CAM_WIDTH, CAM_HEIGHT);     // Во время захвата кадра горит зеленый светодиод

        if (Interrupt_EXTI0_Occured)
        {
            GPIO_set_LOW(GPIOD, 12);
            GPIO_set_LOW(GPIOD, 14);

            DEBUG_Save_File(camera_buffer, CAM_FRAME_BYTES, "ov2640_frame.bin");  // Отправить на ПК сходный снимок камеры  (отправляется в любом случае по нажатию кнопки)
            //RELEASE_Save_File(camera_buffer, CAM_FRAME_BYTES, "ov2640_frame.bin");  // Отправка кадра по USART2

            // Отправить на ПК снимок с повышенной контрастностью
            if (get_high_contrast_frame)
            {
                ImageProcessing_increase_image_contrast(camera_buffer, CAM_FRAME_BYTES);            // Повышение контрастности изображения (изображение получается хорошее, четкое)

                DEBUG_Save_File(camera_buffer, CAM_FRAME_BYTES, "ov2640_frame_high_contrast.bin");  // Кадр после повышения контрастности
                //RELEASE_Save_File(camera_buffer, CAM_FRAME_BYTES, "ov2640_frame_high_contrast.bin");  // Кадр после повышения контрастности
            }

            // Отправить на ПК ЧБ снимок
            if (get_binarized_frame)
            {
                //ImageProcessing_binarize_image(camera_buffer, CAM_FRAME_BYTES);                     // Бинаризация изображения
                ImageProcessing_binarize_adaptive_local(camera_buffer, CAM_WIDTH, CAM_HEIGHT);

                DEBUG_Save_File(camera_buffer, CAM_FRAME_BYTES, "ov2640_frame_binarized.bin");      // Кадр после бинаризации изображения
                //RELEASE_Save_File(camera_buffer, CAM_FRAME_BYTES, "ov2640_frame_binarized.bin");      // Кадр после бинаризации изображения
            }

                float average_X = get_average_value(frame_example, CAM_FRAME_BYTES);   // среднее значение исходного массива X
                float average_Y = get_average_value(camera_buffer, CAM_FRAME_BYTES);   // среднее значение исходного массива Y

                for (uint32_t index = 0; index < CAM_FRAME_BYTES; index++)
                {
                    deviation_X[index] = deviation_from_average(frame_example[index], average_X);
                }

                for (uint32_t index = 0; index < CAM_FRAME_BYTES; index++)
                {
                    deviation_Y[index] = deviation_from_average(camera_buffer[index], average_Y);
                }

                // Произведение отклонений
                float numerator = 0.0f; // числитель коэффициента корреляции

                for (uint32_t index = 0; index < CAM_FRAME_BYTES; index++)
                {
                    numerator += deviation_X[index] * deviation_Y[index];
                }

                // подсчет сумм квадратов отклонений
                float sum_squares_X = sum_squares_deviations(deviation_X);
                float sum_squares_Y = sum_squares_deviations(deviation_Y);

                // знаменатель формулы
               float denominator = sqrt(sum_squares_X * sum_squares_Y);
               correlation_coefficient = numerator / denominator;

               if (correlation_coefficient >= 0.85f) GPIO_set_HIGH(GPIOD, 12);
               else GPIO_set_HIGH(GPIOD, 14);

            Interrupt_EXTI0_Occured = 0;
        }
        delay_ms(1000);

        if (camera_restart || frame_counter == 10)
        {
            ov2640_Reset();
            ov2640_Read_ID(0x30);
            ov2640_Init(0x30);

            camera_restart = 0;
            frame_counter = 0;
        }
    }
}
