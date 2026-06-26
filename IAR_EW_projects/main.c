#include <stdio.h>
#include "stm32f4xx.h"
#include "systick.h"
#include "gpio.h"
#include "i2c.h"
#include "exti.h"

#include "ov2640.h"
/**********************************************************************************************************************/

#define CAM_WIDTH        400                        // в 1 строке 800 байт яркости и цветности, нужна только яркость для ЧБ
#define CAM_HEIGHT       250                        // количество строк не изменится
#define CAM_FRAME_BYTES  (CAM_WIDTH * CAM_HEIGHT)   // размер массива для яркости пикселей 1 кадра

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

/**********************************************************************************************************************/

uint32_t frame_start_us = 0;        // Момент начала кадра в мкс (VSYNC стал высоким)
uint32_t frame_end_us = 0;          // Момент конца кадра в мкс (VSYNC стал низким)
uint32_t frame_duration_us = 0;     // Длительность кадра в мкс

uint32_t raw_start_us = 0;          // Момент начала строки в мкс (HREF стал высоким)
uint32_t raw_end_us = 0;            // Момент конца строки в мкс (HREF стал низким)
uint32_t raw_duration_us = 0;       // Длительность строки в мкс

uint32_t raws = 0;      // Количество строк пикселей в кадре
uint32_t columns = 0;   // Количество тактов DCLK в строке

uint8_t camera_buffer[CAM_FRAME_BYTES]; // Массив яркостей пикселей 1 кадра
uint32_t lines_processed = 0;           // В одном кадре будет 600 строк, из них принимаются только 250

/**********************************************************************************************************************/
typedef struct
{
    uint8_t reg;
    uint8_t val;
}
ov2640_reg_t;

const ov2640_reg_t ov2640_config[] =
{
    /***************************** НАСТРОЙКА МАТРИЦЫ И СИНХРОНИЗАЦИИ (Bank 1) *****************************************/
    {0xff, 0x01},   // переход на Table 1
    {0x11, 0x17},   // регистр CLKRC: делитель частоты встроенного кварца камеры
    {0x15, 0x22},   // регистр COM10: выбрана отрицательная полярность VSYNC, т.е. VSYNC имеет высокий уровень во время кадра, а при начале и завершении кадра становится низким
    {0x04, 0xC8},   // регистр REG04: синхронизация HREF и DCLK

    /***************************** НАСТРОЙКА ЦИФРОВОГО ПОТОКА (Bank 0) ************************************************/
    {0xff, 0x00},   // переход на Table 0
    {0xe0, 0x04},   // регистр RESET: сброс модуля DVP
    /* оставляю значение по умолчанию */    //{0xda, 0x00},   // регистр IMAGE_MODE: выбран формат YUV422, порядок битов "старший байт вперед"
    /* оставляю значение по умолчанию */    //{0xc2, 0x0c},   // регистр CTRL0: включение модуля YUV422
    {0xd3, 0x0F},               // регистр R_DVP_SP: делитель PCLK
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
    /* оставляю значение по умолчанию */    //{0x13, 0xE7},   // COM8: подавления мерцания
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
/**********************************************************************************************************************/
/******* Во время ожидания конца кадра или чего-то подобного (цикл while) обязательно обновлять счетчик мс! ***********/
/**********************************************************************************************************************/

/** Захват кадра камеры
* в кадре 600 строк по 800 байт в каждой строке
* нужно забрать байты яркости (только нечетные) из каждой строки
* окно по высоте: строки от 175 до 425, остальные игнорируются
*/
int ov2640_capture_snapshot(uint8_t *buffer, int width, int height)
{
    GPIO_set_HIGH(GPIOD, 12); // Начало захвата кадра

    int lines_processed = 0;
    uint8_t *p_buf = buffer;

    while (VSYNC_IS_HIGH)   { SysTick_Update_us(); }    // Если кадр уже начался - ожидание конца кадра
    while (!VSYNC_IS_HIGH)  { SysTick_Update_us(); }    // Ожидание начала нового кадра

    // Пропуск верхних 175 строк
    for (int s = 0; s < 175; s++)
    {
        while (!HREF_IS_HIGH);  // Ожидание начала строки
        while (HREF_IS_HIGH);   // Ожидание конца строки
    }

    // Запись 250 строк в буфер
    for (int y = 0; y < height; y++)
    {
        while (!HREF_IS_HIGH);  // Ожидание начала строки

        // Чтение 400 пикселей яркости (800 тактов DCLK)
        for (int x = 0; x < width; x++)
        {
            // Нечетный такт (байт яркости)
            while (!DCLK_IS_HIGH);
            *p_buf++ = (uint8_t)((DATA_PORT->IDR >> 8) & 0xFF);
            while (DCLK_IS_HIGH);

            // Четный такт (байт цветности)
            while (!DCLK_IS_HIGH);
            while (DCLK_IS_HIGH);
        }
        lines_processed++;

        while (HREF_IS_HIGH);   // Ожидание конца строки
    }

    GPIO_set_LOW(GPIOD, 12); // Кадр собран
    return lines_processed;
}

/** Сохранить в бинарный файл значения яркости всех пикселей */
void save_file(uint8_t *buffer, uint32_t size)
{
    FILE *f = fopen("snapshot.bin", "wb");

    if (f != NULL)
    {
        fwrite(buffer, 1, size, f);
        fclose(f);
    }
}

/** Определить длительность кадра, длительность строки, количество строк и количество байт в строке */
void count_pixels_in_frame()
{
    GPIO_set_HIGH(GPIOD, 12);   // Включаем зеленый светодиод

    /** 1. Синхронизация по кадру */
    while (VSYNC_IS_HIGH) {SysTick_Update_us();}
    while (!VSYNC_IS_HIGH) {SysTick_Update_us();}

    /** Кадр начался */
    SysTick_Update_us();
    frame_start_us = get_current_us();
    raws = 0;

    /** 2. Ожидание окончания кадра */
    while (VSYNC_IS_HIGH)
    {
        // Ожидание старта строки
        while (!HREF_IS_HIGH)
        {
            if (!VSYNC_IS_HIGH) break;
        }
        if (!VSYNC_IS_HIGH) break;

        /** Строка началась, HREF высокий */
        SysTick_Update_us();
        raw_start_us = get_current_us();

        uint32_t dclk_counter = 0;

        while (HREF_IS_HIGH)
        {
            while (!DCLK_IS_HIGH && HREF_IS_HIGH);
            if (!HREF_IS_HIGH) break;

            while (DCLK_IS_HIGH && HREF_IS_HIGH);
            if (!HREF_IS_HIGH) break;

            dclk_counter++;
        }

        // Строка физически завершилась. Сохраняем результат в глобальную переменную
        columns = dclk_counter;

        SysTick_Update_us();
        raw_end_us = get_current_us();
        raw_duration_us = raw_end_us - raw_start_us;

        raws++;
    }

    /** Кадр окончен */
    SysTick_Update_us();
    frame_end_us = get_current_us();
    frame_duration_us = frame_end_us - frame_start_us;

    GPIO_set_LOW(GPIOD, 12);
}

/** Получить ID камеры */
void ov2640_Read_ID(uint8_t device_address)
{
    // Инициализация I2C2
    I2C_Enable(I2C2);           // Включение модуля I2C2
    GPIO_Enable_I2C(GPIOB, 10); // SCL
    GPIO_Enable_I2C(GPIOB, 11); // SDA

    uint8_t pid_val = 0;
    uint8_t ver_val = 0;
    I2C_Status_t I2C_status;

    // Переход на страницу сенсора (Bank 1) для чтения ID и Version микросхемы
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0xFF, 0x01);
    delay_ms(5);

    if (I2C_status == I2C_OK) I2C_status = I2C_Read_Reg(I2C2, device_address, 0x0A, &pid_val);
    if (I2C_status == I2C_OK) I2C_status = I2C_Read_Reg(I2C2, device_address, 0x0B, &ver_val);

    // Если нет физического контакта I2C или ID не совпадает — мигание красного светодиода в бесконечном цикле
    if (I2C_status != I2C_OK || pid_val != 0x26 || ver_val != 0x42)
    {
        while(1)
        {
            GPIO_toggle_Pin(GPIOD, 14);
            delay_ms(50);
        }
    }
}

