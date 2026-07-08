#include "ov2640.h"
#include "i2c.h"
#include "gpio.h"
#include "systick.h"

uint32_t frame_start_us = 0;        // Момент начала кадра в мкс (VSYNC стал высоким)
uint32_t frame_end_us = 0;          // Момент конца кадра в мкс (VSYNC стал низким)
uint32_t frame_duration_us = 0;     // Длительность кадра в мкс

uint32_t raw_start_us = 0;          // Момент начала строки в мкс (HREF стал высоким)
uint32_t raw_end_us = 0;            // Момент конца строки в мкс (HREF стал низким)
uint32_t raw_duration_us = 0;       // Длительность строки в мкс

uint32_t raws = 0;      // Количество строк пикселей в кадре
uint32_t columns = 0;   // Количество тактов DCLK в строке

/********************************* Таблицы регистров OV2640 ***********************************************************/
// SVGA
const ov2640_reg_t  ov2640_settings_to_svga[] =
{
    {0xFF, 0x01},
    {0x12, 0x40},

    //Set the sensor output window
    {0x03, 0x0A},
    {0x32, 0x09},
    {0x17, 0x11},
    {0x18, 0x43},
    {0x19, 0x00},
    {0x1A, 0x4b},

    // {CLKRC, 0x00},
    {0x37, 0xc0},
    {0x4F, 0xca},
    {0x50, 0xa8},
    {0x5a, 0x23},
    {0x6d, 0x00},
    {0x3d, 0x38},
    {0x39, 0x92},
    {0x35, 0xda},
    {0x22, 0x1a},
    {0x37, 0xc3},
    {0x23, 0x00},
    {0x34, 0xc0},
    {0x06, 0x88},
    {0x07, 0xc0},
    {0x0D, 0x87},
    {0x0e, 0x41},
    {0x42, 0x03},
    {0x4c, 0x00},

    {0xFF, 0x00},
    {0xE0, 0x04},

    //Set the sensor resolution (UXGA, SVGA, CIF)
    {0xC0, 0x64},
    {0xC1, 0x4B},
    {0x8C, 0x00},

    //Set the image window size >= output size
    {0x51, 0xC8},
    {0x52, 0x96},
    {0x53, 0x00},
    {0x54, 0x00},
    {0x55, 0x00},
    {0x57, 0x00},

    {0x86, 0x3D},
    {0x50, 0x80},
    // {R_DVP_SP, 0x08},
    {0x00, 0x00}
};

// Настройки камеры в режиме Master Mode
const ov2640_reg_t ov2640_config_Master_Mode[] =
{
    /***************************** НАСТРОЙКА ЦИФРОВОГО ПОТОКА (Bank 0) ************************************************/
    {0xff, 0x00},   // переход на Table 0
    {0x05, 0x01},

    {0xE0, 0x04},   // регистр RESET: сброс модуля DVP              ВЗЯЛ ИЗ НАСТРОЙКИ YUV422
    {0xDA, 0x00},
    {0xD7, 0x01},
    {0xE1, 0x67},
    {0xE0, 0x00},
    {0xd3, 0x0F},               // регистр R_DVP_SP: делитель PCLK
    {0x2c, 0xff}, {0x2e, 0xdf}, // настройка PLL (в даташите помечены как RESERVED)

    /********************************** НАСТРОЙКА ГЕОМЕТРИИ  **********************************************************/
    {0xC0, 0x64},   //                                                  ВЗЯЛ ИЗ SVGA
    {0xC1, 0x4B},

    {0xFF, 0x00},   //                                                  ВЗЯЛ НАСТРОЙКИ ОКНА
    {0x51, 0x20},   // max_x & 0xFF         max_x = 0x320
    {0x52, 0x58},   // max_y & 0xFF         max_y = 0x258
    {0x53, 0x00},   // offset_x & 0xFF      offset_x = 0x0
    {0x54, 0x00},   // offset_y & 0xFF      offset_y = 0x0
    {0x55, 0x08},
    {0x57, 0X80},
    {0x5A, 0x20},   // w = 0x320
    {0x5B, 0x58},   // h = 0x258
    {0x5C, 0x03},

    {0x86, 0x3d},   // Включение модуля масштабирования                 ПЕРЕНЕС СЮДА

    {0xe1, 0x67},   // RESERVED
    {0xe5, 0x1f},   // RESERVED
    // {0x5A, 0x14},   // Выходная ширина DSP / 10 = 16 (0x10)
    // {0x5B, 0x0f},   // Выходная высота DSP / 10 = 12 (0x0C)

    {0xd7, 0x03},   // RESERVED

    /**************************** НАСТРОЙКА МАТРИЦЫ СЕНСОРА (Bank 1) **************************************************/
    {0xff, 0x01},   // переход на Table 1
    {0x03, 0x06},
    {0x11, 0x17},   // регистр CLKRC: делитель частоты встроенного кварца камеры
    {0x15, 0x22},   // регистр COM10: выбрана отрицательная полярность VSYNC, т.е. VSYNC имеет высокий уровень во время кадра, а при начале и завершении кадра становится низким
    {0x04, 0xC8},   // регистр REG04: синхронизация HREF и DCLK

    /** запуск конвейера */
    {0xff, 0x01},   // переход на Table 1
    {0x12, 0x44},   // регистр COM7: выбор разрешения SVGA
    {0xE0, 0x04},
    {0xE0, 0x00},
    {0x05, 0x00},
    {0x00, 0x00}    // Конец таблицы
};

