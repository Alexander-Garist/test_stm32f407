#include <stdio.h>
#include "stm32f4xx.h"
#include "systick.h"
#include "gpio.h"
#include "i2c.h"

#include "image_processing.h"


/**********************************************************************************************************************/
#define CAM_WIDTH        160
#define CAM_HEIGHT       120
#define CAM_FRAME_BYTES  (CAM_WIDTH * CAM_HEIGHT * 2)

uint8_t camera_buffer[CAM_FRAME_BYTES];


typedef struct
{
    uint8_t reg;
    uint8_t val;
}
ov2640_reg_t;

#define PAUSE   {0x00, 0xff}

// Регистры
#define SELECT_R_TABLE  0xFF
#define COM1_R          0x03
#define COM2_R          0x09
#define COM3_R          0x0C
#define COM4_R          0x0D
#define COM7_R          0x12
#define COM8_R          0x13
#define COM9_R          0x14
#define COM10_R         0x15
#define CLKRC_R         0x11
#define REG04_R         0x04
#define ARCOM2_R        0x34
#define COM19_R         0x48

// Значения регистров
#define TABLE_0         0x00
#define TABLE_1         0x01
#define SYSTEM_RESET    0x80


// Настройка регистров OV2640 как в примере
//const ov2640_reg_t ov2640_config[] =
//{
//    /******************************************* Table 1 **************************************************************/
//    {SELECT_R_TABLE, TABLE_1},
//    {COM7_R, SYSTEM_RESET},
//    PAUSE,
//
//    /******************************************* Table 0 **************************************************************/
//    {SELECT_R_TABLE, TABLE_0},
//    {0x2c, 0xff}, {0x2e, 0xdf},                                             // Reserved registers
//
//    /******************************************* Table 1 **************************************************************/
//    {SELECT_R_TABLE, TABLE_1},
//    {0x3c, 0x32},                                                           // Reserved registers
//
//    {CLKRC_R,   0x01},  // CLK = 12 МГц / (1+1)
//    {COM2_R,    0x02},  // Выходная пропускная способность
//    {REG04_R,   0x28},  // Включение HREF
//    {COM8_R,    0xe5},  // Полосовой фильтр включен
//    {COM9_R,    0x48},
//    {0x2c, 0x0c}, {0x33, 0x78}, {0x3a, 0x33}, {0x3b, 0xfb},                 // Reserved registers
//    {0x3e, 0x00}, {0x43, 0x11}, {0x16, 0x10}, {0x39, 0x92},                 // Reserved registers
//    {0x35, 0xda}, {0x22, 0x1a}, {0x37, 0xc3}, {0x23, 0x00},                 // Reserved registers
//    {ARCOM2_R,  0xc0},
//    {0x36, 0x1a}, {0x06, 0x88}, {0x07, 0xc0},                               // Reserved registers
//    {COM4_R,    0x87},
//    {0x0e, 0x41}, {0x4c, 0x00},
//    {COM19_R,   0x00},
//    {0x5b, 0x00}, {0x42, 0x03}, {0x4a, 0x81}, {0x21, 0x99},                 // Reserved registers
//
//    {0x24, 0x40}, {0x25, 0x38}, {0x26, 0x82},   // AEW AEB VV
//    {0x5c, 0x00}, {0x63, 0x00},                                             // Reserved registers
//    {0x61, 0x70}, {0x62, 0x80}, // HISTO_LOW HISTO_HIGH
//    {0x7c, 0x05}, {0x20, 0x80}, {0x28, 0x30}, {0x6c, 0x00},                 // Reserved registers
//    {0x6d, 0x80}, {0x6e, 0x00}, {0x70, 0x02}, {0x71, 0x94}, {0x73, 0xc1},   // Reserved registers
//
//    {COM7_R,    0x40},
//    {0x17, 0x11}, {0x18, 0x43}, // HREFST HREFEND
//    {0x19, 0x00}, {0x1a, 0x4b}, // VSTRT VEND
//    {0x32, 0x09},               // REG32
//    {0x37, 0xc0},                                                           // Reserved registers
//    {0x4f, 0x60}, {0x50, 0xa8}, // BD50 BD60
//    {0x6d, 0x00}, {0x3d, 0x38},                                             // Reserved registers
//
//    {0x46, 0x3f},   // FLL
//    {0x4f, 0x60},   // BD50
//    {COM3_R, 0x3c},
//
//    /******************************************* Table 0 **************************************************************/
//    {SELECT_R_TABLE, TABLE_0},
//    {0xe5, 0x7f},                                                           // Reserved registers
//    {0xf9, 0xc0},   // MC_BIST
//    {0x41, 0x24},                                                           // Reserved registers
//    {0xe0, 0x14},   // RESET
//    {0x76, 0xff}, {0x33, 0xa0}, {0x42, 0x20}, {0x43, 0x18}, {0x4c, 0x00},   // Reserved registers
//    {0x87, 0xd5},   // CTRL3
//    {0x88, 0x3f}, {0xd7, 0x03}, {0xd9, 0x10},                               // Reserved registers
//    {0xd3, 0x82},   // R_DVP_SP
//
//    {0xc8, 0x08}, {0xc9, 0x80},                                             // Reserved registers
//
//    {0x7c, 0x00}, {0x7d, 0x00},                             // BPADDR BPDATA
//    {0x7c, 0x03}, {0x7d, 0x48}, {0x7d, 0x48},               // BPADDR BPDATA
//    {0x7c, 0x08}, {0x7d, 0x20}, {0x7d, 0x10}, {0x7d, 0x0e}, // BPADDR BPDATA
//
//    {0x90, 0x00},                                                           // Reserved registers
//    {0x91, 0x0e}, {0x91, 0x1a}, {0x91, 0x31}, {0x91, 0x5a}, {0x91, 0x69},   // Reserved registers
//    {0x91, 0x75}, {0x91, 0x7e}, {0x91, 0x88}, {0x91, 0x8f}, {0x91, 0x96},   // Reserved registers
//    {0x91, 0xa3}, {0x91, 0xaf}, {0x91, 0xc4}, {0x91, 0xd7}, {0x91, 0xe8},   // Reserved registers
//    {0x91, 0x20},                                                           // Reserved registers
//    {0x92, 0x00},                                                           // Reserved registers
//    {0x93, 0x06}, {0x93, 0xe3}, {0x93, 0x05}, {0x93, 0x05}, {0x93, 0x00},   // Reserved registers
//    {0x93, 0x04}, {0x93, 0x00}, {0x93, 0x00}, {0x93, 0x00}, {0x93, 0x00},   // Reserved registers
//    {0x93, 0x00}, {0x93, 0x00}, {0x93, 0x00},                               // Reserved registers
//    {0x96, 0x00},                                                           // Reserved registers
//    {0x97, 0x08}, {0x97, 0x19}, {0x97, 0x02}, {0x97, 0x0c}, {0x97, 0x24},   // Reserved registers
//    {0x97, 0x30}, {0x97, 0x28}, {0x97, 0x26}, {0x97, 0x02}, {0x97, 0x98},   // Reserved registers
//    {0x97, 0x80}, {0x97, 0x00}, {0x97, 0x00},                               // Reserved registers
//
//    {0xc3, 0xed},   // CTRL1
//    {0xa4, 0x00}, {0xa8, 0x00}, {0xc5, 0x11}, {0xc6, 0x51}, {0xbf, 0x80},   // Reserved registers
//    {0xc7, 0x10},                                                           // Reserved registers
//    {0xb6, 0x66}, {0xb8, 0xA5}, {0xb7, 0x64}, {0xb9, 0x7C}, {0xb3, 0xaf},   // Reserved registers
//    {0xb4, 0x97}, {0xb5, 0xFF}, {0xb0, 0xC5}, {0xb1, 0x94}, {0xb2, 0x0f},   // Reserved registers
//    {0xc4, 0x5c},                                                           // Reserved registers
//    {0xc0, 0x64},   // HSIZE
//    {0xc1, 0x4B},   // VSIZE
//    {0x8c, 0x00},   // SIZEL
//    {0x86, 0x3D},   // CTRL2
//    {0x50, 0x00},   // CTRLI
//    {0x51, 0xC8},   // HSIZE
//    {0x52, 0x96},   // VSIZE
//    {0x53, 0x00},   // XOFFL
//    {0x54, 0x00},   // YOFFL
//    {0x55, 0x00},   // VHYX
//    {0x5a, 0xC8},   // ZMOW
//    {0x5b, 0x96},   // ZMOH
//    {0x5c, 0x00},   // ZMHH
//    {0xd3, 0x82},   // R_DVP_SP
//    {0xc3, 0xed},   // CTRL1
//    {0x7f, 0x00}, {0xda, 0x00}, {0xe5, 0x1f}, {0xe1, 0x67},                 // Reserved registers
//    {0xe0, 0x00},   // RESET
//    {0xdd, 0x7f},                                                           // Reserved registers
//    {0x05, 0x00},   // R_BYPASS
//};
/**********************************************************************************************************************/
const ov2640_reg_t ov2640_config[] =
{
    // ==================== СБРОС И СТАБИЛИЗАЦИЯ ====================
    {0xff, 0x01}, // Sensor Bank (Bank 1)
    {0x12, 0x80}, // Software Reset (Заводской сброс)
    {0x00, 0xff}, // Пауза на стабилизацию кристалла

    // ==================== НАСТРОЙКА МАТРИЦЫ И СИНХРОНИЗАЦИИ (Bank 1) ====================
    {0xff, 0x01}, // Строго Sensor Bank
    {0x11, 0x0B}, // CLKRC: Делитель частоты матрицы = 2 (Стабильный аналоговый режим)
    {0x15, 0x02}, // COM10: Положительная полярность VSYNC
    {0x04, 0x08}, // REG04: ИСПРАВЛЕНО! Включаем HREF строго в Sensor Bank

    // ==================== НАСТРОЙКА ЦИФРОВОГО ПОТОКА (Bank 0) ====================
    {0xff, 0x00}, // Переходим в DSP Bank (Bank 0)
    {0xe0, 0x04}, // Входим в режим конфигурирования конвейера DVP

    {0xda, 0x00}, // IMAGE_MODE: ИСПРАВЛЕНО! Отключаем JPEG, включаем построчный YUV
    {0xc2, 0x0c}, // CTRL0: ИСПРАВЛЕНО! Активируем процессор обработки YUV422
    {0xd3, 0x00},

    {0x2c, 0xff}, {0x2e, 0xdf},
    {0x08, 0x00},
    {0xc8, 0x06}, // DA_BYTE_EN: Выходная последовательность YUYV

    // ==================== МАТРИЦА ГЕОМЕТРИИ (СТРОГО 160x120) ====================
    {0xc0, 0x28}, // HSIZE: 160 / 4 = 0x28 (Размер строки QQVGA)
    {0xc1, 0x1E}, // VSIZE: 120 / 4 = 0x1E (Размер кадра по вертикали)
    {0x86, 0x3d}, {0x50, 0x00},
    {0x51, 0x90}, {0x52, 0x2c}, {0x53, 0x00}, {0x54, 0x00}, {0x55, 0x00},
    {0x57, 0x00}, {0x5a, 0xa0}, {0x5b, 0x78}, {0x5c, 0x00}, {0xd7, 0x03},
    {0xe1, 0x67}, {0xe5, 0x1f}, {0xe7, 0x10}, {0xe8, 0x50}, {0xe9, 0x02},
    {0xea, 0x40}, {0xee, 0xa0}, {0xef, 0x00},

    {0xff, 0x01}, // Переключаемся в Sensor Bank для фиксации масштабирования
    {0x12, 0x40}, // COM7: Включение блока Scaling
    {0x17, 0x11}, {0x18, 0x75}, {0x19, 0x01}, {0x1a, 0x97}, {0x32, 0x36},
    {0x03, 0x0f}, {0x37, 0x40}, {0x4f, 0xbb}, {0x50, 0x9c}, {0x5a, 0x57},
    {0x6d, 0x00}, {0x3d, 0x34},

    // ==================== КРИТИЧЕСКИЙ ЗАПУСК КОНВЕЙЕРА ====================
    {0xff, 0x00}, // Переходим обратно в DSP Bank
    {0xe0, 0x00}, // ВЫХОД ИЗ СБРОСА! Конвейер запускается, активируя HREF и делитель 0xD3
    {0x00, 0x00}  // Конец таблицы
};





