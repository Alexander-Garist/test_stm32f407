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
const ov2640_reg_t ov2640_config[] =
{
    {0xff, 0x00},
    {0x05, 0x01},
    {0xe0, 0x04},
    {0xda, 0x40},
    {0xd7, 0x01},
    {0xe1, 0x67},
    {0xe0, 0x00},
    {0xd3, 0x0f},
    {0x2c, 0xff},
    {0x2e, 0xdf},
    {0xc0, 0x64},
    {0xc1, 0x4b},
    {0x51, 0x20},
    {0x52, 0x58},
    {0x53, 0x00},
    {0x54, 0x00},
    {0x55, 0x08},
    {0x57, 0x80},
    {0x5a, 0x20},
    {0x5b, 0x58},
    {0x5c, 0x03},
    {0x86, 0x3d},
    {0xe1, 0x67},
    {0xe5, 0x1f},
    {0xd7, 0x03},
    {0xff, 0x01},
    {0x03, 0x0a},
    {0x11, 0x17},
    {0x15, 0x22},
    {0x04, 0xc8},
    {0xff, 0x00},
    {0xff, 0x00},
    {0x7c, 0x00},
    {0x7d, 0x00},
    {0x7c, 0x02},
    {0x7d, 0xc4},
    {0x7c, 0x03},
    {0x7d, 0x0c},
    {0x90, 0x00},
    {0x91, 0x0e},
    {0x91, 0x1a},
    {0x91, 0x31},
    {0x91, 0x5a},
    {0x91, 0x69},
    {0x91, 0x75},
    {0x91, 0x7e},
    {0x91, 0x88},
    {0x91, 0x8f},
    {0x91, 0x96},
    {0x91, 0xa3},
    {0x91, 0xaf},
    {0x91, 0xc4},
    {0x91, 0xd7},
    {0x91, 0xe8},
    {0x91, 0x20},
    {0xff, 0x01},
    {0x24, 0x40},
    {0x25, 0x38},
    {0x26, 0x82},
    {0x12, 0x44},
    {0xff, 0x00},
    {0xe0, 0x04},
    {0xe0, 0x00},

    {0x00, 0x00}    // Конец таблицы
};

/** Получить ID камеры */
static void ov2640_Read_ID(uint8_t device_address)
{
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
static void ov2640_Reset()
{
    // Физический сброс питания и логики камеры
    GPIO_set_LOW(GPIOB, 0);  // PWDN = 0
    GPIO_set_LOW(GPIOB, 1);  // RESET = 0
    delay_ms(50);
    GPIO_set_HIGH(GPIOB, 1); // RESET = 1
    delay_ms(300);
}

/** Инициализация камеры без использования таблицы, каждая команда прописана вручную */
/* Камера выдает кадр 800*600, причем все 800 байт это значения яркости пикселей */
void ov2640_Init(uint8_t device_address)
{
    ov2640_Reset(); // аппаратный сброс

    I2C_Status_t I2C_status;    // Статус операции приема/передачи по I2C

    I2C_status = I2C_Write_Reg(I2C2, device_address, 0xff, 0x01);   			// Переключение банка регистров на Table 1
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x12, 0x80);   			// Программный сброс
    delay_ms(100);

    ov2640_Read_ID(device_address);

    /*************** Запись таблицы регистров *******************************/
/*
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0xff, 0x00); delay_ms(5);	// Переключение банка регистров на Table 0
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0x05, 0x01); delay_ms(5);	// R_BYPASS (отключить DSP на время настройки)
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0xe0, 0x04); delay_ms(5);	// RESET (сбросить DVP)
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0xda, 0x40); delay_ms(5);	// IMAGE_MODE (по умолчанию: без сжатия JPEG, YUV422, порядок байт YUYV)
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0xd7, 0x01); delay_ms(5);		// RESERVED (в даташите нет информации)
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0xe1, 0x67); delay_ms(5);		// RESERVED (в даташите нет информации)
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0xe0, 0x00); delay_ms(5);	// RESET (включить DVP)
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0xd3, 0x0f); delay_ms(5);	// R_DVP_SP: PCLK = sysclk / 16 = 750 кГц (бьет по осциллографу) частота DCLK
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0x2c, 0xff); delay_ms(5);	// Включение PLL
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0x2e, 0xdf); delay_ms(5);	// Включение PLL
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0xc0, 0x64); delay_ms(5);	// IMAGE H_SIZE[10:3] = 0x64 = 100 => IMAGE H_SIZE = 100*8 = 800 (800 тактов PCLK в одной строке)
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0xc1, 0x4b); delay_ms(5);	// IMAGE V_SIZE[10:3] = 0x4b = 75  => IMAGE W_SIZE = 75*8 = 600  (600 строк в одном кадре)
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0x51, 0x20); delay_ms(5);	// взято из примера 	// max_x 	& 0xFF		max_x = 0x320
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0x52, 0x58); delay_ms(5);	// взято из примера 	// max_y 	& 0xFF		max_y = 0x258
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0x53, 0x00); delay_ms(5);	// взято из примера 	// offset_x & 0xFF      offset_x = 0x0
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0x54, 0x00); delay_ms(5);	// взято из примера 	// offset_y & 0xFF      offset_y = 0x0
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0x55, 0x08); delay_ms(5);	// взято из примера
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0x57, 0x80); delay_ms(5);	// взято из примера
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0x5a, 0x20); delay_ms(5);	// взято из примера		// w = 0x320
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0x5b, 0x58); delay_ms(5);	// взято из примера		// h = 0x258
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0x5c, 0x03); delay_ms(5);	// взято из примера
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0x86, 0x3d); delay_ms(5);	// CTRL2: +DCW +SDE (хз что это)
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0xe1, 0x67); delay_ms(5);		// RESERVED (в даташите нет информации)
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0xe5, 0x1f); delay_ms(5);		// RESERVED (в даташите нет информации)
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0xd7, 0x03); delay_ms(5);		// RESERVED (в даташите нет информации)
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0xff, 0x01); delay_ms(5);	// Переключение банка регистров на Table 1
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0x03, 0x0a); delay_ms(5);	// COM1: по умолчанию для SVGA 0x0A
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0x11, 0x17); delay_ms(5);	// CLKRC: делитель системной частоты 1 (по умолчанию)
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0x15, 0x22); delay_ms(5);	// COM10: VSYNC отрицательная полярность + PCLK генерируется только во время высокого уровня HREF
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0x04, 0xc8); delay_ms(5);	// REG04: HREF 1, отражение по вертикали и горизонтали
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0xff, 0x00); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0xff, 0x00); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7c, 0x00); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0x00); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7c, 0x02); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0xc4); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7c, 0x03); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x7d, 0x0c); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x90, 0x00); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x91, 0x0e); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x91, 0x1a); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x91, 0x31); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x91, 0x5a); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x91, 0x69); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x91, 0x75); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x91, 0x7e); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x91, 0x88); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x91, 0x8f); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x91, 0x96); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x91, 0xa3); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x91, 0xaf); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x91, 0xc4); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x91, 0xd7); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x91, 0xe8); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x91, 0x20); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0xff, 0x01); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x24, 0x40); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x25, 0x38); delay_ms(5);
    I2C_status = I2C_Write_Reg(I2C2, device_address, 0x26, 0x82); delay_ms(5);
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0x12, 0x44); delay_ms(5);	// COM7: zoom mode + разрешение SVGA
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0xff, 0x00); delay_ms(5);	// Переключение банка регистров на Table 0
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0xe0, 0x04); delay_ms(5);	// RESET (сбросить DVP)
	I2C_status = I2C_Write_Reg(I2C2, device_address, 0xe0, 0x00); delay_ms(5);	// RESET (включить DVP)
*/
    // Вместо записи вручную каждого регистра сделать цикл
    for (uint32_t index = 0; ; index++)
    {
        if (ov2640_config[index].reg == 0x0 && ov2640_config[index].val == 0x0) break;
        I2C_status = I2C_Write_Reg(I2C2, device_address, ov2640_config[index].reg, ov2640_config[index].val);
        delay_ms(5);
    }

    if (I2C_status == I2C_OK)
    {
        GPIO_set_HIGH(GPIOD, 15);   // Синий загорелся - настройки камеры записаны успешно
        delay_ms(1000);
        GPIO_set_LOW(GPIOD, 15);    // Синий погас - переход к захвату кадра
    }
}