// Настройки камеры в режиме SLAVE MODE
const ov2640_reg_t ov2640_config_Slave_Mode[] =
{
    /***************************** НАСТРОЙКА ЦИФРОВОГО ПОТОКА (Bank 0) ************************************************/
    {0xff, 0x00},   // переход на Table 0
    {0x05, 0x01},

    {0xE0, 0x04},   // регистр RESET: сброс модуля DVP              ВЗЯЛ ИЗ НАСТРОЙКИ YUV422
    {0xDA, 0x00},
    {0xD7, 0x01},
    {0xE1, 0x67},
    {0xE0, 0x00},
    {0xd3, 0x0F},               // регистр R_DVP_SP: делитель PCLK
    {0x2c, 0xff}, {0x2e, 0xdf}, // настройка PLL (в даташите помечены как RESERVED)

    /********************************** НАСТРОЙКА ГЕОМЕТРИИ  **********************************************************/
    {0xC0, 0x64},   //                                                  ВЗЯЛ ИЗ SVGA
    {0xC1, 0x4B},

    {0xFF, 0x00},   //                                                  ВЗЯЛ НАСТРОЙКИ ОКНА
    {0x51, 0x20},   // max_x & 0xFF         max_x = 0x320
    {0x52, 0x58},   // max_y & 0xFF         max_y = 0x258
    {0x53, 0x00},   // offset_x & 0xFF      offset_x = 0x0
    {0x54, 0x00},   // offset_y & 0xFF      offset_y = 0x0
    {0x55, 0x08},
    {0x57, 0X80},
    {0x5A, 0x20},   // w = 0x320
    {0x5B, 0x58},   // h = 0x258
    {0x5C, 0x03},

    {0x86, 0x3d},   // Включение модуля масштабирования                 ПЕРЕНЕС СЮДА

    {0xe1, 0x67},   // RESERVED
    {0xe5, 0x1f},   // RESERVED
    {0x5a, 0x50},   // Выходная ширина DSP 800 / 10 = 80 (0x50)
    {0x5b, 0x3c},   // Выходная высота DSP 600 / 10 = 60 (0x3C)

    {0xd7, 0x03},   // RESERVED

    /**************************** НАСТРОЙКА МАТРИЦЫ СЕНСОРА (Bank 1) **************************************************/
    {0xff, 0x01},   // переход на Table 1
    {0x03, 0x06},
    {0x11, 0x8F},   // регистр CLKRC: делитель частоты встроенного кварца камеры
    {0x15, 0x22},   // регистр COM10: выбрана отрицательная полярность VSYNC, т.е. VSYNC имеет высокий уровень во время кадра, а при начале и завершении кадра становится низким
    {0x04, 0xC8},   // регистр REG04: синхронизация HREF и DCLK

    // ==== ДОБАВЛЕНЫ КРИТИЧЕСКИЕ РЕГИСТРЫ СБОРКИ SLAVE MODE ПО ДАТАШИТУ ====
    {0x09, 0x12},   // COM2: Взводим Bit 4 (Аппаратное включение Slave Mode для логики кристалла)
    {0x2c, 0x0c},   // Выравнивание баланса каналов АЦП (Убирает вашу полосатую "зебру")
    {0x3a, 0x33},   // Коррекция темновых токов
    {0x3b, 0xfb},   // Стабилизация межстрочных смещений

    /** запуск конвейера */
    {0xff, 0x01},   // переход на Table 1

    // ИСПРАВЛЕНО ДЛЯ SLAVE MODE: В регистре COM7 принудительно взводим Bit 7 (AEC/AGC Slave Enable)
    // Маска: 0x44 (SVGA) + 0x80 (Slave) = 0xC4
    {0x12, 0xC4},   // регистр COM7: Выбор разрешения SVGA + Включение Slave-затвора AEC/AGC

    {0xff, 0x00},   // Переходим в Bank 0 для фиксации сброса DVP
    {0xE0, 0x04},   // Выводим скейлер из сброса
    {0xE0, 0x00},   // Запускаем выходные буферы DVP
    {0x05, 0x00},   // Отключаем R_BYPASS, конвейер Slave Mode запущен и ждет пинка от STM32
    {0x00, 0x00}
};

