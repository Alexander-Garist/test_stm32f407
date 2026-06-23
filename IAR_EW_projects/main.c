#include <stdio.h>
#include "stm32f4xx.h"
#include "systick.h"
#include "gpio.h"
#include "i2c.h"
#include "image_processing.h"

/**********************************************************************************************************************/
#define CAM_WIDTH        160
#define CAM_HEIGHT       120
#define CAM_FRAME_BYTES  (CAM_WIDTH * CAM_HEIGHT)

uint8_t camera_buffer[CAM_FRAME_BYTES];
int lines_processed = 0;

typedef struct
{
    uint8_t reg;
    uint8_t val;
}
ov2640_reg_t;

const ov2640_reg_t ov2640_config[] =
{
    /*********************************** СБРОС И СТАБИЛИЗАЦИЯ *********************************************************/
    {0xff, 0x01},   // переход на Table 1
    {0x12, 0x80},   // программный сброс модуля камеры
    {0x00, 0xff},   // пауза 100 мс для стабилизации настроек

    /***************************** НАСТРОЙКА МАТРИЦЫ И СИНХРОНИЗАЦИИ (Bank 1) *****************************************/
    {0xff, 0x01},   // переход на Table 1
/* оставляю значение по умолчанию */    //{0x11, 0x00},   // регистр CLKRC: делитель частоты встроенного кварца камеры
    {0x15, 0x22},   // регистр COM10: выбрана отрицательная полярность VSYNC, т.е. VSYNC имеет высокий уровень во время кадра, а при начале и завершении кадра становится низким
    {0x04, 0x08},   // регистр REG04: синхронизация HREF и DCLK

    /***************************** НАСТРОЙКА ЦИФРОВОГО ПОТОКА (Bank 0) ************************************************/
    {0xff, 0x00},   // переход на Table 0
    {0xe0, 0x04},   // регистр RESET: сброс модуля DVP
/* оставляю значение по умолчанию */    //{0xda, 0x00},   // регистр IMAGE_MODE: выбран формат YUV422, порядок битов "старший байт вперед"
/* оставляю значение по умолчанию */    //{0xc2, 0x0c},   // регистр CTRL0: включение модуля YUV422
    {0xd3, 0x8C},               // регистр R_DVP_SP: делитель PCLK
    {0x2c, 0xff}, {0x2e, 0xdf}, // настройка PLL (в даташите помечены как RESERVED)

    /********************************** НАСТРОЙКА ГЕОМЕТРИИ  **********************************************************/
    {0xc0, 0x14},   // HSIZE: Размер строки QQVGA
    {0xc1, 0x0f},   // VSIZE: Количество строк QQVGA)
    {0x86, 0x3d},   // Включение модуля масштабирования
/* оставляю значение по умолчанию */    //{0x50, 0x00},   // YUV формат входа
    {0x51, 0xa0},   // H_SIZE[7:0] = 160 (0xA0)
    {0x52, 0x78},   // V_SIZE[7:0] = 120 (0x78)
/* оставляю значение по умолчанию */    //{0x57, 0x00},
    {0xe1, 0x67},   // RESERVED
    {0xe5, 0x1f},   // RESERVED
    {0x5a, 0x14},   // Выходная ширина DSP / 10 = 16 (0x10)
    {0x5b, 0x0f},   // Выходная высота DSP / 10 = 12 (0x0C)
/* оставляю значение по умолчанию */    //{0x5c, 0x00},
    {0xd7, 0x03},   // RESERVED

    /**************************** НАСТРОЙКА МАТРИЦЫ СЕНСОРА (Bank 1) **************************************************/
    {0xff, 0x01},   // переход на Table 1
    {0x13, 0xE7},   // COM8: подавления мерцания
/* оставляю значение по умолчанию */    //{0x14, 0x38},
/* оставляю значение по умолчанию */    //{0x17, 0x11},   // регистр HREFST
/* оставляю значение по умолчанию */    //{0x18, 0x43},   // регистр HREFEND
/* оставляю значение по умолчанию */    //{0x19, 0x00},   // регистр VSTRT
/* оставляю значение по умолчанию */    //{0x1a, 0x97},   // регистр VEND
/* оставляю значение по умолчанию */    //{0x32, 0x09},   // Контроль HREF
    {0x03, 0x06},

    /** запуск конвейера */
    {0xff, 0x00}, // переход на Table 0
    {0xe0, 0x04},
    {0xe0, 0x00},
    {0xff, 0x01},   // переход на Table 1
    {0x12, 0x44},   // регистр COM7: выбор разрешения SVGA
    {0x00, 0x00}  // Конец таблицы
};