/** Определить длительность кадра, длительность строки, количество строк и количество байт в строке */
void ov2640_count_pixels_in_frame()
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

/** Захват кадра + бинаризация на лету + упаковка в сжатый массив  БЕЗ СОХРАНЕНИЯ ИСХОДНОГО КАДРА*/
int ov2640_capture_and_process(uint8_t *packed_buffer, int width, int height)
{
    int lines_processed = 0;    // обработанные строки
    uint8_t *p_packed = packed_buffer;  // буфер упакованного ЧБ кадра

    // Параметры для бинаризации на лету
    const uint32_t s = 5;
    const uint32_t t = 12;
    uint32_t running_average = 30 << s; // начальный порог ЧБ 30

    // Переменные для битовой упаковки на лету
    uint8_t bit_accumulator = 0;
    uint8_t bit_count = 0;

    while (VSYNC_IS_HIGH)   { SysTick_Update_us(); }    // Если кадр уже начался - ожидание конца кадра
    while (!VSYNC_IS_HIGH)  { SysTick_Update_us(); }    // Ожидание начала нового кадра

    // Запись CAM_HEIGHT строк в буфер
    for (int y = 0; y < height / 2; y++)
    {
        while (!HREF_IS_HIGH);  // Ожидание начала строки

        // Чтение 400 пикселей яркости (из 800 тактов DCLK)
        for (int x = 0; x < width / 2; x++)
        {
            // Нечетный такт (байт яркости)
            while (!DCLK_IS_HIGH);
            uint8_t current_pixel = (uint8_t)((DATA_PORT->IDR >> 8) & 0xFF);
            while (DCLK_IS_HIGH);

            if (current_pixel < 150) current_pixel = (current_pixel * current_pixel) / 150;
            else current_pixel = 255;

            if (x == 0) running_average = current_pixel << s;

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

            // пропуск четного байта
            while (!DCLK_IS_HIGH);
            while (DCLK_IS_HIGH);
        }
        lines_processed++;

        while (HREF_IS_HIGH);   // Ожидание конца строки

        while (!HREF_IS_HIGH);  // Ожидание начала строки
        while (HREF_IS_HIGH);   // Ожидание конца строки

    }
    return lines_processed;
}












