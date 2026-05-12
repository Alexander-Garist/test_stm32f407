#ifndef __SOFT_SWD_H__
#define __SOFT_SWD_H__

#include "gpio.h"

/********************************************************** Определение пинов GPIO в качестве пинов программного SWD  */
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


/********************************************************************** Управление состоянием пинов программного SWD  */
#define SOFT_SWD_DATA_HIGH()    (SOFT_SWD_DATA_PORT->BSRR = (0x1 << SOFT_SWD_DATA_PIN))
#define SOFT_SWD_DATA_LOW()     (SOFT_SWD_DATA_PORT->BSRR = (0x1 << (SOFT_SWD_DATA_PIN + 16)))

#define SOFT_SWD_CLK_HIGH()     (SOFT_SWD_CLK_PORT->BSRR = (0x1 << SOFT_SWD_CLK_PIN))
#define SOFT_SWD_CLK_LOW()      (SOFT_SWD_CLK_PORT->BSRR = (0x1 << (SOFT_SWD_CLK_PIN + 16)))

#define SOFT_SWD_RESET_TARGET_HIGH()    (SOFT_SWD_TARGET_RESET_PORT->BSRR = (0x1 << SOFT_SWD_TARGET_RESET_PIN))
#define SOFT_SWD_RESET_TARGET_LOW()     (SOFT_SWD_TARGET_RESET_PORT->BSRR = (0x1 << (SOFT_SWD_TARGET_RESET_PIN + 16)))
/**********************************************************************************************************************/


/******************************************************************************* Настройка пина SWDIO на Input/Output */
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


/*********************************************************************************************************** Регистры */


#define SOFT_SWD_TICK_DURATION      (4)         // Продолжительность 1 такта SWD в тактах процессора
#define SOFT_SWD_JTAG_TO_SWD        (0xE79E)    // Запрос на переключение порта отладки таргета с JTAG на SWD

typedef struct
{
    uint8_t start   : 1;    // Стартовый бит всегда 1
    uint8_t DP_AP   : 1;    // Выбор: 0 - DP, 1 - AP
    uint8_t RnW     : 1;    // Выбор: 0 - write, 1 - read
    uint8_t Addr_2  : 1;    // Адрес регистра DP или AP
    uint8_t Addr_3  : 1;    // Адрес регистра DP или AP
    uint8_t parity  : 1;    // Бит четности (количество 1 среди DP_AP, RnW, Addr)
    uint8_t stop    : 1;    // Стоповый бит всегда 0
    uint8_t park    : 1;    // Всегда 1
}
SoftSWD_Request;

/** Основа любого запроса к таргету: биты start=1, stop=0, park=1 никогда не меняются */
#define REQUEST_BASE                0x81    // 1000 0001

/** Изменяемые биты запроса */
// DP_AP
#define DP_AP_BIT_POS               6
#define DP_AP_MASK(bit)             (bit << DP_AP_BIT_POS)

// RnW
#define RnW_BIT_POS                 5
#define RnW_MASK(bit)               (bit << RnW_BIT_POS)

// Address
#define ADDR2_BIT_POS               4
#define ADDR2_MASK(reg_address)     (((reg_address >> 2) & 0x1) << ADDR2_BIT_POS)
#define ADDR3_BIT_POS               3
#define ADDR3_MASK(reg_address)     (((reg_address >> 3) & 0x1) << ADDR3_BIT_POS)

// Parity
#define PARITY_BIT_POS              2
#define PARITY_MASK(parity)         (parity << PARITY_BIT_POS)   // ! Аргумент надо посчитать перед подстановкой в запрос!

/** здесь будут определены маски для формирования запросов
* запрос 8 бит: START  DP_AP  RnW  Address[2]  Address[3] Parity  Stop  Park
* маска будет для бит DP_AP, RnW и Address
* Пример: запись в WCR => RnW = 0, WCR => DP = 0 Address = 0x4 (0100)
* итог: #define DP_WRITE_WCR (0 << DP_AP_BIT_POS) | (0 << RnW_BIT_POS) | (1 << ADDR2_BIT_POS) | (0 << ADDR3_BIT_POS)
*/
// DP_AP
#define DP              0x0
#define AP              0x1

// RnW
#define WRITE           0x0
#define READ            0x1