#define VSYNC_PORT      GPIOB
#define VSYNC_PIN       (1 << 5)
#define VSYNC_IS_HIGH   (VSYNC_PORT->IDR & VSYNC_PIN)
#define HREF_PORT       GPIOC
#define HREF_PIN        (1 << 9)
#define HREF_IS_HIGH    (HREF_PORT->IDR & HREF_PIN)
#define DCLK_PORT       GPIOC
#define DCLK_PIN        (1 << 10)
#define DCLK_IS_HIGH    (DCLK_PORT->IDR & DCLK_PIN)
#define DATA_PORT  GPIOE

int ov2640_capture_snapshot(uint8_t *buffer, int width, int height)
{
    GPIO_set_HIGH(GPIOD, 15);
    int lines_processed = 0;
    uint8_t *p_buf = buffer;

    // 1. Синхронизация по кадру
    while (VSYNC_PORT->IDR & VSYNC_PIN);    // Ждем окончания текущего кадра
    while (!(VSYNC_PORT->IDR & VSYNC_PIN)); // Ждем начала нового кадра

    // 2. Читаем строго первые 120 строк, которые нам нужны
    for (int y = 0; y < height; y++)
    {
        // Ожидание старта физической строки (HREF -> 1)
        while (!(HREF_PORT->IDR & HREF_PIN));

        // Вычитываем первые 160 пикселей (из 320 доступных).
        // 1 пиксель = 1 байт яркости Y (сохраняем) + 1 байт цветности U/V (пропускаем)
        for (int x = 0; x < width; x++)
        {
            // --- ЧТЕНИЕ БАЙТА ЯРКОСТИ (Y) ---
            while (!(DCLK_PORT->IDR & DCLK_PIN));
            *p_buf++ = (uint8_t)((GPIOE->IDR >> 8) & 0xFF);
            while (DCLK_PORT->IDR & DCLK_PIN);

            // --- ПРОПУСК БАЙТА ЦВЕТНОСТИ (U/V) ---
            while (!(DCLK_PORT->IDR & DCLK_PIN));
            while (DCLK_PORT->IDR & DCLK_PIN);
        }
        lines_processed++;

        // КРИТИЧЕСКИЙ ШАГ: Сливаем оставшиеся 480 импульсов DCLK этой строки!
        // Процессор просто ждет, пока синий сигнал HREF упадет в 0,
        // игнорируя весь остаток строки и Dummy-пиксели камеры.
        while (HREF_PORT->IDR & HREF_PIN);
    }

    // После того как мы собрали 120 строк, на камере останется еще ~180 строк кадра.
    // Мы их просто игнорируем и выходим из функции — светодиод гаснет.
    GPIO_set_LOW(GPIOD, 15);
    return lines_processed;
}





// Во время ожидания конца кадра или чего-то подобного (цикл while) обязательно обновлять счетчик мс!

uint32_t frame_start_us = 0;        // Момент начала кадра в мкс (VSYNC стал высоким)
uint32_t frame_end_us = 0;          // Момент конца кадра в мкс (VSYNC стал низким)
uint32_t frame_duration_us = 0;     // Длительность кадра в мкс

uint32_t raw_start_us = 0;          // Момент начала строки в мкс (HREF стал высоким)
uint32_t raw_end_us = 0;            // Момент конца строки в мкс (HREF стал низким)
uint32_t raw_duration_us = 0;       // Длительность строки в мкс

uint32_t raws = 0;      // Количество строк пикселей в кадре
uint32_t columns = 0;   // Количество столбцов пикселей в кадре