/**********************************************************************************************************************/
// Запись настроек в камеру через I2C
static I2C_Status_t write_register_table(const ov2640_reg_t* register_table, uint8_t device_address)
{
    I2C_Status_t I2C_status;
    uint32_t reg_idx = 0;
    while (!(register_table[reg_idx].reg == 0x00 && register_table[reg_idx].val == 0x00))
    {
        uint8_t reg = register_table[reg_idx].reg;
        uint8_t val = register_table[reg_idx].val;

        I2C_status = I2C_Write_Reg(I2C2, device_address, reg, val);
        delay_ms(10);

        reg_idx++;
    }

    if (I2C_status != I2C_OK)
    {
        while(1)
        {
            GPIO_toggle_Pin(GPIOD, 14);
            delay_ms(250);
        }
    }

    return I2C_status;
}

/**********************************************************************************************************************/
// РАБОЧАЯ ВЕРСИЯ
/** Инициализация камеры в режиме Master Mode */
void ov2640_Init_Master_Mode(uint8_t device_address)
{
    ov2640_Reset_Master_Mode();

    I2C_Status_t I2C_status;    // Статус операции приема/передачи по I2C

    I2C_status = I2C_Write_Reg(I2C2, device_address, 0xFF, 0x01);   // Table 1
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x12, 0x80);   // Программный сброс
    delay_ms(100);

    ov2640_Read_ID_Master_Mode(device_address);

    I2C_status = write_register_table(ov2640_config_Master_Mode, 0x30);         // Запись таблицы регистров

    // Добавить настройку яркости и контрастности
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0xff, 0x00); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7c, 0x00); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0x04); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7c, 0x07); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0x00); delay_ms(5);  // Значение яркости
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0x28); delay_ms(5);  // Младший байт контрастности
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0xc2); delay_ms(5);  // Старший байт контрастности
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0x06); delay_ms(5);

    if (I2C_status == I2C_OK)
    {
        GPIO_set_HIGH(GPIOD, 15);   // Синий загорелся - настройки камеры записаны успешно
        delay_ms(1000);
        GPIO_set_LOW(GPIOD, 15);    // Синий погас - переход к захвату кадра
    }
}

/** Захват кадра камеры в режиме Master Mode */
int ov2640_capture_snapshot_Master_Mode(uint8_t *buffer, int width, int height)
{
    int lines_processed = 0;
    uint8_t *p_buf = buffer;

    while (VSYNC_IS_HIGH)   { SysTick_Update_us(); }    // Если кадр уже начался - ожидание конца кадра
    while (!VSYNC_IS_HIGH)  { SysTick_Update_us(); }    // Ожидание начала нового кадра

/*  // Пропуск верхних 175 строк
    for (int s = 0; s < 120; s++)
    {
        while (!HREF_IS_HIGH);  // Ожидание начала строки
        while (HREF_IS_HIGH);   // Ожидание конца строки
    }
*/
    // Запись CAM_HEIGHT строк в буфер
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

        /** пропуск темной строки */
        while (!HREF_IS_HIGH);  // Ожидание начала строки
        while (HREF_IS_HIGH);   // Ожидание конца строки
    }
    return lines_processed;
}




//