#define VSYNC_PORT GPIOD
#define VSYNC_PIN  (1 << 11)
#define HREF_PORT  GPIOC
#define HREF_PIN   (1 << 9)
#define DCLK_PORT  GPIOC
#define DCLK_PIN   (1 << 10)
#define DATA_PORT  GPIOE

/** Захват кадра камерой OV2640 */
//////int ov2640_capture_snapshot(uint8_t *buffer, int width, int height)
//////{
//////    GPIO_set_HIGH(GPIOD, 15);   // Загорается синий светодиод для обозначения начала захвата кадра
//////
//////    int lines_processed = 0;    // Количество полученных строк
//////    uint8_t *p_buf = buffer;
//////
//////    while (!(VSYNC_PORT->IDR & VSYNC_PIN)); // Ожидание высокого уровня VSYNC
//////    while (VSYNC_PORT->IDR & VSYNC_PIN);    // Ожидание низкого уровня  VSYNC (старт кадра)
//////
//////    for (int y = 0; y < height; y++)
//////    {
//////        while (!(HREF_PORT->IDR & HREF_PIN)); // Ожидание начала строки (высокого уровня HREF)
//////
//////        for (int x = 0; x < width; x++)
//////        {
//////            // Чтение байта яркости (Y)
//////            while (!(DCLK_PORT->IDR & DCLK_PIN));
//////            while (DCLK_PORT->IDR & DCLK_PIN);
//////
//////            __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
//////            __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
//////            *p_buf++ = (uint8_t)((DATA_PORT->IDR >> 8) & 0xFF);
//////
//////            // Пропуск байта цветности (U/V)
//////            while (!(DCLK_PORT->IDR & DCLK_PIN));
//////            while (DCLK_PORT->IDR & DCLK_PIN);
//////        }
//////        lines_processed++;
//////        while (HREF_PORT->IDR & HREF_PIN); // Ждем конца строки
//////    }
//////    GPIO_set_LOW(GPIOD, 15);    // Гаснет синий светодиод для обозначения окончания захвата кадра
//////    return lines_processed;
//////}

