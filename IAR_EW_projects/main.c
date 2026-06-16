#include <stdio.h>
#include "stm32f4xx.h"
#include "systick.h"
#include "gpio.h"
#include "i2c.h"
//#include "dcmi.h"
#include "image_processing.h"

#define CAM_WIDTH        160
#define CAM_HEIGHT       120
#define CAM_FRAME_BYTES  (CAM_WIDTH * CAM_HEIGHT)
uint8_t camera_buffer[CAM_FRAME_BYTES];

typedef struct {
    uint8_t reg;
    uint8_t val;
} ov2640_reg_t;


const ov2640_reg_t ov2640_config[] = {
    // ==================== СБРОС И СТАБИЛИЗАЦИЯ ====================
    {0xff, 0x01}, // Sensor Bank (Bank 1)
    {0x12, 0x80}, // Software Reset (Заводской сброс)
    {0x00, 0xff}, // Пауза на стабилизацию кристалла

    // ==================== НАСТРОЙКА МАТРИЦЫ И СИНХРОНИЗАЦИИ (Bank 1) ====================
    {0xff, 0x01}, // Строго Sensor Bank
    {0x11, 0x01}, // CLKRC: Делитель частоты матрицы = 2 (Стабильный аналоговый режим)
    {0x15, 0x02}, // COM10: Положительная полярность VSYNC
    {0x04, 0x08}, // REG04: ИСПРАВЛЕНО! Включаем HREF строго в Sensor Bank

    // ==================== НАСТРОЙКА ЦИФРОВОГО ПОТОКА (Bank 0) ====================
    {0xff, 0x00}, // Переходим в DSP Bank (Bank 0)
    {0xe0, 0x04}, // Входим в режим конфигурирования конвейера DVP

    {0xda, 0x00}, // IMAGE_MODE: ИСПРАВЛЕНО! Отключаем JPEG, включаем построчный YUV
    {0xc2, 0x0c}, // CTRL0: ИСПРАВЛЕНО! Активируем процессор обработки YUV422
    {0xd3, 0x07}, // R_DVP_SP: ТЕПЕРЬ ОН ЗАРАБОТАЕТ. Жестко делим DCLK на 8.
                  // Частота PCLK на пине упадет с 8.2 МГц до идеальных ~1 МГц.

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


int ov2640_capture_snapshot(uint8_t *buffer, int width, int height) {
    int lines_processed = 0;
    uint8_t *p_buf = buffer;

    // 1. Ловим спад кадра (переход VSYNC из 1 в 0) при полярности 0x02
    while (!(VSYNC_PORT->IDR & VSYNC_PIN)); // Ждем высокого уровня
    while (VSYNC_PORT->IDR & VSYNC_PIN);    // Ждем спада в 0 (Старт кадра)

    // 2. Построчный сбор
    for (int y = 0; y < height; y++) {
        while (!(HREF_PORT->IDR & HREF_PIN)); // Ждем начала строки

        for (int x = 0; x < width; x++) {
            // Чтение байта яркости (Y)
            while (DCLK_PORT->IDR & DCLK_PIN);
            while (!(DCLK_PORT->IDR & DCLK_PIN));
            *p_buf++ = (uint8_t)((GPIOE->IDR >> 8) & 0xFF);

            // Пропуск байта цветности (U/V)
            while (DCLK_PORT->IDR & DCLK_PIN);
            while (!(DCLK_PORT->IDR & DCLK_PIN));
        }
        lines_processed++;
        while (HREF_PORT->IDR & HREF_PIN); // Ждем конца строки
    }
    return lines_processed;
}

// Правильная конфигурация пинов ввода для работы с КМОП-камерой
void GPIO_Camera_Input_Enable(GPIO_TypeDef* GPIO_port, uint8_t GPIO_pin)
{
    GPIO_RCC_Enable(GPIO_port);
    GPIO_port->MODER &= ~(MODER_ANALOG << (GPIO_pin * 2));
    GPIO_port->PUPDR &= ~(PUPDR_RESERVED << (GPIO_pin * 2));
    //GPIO_port->OSPEEDR |= (OSPEEDR_VERY_HIGH << (GPIO_pin * 2));
}
uint32_t start_time, end_time, delta_time;
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

    // ЗАПИСЬ ПОЛНОЙ ТАБЛИЦЫ НАСТРОЕК ГЕОМЕТРИИ И ISP
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
            // Если прочитано меньше 120 строк
            GPIO_toggle_Pin(GPIOD, 14);
            GPIO_set_LOW(GPIOD, 15);
        }
        else
        {
            // Прочитаны все 120 строк
            GPIO_set_LOW(GPIOD, 14);
            GPIO_toggle_Pin(GPIOD, 15);
        }
    }
}