/** Захват кадра + бинаризация на лету + упаковка в сжатый массив */
int ov2640_capture_and_process_Master_Mode(uint8_t *packed_buffer,  // упакованный бинарный кадр
                                           int width, int height,   // размеры кадра
                                           uint8_t get_binary)      // флаг "нужно выполнить бинаризацию"
{
    int lines_processed = 0;    // обработанные строки
    uint8_t *p_packed = packed_buffer;  // буфер упакованного ЧБ кадра

    // Параметры для бинаризации на лету
    const uint32_t s = 3;
    const uint32_t t = 14;
    uint32_t running_average = 30 << s; // начальный порог ЧБ 30

    // Переменные для битовой упаковки на лету
    uint8_t bit_accumulator = 0;
    uint8_t bit_count = 0;

    while (VSYNC_IS_HIGH)   { SysTick_Update_us(); }    // Если кадр уже начался - ожидание конца кадра
    while (!VSYNC_IS_HIGH)  { SysTick_Update_us(); }    // Ожидание начала нового кадра

    // Запись CAM_HEIGHT строк в буфер
    for (int y = 0; y < height; y++)
    {
        while (!HREF_IS_HIGH);  // Ожидание начала строки

        // Чтение 400 пикселей яркости (800 тактов DCLK)
        for (int x = 0; x < width; x++)
        {
            // Нечетный такт (байт яркости)
            while (!DCLK_IS_HIGH);
            uint8_t current_pixel = (uint8_t)((DATA_PORT->IDR >> 8) & 0xFF);
            while (DCLK_IS_HIGH);

            if (get_binary)
            {
                // Математика скользящего среднего Эйвеля (занимает ~10 тактов процессора)
                running_average = running_average - (running_average >> s) + current_pixel;
                uint32_t local_threshold = (running_average >> s) * (100 - t) / 100;

                // Упаковываем пиксель в текущий бит накапливаемого байта
                if (current_pixel > local_threshold)
                {
                    bit_accumulator |= (1 << (7 - bit_count)); // 1 = Белый пиксель
                }
                // Если черный (0) - бит автоматически остается нулем

                bit_count++;
                if (bit_count == 8) {
                    *p_packed++ = bit_accumulator; // Записываем готовые 8 пикселей (1 байт) в ОЗУ
                    bit_accumulator = 0;
                    bit_count = 0;
                }
            }

            // все 800 байт в строке уже яркость, цвета нет вообще, пропускать нечего
//            // Четный такт (байт цветности)
//            while (!DCLK_IS_HIGH);
//            while (DCLK_IS_HIGH);
        }
        lines_processed++;

        while (HREF_IS_HIGH);   // Ожидание конца строки

//        /** пропуск темной строки */
//        while (!HREF_IS_HIGH);  // Ожидание начала строки
//        while (HREF_IS_HIGH);   // Ожидание конца строки
    }
    return lines_processed;
}


// КОПИЯ функции, здесь бинаризация и сжатие на лету + сохранение исходного кадра
///** Захват кадра + бинаризация на лету + упаковка в сжатый массив */
//int ov2640_capture_and_process_Master_Mode(uint8_t *buffer,         // исходный кадр
//                                           uint8_t *packed_buffer,  // упакованный бинарный кадр
//                                           int width, int height,   // размеры кадра
//                                           uint8_t get_binary)      // флаг "нужно выполнить бинаризацию"
//{
//    int lines_processed = 0;    // обработанные строки
//    uint8_t *p_buf = buffer;            // буфер для исходного кадра
//    uint8_t *p_packed = packed_buffer;  // буфер упакованного ЧБ кадра
//
//    // Параметры для бинаризации на лету
//    const uint32_t s = 4;
//    const uint32_t t = 14;
//    uint32_t running_average = 30 << s; // начальный порог ЧБ 30
//
//    // Переменные для битовой упаковки на лету
//    uint8_t bit_accumulator = 0;
//    uint8_t bit_count = 0;
//
//    while (VSYNC_IS_HIGH)   { SysTick_Update_us(); }    // Если кадр уже начался - ожидание конца кадра
//    while (!VSYNC_IS_HIGH)  { SysTick_Update_us(); }    // Ожидание начала нового кадра
//
//    // Запись CAM_HEIGHT строк в буфер
//    for (int y = 0; y < height; y++)
//    {
//        while (!HREF_IS_HIGH);  // Ожидание начала строки
//
//        // Чтение 400 пикселей яркости (800 тактов DCLK)
//        for (int x = 0; x < width; x++)
//        {
//            // Нечетный такт (байт яркости)
//            while (!DCLK_IS_HIGH);
//            uint8_t current_pixel = (uint8_t)((DATA_PORT->IDR >> 8) & 0xFF);
//            while (DCLK_IS_HIGH);
//
//            *p_buf++ = current_pixel;
//
//            if (get_binary)
//            {
//                // Математика скользящего среднего Эйвеля (занимает ~10 тактов процессора)
//                running_average = running_average - (running_average >> s) + current_pixel;
//                uint32_t local_threshold = (running_average >> s) * (100 - t) / 100;
//
//                // Упаковываем пиксель в текущий бит накапливаемого байта
//                if (current_pixel > local_threshold)
//                {
//                    bit_accumulator |= (1 << (7 - bit_count)); // 1 = Белый пиксель
//                }
//                // Если черный (0) - бит автоматически остается нулем
//
//                bit_count++;
//                if (bit_count == 8) {
//                    *p_packed++ = bit_accumulator; // Записываем готовые 8 пикселей (1 байт) в ОЗУ
//                    bit_accumulator = 0;
//                    bit_count = 0;
//                }
//            }
//
//            // Четный такт (байт цветности)
//            while (!DCLK_IS_HIGH);
//            while (DCLK_IS_HIGH);
//        }
//        lines_processed++;
//
//        while (HREF_IS_HIGH);   // Ожидание конца строки
//
//        /** пропуск темной строки */
//        while (!HREF_IS_HIGH);  // Ожидание начала строки
//        while (HREF_IS_HIGH);   // Ожидание конца строки
//    }
//    return lines_processed;
//}