/**
 * Захват полного кадра OV2640 в формате YUV422 (38400 байт для 160x120)
 * buffer должен иметь размер не менее width * height * 2
 */
int ov2640_capture_snapshot(uint8_t *buffer, int width, int height)
{
    // Включаем синий светодиод — начало захвата
    GPIO_set_HIGH(GPIOD, 15);

    int lines_processed = 0;
    uint8_t *p_buf = buffer;

    // Синхронизация по кадру
    while (VSYNC_PORT->IDR & VSYNC_PIN);    // дождаться окончания текущего кадра
    while (!(VSYNC_PORT->IDR & VSYNC_PIN)); // дождаться пока начнется новый кадр

    // Кадр принимается пока VSYNC высокий
    while (VSYNC_PORT->IDR & VSYNC_PIN)
    {
        // для каждой строки кадра:
        // дождаться начала строки (HREF станет 1)
        // пока HREF не станет 0 данные должны приниматься

        // Ожидание начала строки (HREF становится HIGH)
        while (!(HREF_PORT->IDR & HREF_PIN));
        while(HREF_PORT->IDR & HREF_PIN)
        {
            while (DCLK_PORT->IDR & DCLK_PIN);
            *p_buf++ = (uint8_t)(DATA_PORT->IDR >> 8);
            while (!(DCLK_PORT->IDR & DCLK_PIN));
        }
    }

    // Гасим синий светодиод — захват окончен
    GPIO_set_LOW(GPIOD, 15);

    return lines_processed;
}