/** Сброс камеры перед работой */
void ov2640_Reset()
{
    // Физический сброс питания и логики камеры
    GPIO_set_LOW(GPIOB, 0);  // PWDN = 0
    GPIO_set_LOW(GPIOB, 1);  // RESET = 0
    delay_ms(10);
    GPIO_set_HIGH(GPIOB, 1); // RESET = 1
    delay_ms(20);
}

/** Инициализация камеры: запись регистров по I2C */
void ov2640_Init(uint8_t device_address)
{
    I2C_Status_t I2C_status;    // Статус операции приема/передачи по I2C

    I2C_status = I2C_Write_Reg(I2C2, device_address, 0xFF, 0x01); // Table 1
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x12, 0x80); // Программный сброс
    delay_ms(100);



    // Запись регистров по I2C
    uint32_t reg_idx = 0;
    while (!(ov2640_config[reg_idx].reg == 0x00 && ov2640_config[reg_idx].val == 0x00))
    {
        uint8_t reg = ov2640_config[reg_idx].reg;
        uint8_t val = ov2640_config[reg_idx].val;

        I2C_status = I2C_Write_Reg(I2C2, device_address, reg, val);
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

    // добавить настройку яркости и контрастности
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0xff, 0x00); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7c, 0x00); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0x04); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7c, 0x07); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0x20); delay_ms(5);  // Значение яркости
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0x28); delay_ms(5);  // Младший байт контрастности
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0xc2); delay_ms(5);  // Старший байт контрастности
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0x06); delay_ms(5);
}

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

    // Сброс, проверка ID и запись регистров конфигурации в OV2640
    ov2640_Reset();
    ov2640_Read_ID(0x30);
    ov2640_Init(0x30);

    // Синий светодиод горит стабильно => Конфигурация записана
    GPIO_set_HIGH(GPIOD, 15);
    delay_ms(1000);

    // Перед снимком камера должна сделать несколько холостых снимков для адаптации к свету
    for (int i = 0; i < 25; i++)
    {
        GPIO_set_HIGH(GPIOD, 15);
        ov2640_capture_snapshot(camera_buffer, 400, 250);
        GPIO_set_LOW(GPIOD, 15);
    }

    while(1)
    {
        count_pixels_in_frame();

        int final_lines = ov2640_capture_snapshot(camera_buffer, 400, 250);
        if (Interrupt_EXTI0_Occured)
        {
            save_file(camera_buffer, 100000);
            Interrupt_EXTI0_Occured = 0;
            GPIO_toggle_Pin(GPIOD, 14);
            delay_ms(250);
            GPIO_toggle_Pin(GPIOD, 14);
        }
        delay_ms(1000);
    }
}