/** Определить длительность кадра, длительность строки, количество строк и количество байт в строке */
void ov2640_count_pixels_in_frame_Master_Mode()
{
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
}

/** Получить ID камеры */
void ov2640_Read_ID_Master_Mode(uint8_t device_address)
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

/** Сброс камеры */
void ov2640_Reset_Master_Mode()
{
    // Физический сброс питания и логики камеры
    GPIO_set_LOW(GPIOB, 0);  // PWDN = 0
    GPIO_set_LOW(GPIOB, 1);  // RESET = 0
    delay_ms(50);
    GPIO_set_HIGH(GPIOB, 1); // RESET = 1
    delay_ms(300);
}

/**********************************************************************************************************************/




/** Инициализация камеры в режиме Slave Mode */
void ov2640_Init_Slave_Mode(uint8_t device_address)
{
    ov2640_Reset_Master_Mode();

    I2C_Status_t I2C_status;    // Статус операции приема/передачи по I2C

    I2C_status = I2C_Write_Reg(I2C2, device_address, 0xFF, 0x01);   // Table 1
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x12, 0x80);   // Программный сброс
    delay_ms(100);

    ov2640_Read_ID_Master_Mode(device_address);

    I2C_status = write_register_table(ov2640_config_Slave_Mode, 0x30);         // Запись таблицы регистров

//    I2C_Write_Reg(I2C2, device_address, 0xFF, 0x01); delay_ms(5);
//    I2C_Write_Reg(I2C2, device_address, 0x12, 0x4C); delay_ms(5);
//    I2C_Write_Reg(I2C2, device_address, 0x11, 0x57); delay_ms(5);
//    I2C_Write_Reg(I2C2, device_address, 0x09, 0x04); delay_ms(5);

    // Добавить настройку яркости и контрастности
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0xff, 0x00); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7c, 0x00); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0x04); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7c, 0x07); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0x00); delay_ms(5);  // Значение яркости
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0x28); delay_ms(5);  // Младший байт контрастности
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0xc2); delay_ms(5);  // Старший байт контрастности
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0x06); delay_ms(5);

    if (I2C_status == I2C_OK)
    {
        GPIO_set_HIGH(GPIOD, 15);   // Синий загорелся - настройки камеры записаны успешно
        delay_ms(1000);
        GPIO_set_LOW(GPIOD, 15);    // Синий погас - переход к захвату кадра
    }
}

/** Захват кадра камеры в режиме Slave Mode*/
int ov2640_capture_snapshot_Slave_Mode(uint8_t *buffer, int width, int height)
{

    return 0;
}

