// Посчитать пиксели в 1 кадре
void count_pixels_in_frame()
{
    GPIO_set_HIGH(GPIOD, 12);   // Во время подсчета строк/пикселей в кадре загорается зеленый светодиод

    /** Дождаться конца текущего начатого кадра, затем подождать паузу между кадрами */
    while (VSYNC_IS_HIGH) {SysTick_Update_us();}    // Ожидание пока уровень высокий (начатый кадр должен закончиться)
    while (!VSYNC_IS_HIGH) {SysTick_Update_us();}   // Ожидание пока уровень низкий (пауза между кадрами должна закончиться)

    /** Кадр начался, VSYNC сейчас высокий, HREF еще низкий */
    frame_start_us = get_current_us();      // Начало кадра в мкс
    raws = 0;                               // Обнуление счетчика строк в кадре

    /** Ожидание окончания кадра */
    while (VSYNC_IS_HIGH)
    {
        while (!HREF_IS_HIGH)       // Ожидание начала строки (HREF должен стать высоким)
        {
            if (!VSYNC_IS_HIGH) break;
        }
        if (!VSYNC_IS_HIGH) break;

        /** Строка началась, HREF высокий */
        SysTick_Update_us();
        raw_start_us = get_current_us();    // Начало строки в мкс
        while (HREF_IS_HIGH);               // Ожидание окончания строки (HREF должен стать низким)

        SysTick_Update_us();
        raw_end_us = get_current_us();                  // Конец строки в мкс
        raw_duration_us = raw_end_us - raw_start_us;    // Длительность строки в мкс

        raws++;
    }

    SysTick_Update_us();
    frame_end_us = get_current_us();                                // Конец кадра в мкс
    frame_duration_us = frame_end_us - frame_start_us;              // Длительность кадра в мкс

    GPIO_set_LOW(GPIOD, 12);
}


int main(void)
{
    Clock_Config_168MHz_HSI();
    SysTick_Init();

    // Входы синхронизации
    GPIO_Camera_Input_Enable(GPIOD, 11);  // VSYNC
    GPIO_Camera_Input_Enable(GPIOC, 9);  // HREF
    GPIO_Camera_Input_Enable(GPIOC, 10);  // DCLK

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

    // Физический сброс питания и логики камеры
    GPIO_set_LOW(GPIOB, 0);  // PWDN = 0
    GPIO_set_LOW(GPIOB, 1);  // RESET = 0
    delay_ms(100);
    GPIO_set_HIGH(GPIOB, 1); // RESET = 1
    delay_ms(300);

    uint8_t pid_val = 0;
    uint8_t ver_val = 0;
    I2C_Status_t I2C_status;

    // Переход на страницу сенсора (Bank 1) для чтения паспорта микросхемы
    I2C_status = I2C_Write_Reg(I2C2, 0x30, 0xFF, 0x01);
    delay_ms(5);

    if (I2C_status == I2C_OK) I2C_status = I2C_Read_Reg(I2C2, 0x30, 0x0A, &pid_val);
    if (I2C_status == I2C_OK) I2C_status = I2C_Read_Reg(I2C2, 0x30, 0x0B, &ver_val);

    // Если нет физического контакта I2C или ID не совпадает — мигание красного светодиода
    if (I2C_status != I2C_OK || pid_val != 0x26 || ver_val != 0x42)
    {
        while(1)
        {
            GPIO_toggle_Pin(GPIOD, 14);
            delay_ms(50);
        }
    }

    // Запись регистров по I2C
    uint32_t reg_idx = 0;
    while (!(ov2640_config[reg_idx].reg == 0x00 && ov2640_config[reg_idx].val == 0x00))
    {
        uint8_t reg = ov2640_config[reg_idx].reg;
        uint8_t val = ov2640_config[reg_idx].val;

        // Обработка маркера искусственной паузы
        if (reg == 0x00 && val == 0xff)
        {
            delay_ms(100);
            reg_idx++;
            continue;
        }

        I2C_status = I2C_Write_Reg(I2C2, 0x30, reg, val);
        delay_ms(10);

        // Если запись таблицы оборвалась посередине — МЕДЛЕННОЕ мигание КРАСНОГО
        if (I2C_status != I2C_OK)
        {
            while(1)
            {
                GPIO_toggle_Pin(GPIOD, 14);
                delay_ms(250);
            }
        }
        reg_idx++;
    }

    // Синий светодиод горит стабильно => Конфигурация записана
    GPIO_set_HIGH(GPIOD, 15);
    delay_ms(1000);

    while(1)
    {
        count_pixels_in_frame();
        delay_ms(1000);
    }
}