// REGISTER DP ADDRESS
#define ADDR_ABORT      0x00
#define ADDR_IDCODE     0x00
#define ADDR_CTRL_STAT  0x04
#define ADDR_WCR        0x04
#define ADDR_SELECT     0x08
#define ADDR_RESEND     0x08
#define ADDR_RDBUFF     0x0C

// REGISTER AP ADDRESS
#define ADDR_CSW        0x00
#define ADDR_TAR        0x04
#define ADDR_DRW        0x0C
#define ADDR_BD0        0x10
#define ADDR_BD1        0x14
#define ADDR_BD2        0x18
#define ADDR_BD3        0x1C
#define ADDR_CFG        0xF4
#define ADDR_BASE       0xF8
#define ADDR_IDR        0xFC

/************************************************************************************* Маски для записи в регистры DP */

/** Запись в DP_ABORT */
#define ORUNERRCLR_Pos  4   // Сброс ошибки переполнения
#define ORUNERRCLR      (0x1 << ORUNERRCLR_Pos)

#define WDATAERR_Pos    3   // Сброс ошибки записи
#define WDATAERR        (0x1 << WDATAERR_Pos)

#define STICKYERR_Pos   2   // Сброс ошибки транзакции
#define STICKYERR       (0x1 << STICKYERR_Pos)

#define STICKYCMP_Pos   1   // Сброс ошибки сравнения
#define STICKYCMP       (0x1 << STICKYCMP_Pos)

#define DAPABORT_Pos    0   // Прервать выполнение операции
#define DAPABORT        (0x1 << DAPABORT_Pos)

/** Запись в DP_CTRL_STAT */
#define CSYSPWRUPREQ_Pos    30  // Включение питания системы
#define CSYSPWRUPREQ        (0x1 << CSYSPWRUPREQ_Pos)

#define CDBGPWRUPREQ_Pos    28  // Включение питания модуля отладки
#define CDBGPWRUPREQ        (0x1 << CDBGPWRUPREQ_Pos)

#define CDBGRSTREQ_Pos      26  // Сброс модуля отладки
#define CDBGRSTREQ          (0x1 << CDBGRSTREQ_Pos)


/** Запись в DP_SELECT */
#define APSEL_Pos           24  // Выбор текущего AP
#define APSEL(x)            (x << APSEL_Pos)

#define APBANKSEL_Pos       4   // Выбор активного регистрового блока выбранного AP
#define APBANKSEL(x)        (x << APBANKSEL_Pos)

#define CTRLSEL_Pos         0   // Выбор регистра DP по адресу 0x04 (после сброса значение 0 => DP_CTRL_STAT)
#define CTRLSEL             (0x1 << CTRLSEL_Pos)    // Если установить 1, то будет выбран регистр DP_WCR

/**********************************************************************************************************************/


/************************************************************************************* Маски для записи в регистры AP */






/**********************************************************************************************************************/

#define SWD_ACK_OK      0x1
#define SWD_ACK_WAIT    0x2
#define SWD_ACK_FAIL    0x4









/************************************************************************************************** Прототипы функций */

/** Инициализация программного SWD, включение тактирования портов GPIO, на которых реализованы SWDIO, SWCLK, SWRST;
*       конфигурация пинов SoftSWD */
void SoftSWD_Init();

/** Синхронизировать между собой мастер и таргет */
void SoftSWD_Sync_Target();

/** Аппаратный сброс таргета */
void SoftSWD_Reset_Target();

/** Чтение из памяти таргета:
*       начиная с адреса address
*       записывая в буфер buffer
*       количество байт size
 */
void SoftSWD_ReadMemory(uint32_t address, uint8_t* buffer, uint32_t size);

/** Прочитать DP_IDCODE */
uint32_t SoftSWD_Get_IDCODE();

/** Запись в RAM память таргета:
*       начиная с адреса address
*       записывая данные из buffer
*       количество байт size
 */
void SoftSWD_Write_RAM(uint32_t address, uint8_t* buffer, uint32_t size);

/** Запись в Flash память таргета:
*       начиная с адреса address
*       записывая данные из buffer
*       количество байт size
 */
void SoftSWD_Write_Flash(uint32_t address, uint8_t* buffer, uint32_t size);
void SoftSWD_ClearErrors(void);


/** Запись в flash таргета */
void SoftSWD_Write_FLASH(uint32_t address, uint8_t* buffer, uint32_t size);

void SoftSWD_Erase_Flash(uint32_t address, uint32_t size);
void SoftSWD_Halt_Target();
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