/**************** Исходные рабочие варианты ***************************************************************************/
//// Настройки камеры в режиме Master Mode
//const ov2640_reg_t ov2640_config_Master_Mode[] =
//{
//    /***************************** НАСТРОЙКА МАТРИЦЫ И СИНХРОНИЗАЦИИ (Bank 1) *****************************************/
//    {0xff, 0x01},   // переход на Table 1
//    {0x11, 0x17},   // регистр CLKRC: делитель частоты встроенного кварца камеры
//    {0x15, 0x22},   // регистр COM10: выбрана отрицательная полярность VSYNC, т.е. VSYNC имеет высокий уровень во время кадра, а при начале и завершении кадра становится низким
//    {0x04, 0xC8},   // регистр REG04: синхронизация HREF и DCLK
//
//    /***************************** НАСТРОЙКА ЦИФРОВОГО ПОТОКА (Bank 0) ************************************************/
//    {0xff, 0x00},   // переход на Table 0
//    {0xE0, 0x04},   // регистр RESET: сброс модуля DVP              ВЗЯЛ ИЗ НАСТРОЙКИ YUV422
//    {0xDA, 0x00},
//    {0xD7, 0x01},
//    {0xE1, 0x67},
//    {0xE0, 0x00},
//
//    {0xd3, 0x0F},               // регистр R_DVP_SP: делитель PCLK
//    {0x2c, 0xff}, {0x2e, 0xdf}, // настройка PLL (в даташите помечены как RESERVED)
//
//    /********************************** НАСТРОЙКА ГЕОМЕТРИИ  **********************************************************/
//    {0xC0, 0x64},   //                                                  ВЗЯЛ ИЗ SVGA
//    {0xC1, 0x4B},
//
//
//    {0xFF, 0x00},   //                                                  ВЗЯЛ НАСТРОЙКИ ОКНА
//    {0x51, 0x20},   // max_x & 0xFF         max_x = 0x320
//    {0x52, 0x58},   // max_y & 0xFF         max_y = 0x258
//    {0x53, 0x00},   // offset_x & 0xFF      offset_x = 0x0
//    {0x54, 0x00},   // offset_y & 0xFF      offset_y = 0x0
//    {0x55, 0x08},
//    {0x57, 0X80},
//    {0x5A, 0x20},   // w = 0x320
//    {0x5B, 0x58},   // h = 0x258
//    {0x5C, 0x03},
//
//    {0x86, 0x3d},   // Включение модуля масштабирования                 ПЕРЕНЕС СЮДА
//
//    {0xe1, 0x67},   // RESERVED
//    {0xe5, 0x1f},   // RESERVED
//    {0x5a, 0x14},   // Выходная ширина DSP / 10 = 16 (0x10)
//    {0x5b, 0x0f},   // Выходная высота DSP / 10 = 12 (0x0C)
//
//    {0xd7, 0x03},   // RESERVED
//
//    /**************************** НАСТРОЙКА МАТРИЦЫ СЕНСОРА (Bank 1) **************************************************/
//    {0xff, 0x01},   // переход на Table 1
//    {0x03, 0x06},
//
//    /** запуск конвейера */
//    {0xff, 0x01},   // переход на Table 1
//    {0x12, 0x44},   // регистр COM7: выбор разрешения SVGA
//    {0x00, 0x00}    // Конец таблицы
//};


// РАБОЧАЯ ВЕРСИЯ КОПИЯ
/** Инициализация камеры: запись регистров по I2C */
//void ov2640_Init(uint8_t device_address)
//{
//    I2C_Status_t I2C_status;    // Статус операции приема/передачи по I2C
//
//    I2C_status = I2C_Write_Reg(I2C2, device_address, 0xFF, 0x01); // Table 1
//    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x12, 0x80); // Программный сброс
//    delay_ms(100);
//
//    // Запись регистров по I2C
//    uint32_t reg_idx = 0;
//    while (!(ov2640_config[reg_idx].reg == 0x00 && ov2640_config[reg_idx].val == 0x00))
//    {
//        uint8_t reg = ov2640_config[reg_idx].reg;
//        uint8_t val = ov2640_config[reg_idx].val;
//
//        I2C_status = I2C_Write_Reg(I2C2, device_address, reg, val);
//        delay_ms(10);
//
//        // Если запись таблицы оборвалась посередине — МЕДЛЕННОЕ мигание КРАСНОГО
//        if (I2C_status != I2C_OK)
//        {
//            while(1)
//            {
//                GPIO_toggle_Pin(GPIOD, 14);
//                delay_ms(250);
//            }
//        }
//        reg_idx++;
//    }
//    I2C_status = I2C_Write_Reg(I2C2, device_address, 0xff, 0x01); delay_ms(5);
//    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x12, 0x44); delay_ms(5);
//
//    // добавить настройку яркости и контрастности
//    I2C_status = I2C_Write_Reg(I2C2, device_address, 0xff, 0x00); delay_ms(5);
//    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7c, 0x00); delay_ms(5);
//    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0x04); delay_ms(5);
//    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7c, 0x07); delay_ms(5);
//    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0x40); delay_ms(5);  // Значение яркости
//    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0x28); delay_ms(5);  // Младший байт контрастности
//    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0xc2); delay_ms(5);  // Старший байт контрастности
//    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0x06); delay_ms(5);
//}
/**********************************************************************************************************************/