/**********************************************************************************************************************/

// Повышение контрастности кадра
void image_enhance_contrast(const uint8_t *src, uint8_t *dst, int width, int height)
{
    uint8_t min_val = 255;
    uint8_t max_val = 0;
    int size = width * height;

    // 1. Находим реальный минимум и максимум в темном кадре
    for (int i = 0; i < size; i++)
    {
        if (src[i] < min_val) min_val = src[i];
        if (src[i] > max_val) max_val = src[i];
    }

    // Защита от деления на ноль, если кадр абсолютно однороден
    int delta = max_val - min_val;
    if (delta == 0) delta = 1;

    // 2. Растягиваем гистограмму яркости до 0..255
    for (int i = 0; i < size; i++)
    {
        int brightened = ((src[i] - min_val) * 255) / delta;
        dst[i] = (uint8_t)brightened;
    }
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

    /** ================= БЛОК ПРОВЕРКИ АППАРАТНОГО ID КАМЕРЫ ================= */
    uint8_t pid_val = 0;
    uint8_t ver_val = 0;
    I2C_Status_t I2C_status;

    // Переход на страницу сенсора (Bank 1) для чтения паспорта микросхемы
    I2C_status = I2C_Write_Reg(I2C2, 0x30, 0xFF, 0x01);
    delay_ms(5);

    if (I2C_status == I2C_OK) I2C_status = I2C_Read_Reg(I2C2, 0x30, 0x0A, &pid_val);
    if (I2C_status == I2C_OK) I2C_status = I2C_Read_Reg(I2C2, 0x30, 0x0B, &ver_val);

    // Если нет физического контакта I2C или ID не совпадает — БЫСТРОЕ мигание КРАСНОГО
    if (I2C_status != I2C_OK || pid_val != 0x26 || ver_val != 0x42)
    {
        while(1) {
            GPIO_toggle_Pin(GPIOD, 14);
            delay_ms(50);
        }
    }
    /** ============================================================ */

    // Запись регистров по I2C
    uint32_t reg_idx = 0;
    while (ov2640_config[reg_idx].reg != 0x00 || ov2640_config[reg_idx].val != 0x00)
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
    delay_ms(50);

    while(1)
    {

        int lines_read = ov2640_capture_snapshot((uint8_t*)camera_buffer, 160, 120);

        if (lines_read < 120)
        {
            GPIO_toggle_Pin(GPIOD, 14);
            GPIO_set_LOW(GPIOD, 15);
        }

//        uint8_t binary_buffer[CAM_FRAME_BYTES];
//        wolf_binarization_mcu(camera_buffer, binary_buffer, 160, 120, 9, 0.5f);

        delay_ms(1000);

    }
}
