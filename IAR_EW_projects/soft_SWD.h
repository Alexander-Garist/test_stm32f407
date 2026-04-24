#ifndef __SOFT_SWD_H__
#define __SOFT_SWD_H__

#include "gpio.h"

/***************************  Определение пинов GPIO в качестве пинов программного SWD  *******************************/
// PC3 => Target RESET
#define SOFT_SWD_TARGET_RESET_PORT  GPIOC
#define SOFT_SWD_TARGET_RESET_PIN   3

// PA1 => SWDIO
#define SOFT_SWD_DATA_PORT          GPIOA
#define SOFT_SWD_DATA_PIN           1

// PA3 => SWCLK
#define SOFT_SWD_CLK_PORT           GPIOA
#define SOFT_SWD_CLK_PIN            3
/**********************************************************************************************************************/


/*************************  Управление состоянием пинов программного SWD  *********************************************/
#define SOFT_SWD_DATA_HIGH()    (SOFT_SWD_DATA_PORT->BSRR = (0x1 << SOFT_SWD_DATA_PIN))
#define SOFT_SWD_DATA_LOW()     (SOFT_SWD_DATA_PORT->BSRR = (0x1 << (SOFT_SWD_DATA_PIN + 16)))

#define SOFT_SWD_CLK_HIGH()     (SOFT_SWD_CLK_PORT->BSRR = (0x1 << SOFT_SWD_CLK_PIN))
#define SOFT_SWD_CLK_LOW()      (SOFT_SWD_CLK_PORT->BSRR = (0x1 << (SOFT_SWD_CLK_PIN + 16)))

#define SOFT_SWD_RESET_TARGET_HIGH()    (SOFT_SWD_TARGET_RESET_PORT->BSRR = (0x1 << SOFT_SWD_TARGET_RESET_PIN))
#define SOFT_SWD_RESET_TARGET_LOW()     (SOFT_SWD_TARGET_RESET_PORT->BSRR = (0x1 << (SOFT_SWD_TARGET_RESET_PIN + 16)))
/**********************************************************************************************************************/


/***********************  Настройка пина SWDIO на Input/Output   ******************************************************/
typedef enum
{
    Master_Input = 0,
    Master_Output = 1
}
SoftSWD_Direction;

#define SOFT_SWD_DATA_SET_INPUT()                                               \
{                                                                               \
    SOFT_SWD_DATA_PORT->MODER &= ~(MODER_ANALOG << (SOFT_SWD_DATA_PIN * 2));    \
}

#define SOFT_SWD_DATA_SET_OUTPUT()                                              \
{                                                                               \
    SOFT_SWD_DATA_PORT->MODER &= ~(MODER_ANALOG << (SOFT_SWD_DATA_PIN * 2));    \
    SOFT_SWD_DATA_PORT->MODER |=  (MODER_OUTPUT << (SOFT_SWD_DATA_PIN * 2));    \
}
/**********************************************************************************************************************/

#define SOFT_SWD_TICK_DURATION      (4)        // Продолжительность 1 такта SWD в тактах процессора
#define SOFT_SWD_JTAG_TO_SWD        (0xE79E)    // Запрос на переключение порта отладки таргета с JTAG на SWD

typedef struct
{
    uint8_t start   : 1;    // Стартовый бит всегда 1
    uint8_t DP_AP   : 1;    // Выбор: 0 - DP, 1 - AP
    uint8_t RnW     : 1;    // Выбор: 0 - write, 1 - read
    uint8_t Addr_3  : 1;    // Адрес регистра DP или AP
    uint8_t Addr_2  : 1;    // Адрес регистра DP или AP
    uint8_t parity  : 1;    // Бит четности (количество 1 среди DP_AP, RnW, Addr)
    uint8_t stop    : 1;    // Стоповый бит всегда 0
    uint8_t park    : 1;    // Всегда 1
}
SoftSWD_Request;

/********************************* Прототипы функций ******************************************************************/

/** Инициализация программного SWD, включение тактирования портов GPIO, на которых реализованы SWDIO, SWCLK, SWRST;
*       конфигурация пинов SoftSWD */
void SoftSWD_Init();

/** Программный сброс линии SWD (50 тактов, во время которых на линии данных логическая 1). Нужен для синхронизации
*       ведущего и ведомого устройств */
void SoftSWD_Line_Reset();

/** Отправка управляющей последовательности для переключения отладчика в режим SWD */
void SoftSWD_JTAGtoSWD();

/** Чтение значения регистра AP или DP */
uint32_t SoftSWD_ReadRegister(uint8_t DP_AP, uint8_t Addr);

