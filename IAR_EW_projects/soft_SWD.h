#ifndef __SOFT_SWD_H__
#define __SOFT_SWD_H__

#include "gpio.h"

/***************************  Определение пинов GPIO в качестве пинов программного SWD  *******************************/
// PA1 => SWDIO
#define SOFT_SWD_DATA_PORT          GPIOA
#define SOFT_SWD_DATA_PIN           1

// PA3 => SWCLK
#define SOFT_SWD_CLK_PORT           GPIOA
#define SOFT_SWD_CLK_PIN            3

// PC3 => Target RESET
#define SOFT_SWD_TARGET_RESET_PORT  GPIOC
#define SOFT_SWD_TARGET_RESET_PIN   3
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


/******************************** Определение частоты программного SWD ************************************************/
#define SOFT_SWD_DELAY_TICKS(x)     for (uint32_t i = 0; i < x; i++) { __NOP(); }
#define SOFT_SWD_TICK_DURATION      500
/**********************************************************************************************************************/


/********************************* Базовые функции чтения/записи 1 бита ***********************************************/

// Пустой такт без изменения состояния линии SWDIO
#define SOFT_SWD_CLOCK_CYCLE()                                          \
{                                                                       \
    SOFT_SWD_CLK_LOW();                                                 \
    SOFT_SWD_DELAY_TICKS(SOFT_SWD_TICK_DURATION / 2);                   \
    SOFT_SWD_CLK_HIGH();                                                \
    SOFT_SWD_DELAY_TICKS(SOFT_SWD_TICK_DURATION / 2);                   \
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
    SOFT_SWD_CLK_HIGH();                                                \
    SOFT_SWD_DELAY_TICKS(SOFT_SWD_TICK_DURATION / 2);                   \
    bit = (SOFT_SWD_DATA_PORT->IDR & (1U << SOFT_SWD_DATA_PIN)) ? 1 : 0;\
    SOFT_SWD_DELAY_TICKS(SOFT_SWD_TICK_DURATION / 4);                   \
    SOFT_SWD_CLK_LOW();                                                 \
}

#define SOFT_SWD_TRN_TO_INPUT()                                         \
{                                                                       \
    SOFT_SWD_DATA_SET_INPUT();                                          \
    SOFT_SWD_CLOCK_CYCLE();                                             \
}

#define SOFT_SWD_TRN_TO_OUTPUT()                                        \
{                                                                       \
     SOFT_SWD_CLOCK_CYCLE();                                            \
     SOFT_SWD_DATA_SET_OUTPUT();                                        \
}
/**********************************************************************************************************************/


/***************************** Управляющие последовательности бит *****************************************************/
#define SOFT_SWD_JTAG_TO_SWD    (0xE79E)    // Запрос на переключение порта отладки таргета с JTAG на SWD (у некоторых МК по умолчанию подключен JTAG)

// Сброс линии SWD (нужен перед началом работы для синхронизации программатора и таргета)
#define SOFT_SWD_LINE_RESET()           \
{                                       \
    SOFT_SWD_DATA_HIGH();               \
    for (uint8_t i = 0; i < 50; i++)    \
    {                                   \
        SOFT_SWD_CLOCK_CYCLE();         \
    }                                   \
}

#define SOFT_SWD_SWITCH_JTAG_TO_SWD()           \
{                                               \
    uint16_t switch_seq = SOFT_SWD_JTAG_TO_SWD; \
    for (int i = 0; i < 16; i++)                \
    {                                           \
        SoftSWD_WriteBit(switch_seq & 0x01);    \
        switch_seq >>= 1;                       \
    }                                           \
}

#define SOFT_SWD_RESET()            \
{                                   \
    SOFT_SWD_RESET_TARGET_LOW();    \
    SOFT_SWD_RESET_TARGET_HIGH();   \
    SOFT_SWD_DELAY_TICKS(50000000);   \
}

/**********************************************************************************************************************/













#define SOFT_SWD_CYCLE_DURATION         (10)                             // Продолжительность 1 такта soft_SWD в микросекундах
#define SOFT_SWD_LINE_RESET_DURATION    (50 * SOFT_SWD_CYCLE_DURATION)  // Продолжительность сброса линии SWD (минимум 50 тактов линия данных в состоянии 1)



#define SOFT_SWD_REQ_START    (1 << 0)  // Всегда 1
#define SOFT_SWD_REQ_AP       (1 << 1)  // 1 для AP, 0 для DP
#define SOFT_SWD_REQ_DP       (0 << 1)
#define SOFT_SWD_REQ_READ     (1 << 2)  // 1 для Read, 0 для Write
#define SOFT_SWD_REQ_WRITE    (0 << 2)
#define SOFT_SWD_REQ_STOP     (0 << 6)  // Всегда 0
#define SOFT_SWD_REQ_PARK     (1 << 7)  // Всегда 1

// Формирует 8-битный запрос. addr передается как 0x00, 0x04, 0x08 или 0x0C
#define SWD_REQUEST(ap_n_dp, r_n_w, addr) ( \
    SOFT_SWD_REQ_START | \
    (ap_n_dp) | \
    (r_n_w) | \
    (((addr) & 0x0C) << 1) | \
    SOFT_SWD_REQ_STOP | \
    SOFT_SWD_REQ_PARK \
)

void SoftSWD_Init();
void SoftSWD_Line_Reset();
void SoftSWD_JTAGtoSWD();
void SoftSWD_Target_Reset();
void SoftSWD_WriteBit(uint8_t bit);
uint8_t SoftSWD_ReadBit();
void SoftSWD_TurnAround();
uint32_t SoftSWD_ReadIDCODE(void);

#endif /* __SOFT_SWD_H__ */