// Чтение из памяти таргета по адресу
void SoftSWD_ReadMemory(uint32_t address, uint8_t* buffer, uint32_t size);

#endif /* __SOFT_SWD_H__ */



/**************** Работает, но не используется *********************/
/*
#define SOFT_SWD_REQ_START    (1 << 0)  // Всегда 1
#define SOFT_SWD_REQ_AP       (1 << 1)  // 1 для AP, 0 для DP
#define SOFT_SWD_REQ_DP       (0 << 1)
#define SOFT_SWD_REQ_READ     (1 << 2)  // 1 для Read, 0 для Write
#define SOFT_SWD_REQ_WRITE    (0 << 2)
#define SOFT_SWD_REQ_STOP     (0 << 6)  // Всегда 0
#define SOFT_SWD_REQ_PARK     (1 << 7)  // Всегда 1

// Формирует 8-битный запрос. addr передается как 0x00, 0x04, 0x08 или 0x0C
#define SWD_REQUEST(ap_n_dp, r_n_w, addr) (         \
    SOFT_SWD_REQ_START |                            \
    (ap_n_dp) |                                     \
    (r_n_w) |                                       \
    (((addr) & 0x0C) << 1) |                        \
    SOFT_SWD_REQ_STOP |                             \
    SOFT_SWD_REQ_PARK                               \
)

// Задержка в тактах процессора
#define SOFT_SWD_DELAY_TICKS(x)     for (uint32_t i = 0; i < x; i++) { __NOP(); }

// Пустой такт без изменения состояния линии SWDIO
#define SOFT_SWD_CLOCK_CYCLE()                                          \
{                                                                       \
    delay_ticks(SOFT_SWD_TICK_DURATION);                                \
    SOFT_SWD_CLK_HIGH();                                                \
    delay_ticks(SOFT_SWD_TICK_DURATION);                                \
    SOFT_SWD_CLK_LOW();                                                 \
}

// Передача 1 бита данных
#define SOFT_SWD_WRITE_BIT(bit)                                         \
{                                                                       \
    if (bit) SOFT_SWD_DATA_HIGH();                                      \
    else SOFT_SWD_DATA_LOW();                                           \
    SOFT_SWD_DELAY_TICKS(SOFT_SWD_TICK_DURATION / 2);                   \
    SOFT_SWD_CLK_HIGH();                                                \
    SOFT_SWD_DELAY_TICKS(SOFT_SWD_TICK_DURATION / 2);                   \
    SOFT_SWD_CLK_LOW();                                                 \
}

// Прием 1 бита данных
#define SOFT_SWD_READ_BIT(bit)                                          \
{                                                                       \
    SOFT_SWD_DELAY_TICKS(SOFT_SWD_TICK_DURATION / 2);                   \
    bit = (SOFT_SWD_DATA_PORT->IDR & (1U << SOFT_SWD_DATA_PIN)) ? 1 : 0;\
    SOFT_SWD_CLK_HIGH();                                                \
    SOFT_SWD_DELAY_TICKS(SOFT_SWD_TICK_DURATION / 2);                   \
    SOFT_SWD_CLK_LOW();                                                 \
}

// переключить на прием
#define SOFT_SWD_TRN_TO_INPUT()                                         \
{                                                                       \
    SOFT_SWD_DATA_SET_INPUT();                                          \
    SOFT_SWD_CLOCK_CYCLE();                                             \
}

// переключить на передачу
#define SOFT_SWD_TRN_TO_OUTPUT()                                        \
{                                                                       \
     SOFT_SWD_CLOCK_CYCLE();                                            \
     SOFT_SWD_DATA_SET_OUTPUT();                                        \
}

// Сброс линии SWD (нужен перед началом работы для синхронизации программатора и таргета)
#define SOFT_SWD_LINE_RESET()                                           \
{                                                                       \
    SOFT_SWD_DATA_HIGH();                                               \
    for (uint8_t i = 0; i < 50; i++)                                    \
    {                                                                   \
        SOFT_SWD_CLOCK_CYCLE();                                         \
    }                                                                   \
}

// переключить линию с JTAG на SWD
#define SOFT_SWD_SWITCH_JTAG_TO_SWD()                                   \
{                                                                       \
    uint16_t switch_seq = SOFT_SWD_JTAG_TO_SWD;                         \
    for (int i = 0; i < 16; i++)                                        \
    {                                                                   \
        SoftSWD_WriteBit(switch_seq & 0x01);                            \
        switch_seq >>= 1;                                               \
    }                                                                   \
}

*/