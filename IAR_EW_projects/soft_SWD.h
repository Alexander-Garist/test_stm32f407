/***********************************************************************************************************************
*   Программный SWD
*       Интерфейс передачи данных (работа только с регистрами SWD)
*       Не содержит функцию записи во Flash память, т.к. может записывать только в RAM
***********************************************************************************************************************/

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
#define SOFT_SWD_DATA_HIGH()    (SOFT_SWD_DATA_PORT->BSRR = (0x1U << SOFT_SWD_DATA_PIN))
#define SOFT_SWD_DATA_LOW()     (SOFT_SWD_DATA_PORT->BSRR = (0x1U << (SOFT_SWD_DATA_PIN + 16)))

#define SOFT_SWD_CLK_HIGH()     (SOFT_SWD_CLK_PORT->BSRR = (0x1U << SOFT_SWD_CLK_PIN))
#define SOFT_SWD_CLK_LOW()      (SOFT_SWD_CLK_PORT->BSRR = (0x1U << (SOFT_SWD_CLK_PIN + 16)))

#define SOFT_SWD_RESET_TARGET_HIGH()    (SOFT_SWD_TARGET_RESET_PORT->BSRR = (0x1U << SOFT_SWD_TARGET_RESET_PIN))
#define SOFT_SWD_RESET_TARGET_LOW()     (SOFT_SWD_TARGET_RESET_PORT->BSRR = (0x1U << (SOFT_SWD_TARGET_RESET_PIN + 16)))
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


/********************************************************************************************************* Запрос SWD */

// Вариант через структуру
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

// Вариант через битовые маски

/** Основа любого запроса к таргету: биты start=1, stop=0, park=1 никогда не меняются */
#define REQUEST_BASE                0x81U    // 1000 0001

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
#define PARITY_MASK(parity)         (parity << PARITY_BIT_POS)


/*********************************************************************************************************** Регистры */

// DP_AP
#define DP              0x0U    // Физический интерфейс SWD
#define AP              0x1U    // Интерфейс доступа к ресурсам целевого МК

// RnW
#define WRITE           0x0U    // Запись в регистр таргета
#define READ            0x1U    // Чтение регистра таргета

// REGISTER DP ADDRESS
#define DP_ABORT        0x00U   // Регистр принудительного сброса
#define DP_IDCODE       0x00U   // IDCODE таргета
#define DP_CTRL_STAT    0x04U   // Регистр управления и статуса
#define DP_WCR          0x04U   // Выбор режима работы SW-DP
#define DP_SELECT       0x08U   // Регистр выбора текущего порта AP
#define DP_RESEND       0x08U   // Регистр повторной отправки
#define DP_RDBUFF       0x0CU   // 32-битный буфер чтения

// REGISTER AP ADDRESS
#define AP_CSW          0x00U   // Хранит состояние и управление MEM-AP
#define AP_TAR          0x04U   // Хранит адрес для следующего обращения к системе памяти или отладке
#define AP_DRW          0x0CU   // Регистр запуска чтения/записи по адресу в AP_TAR
#define AP_BD0          0x10U   // Банковые регистры обеспечивают прямой доступ к блоку памяти из 4 32-битных слов начиная с адреса в AP_TAR
#define AP_BD1          0x14U
#define AP_BD2          0x18U
#define AP_BD3          0x1CU
#define AP_CFG          0xF4U   // Информация о конфигурации MEM-AP
#define AP_BASE         0xF8U   // Указатель на подключенную систему (отладка или начало ROM памяти)
#define AP_IDR          0xFCU   // Идентификатор MEM-AP


/***************************************************************************************** Маски для регистров DP и AP*/

/** DP_ABORT */

#define DP_ABORT_ORUNERRCLR         (0x1U << 4)        // Сброс ошибки переполнения DP_CTRL_STAT_STICKYORUN
#define DP_ABORT_WDERRCLR           (0x1U << 3)        // Сброс ошибки записи DP_CTRL_STAT_WDATAERR
#define DP_ABORT_STKERRCLR          (0x1U << 2)        // Сброс ошибки транзакции DP_CTRL_STAT_STICKYERR
#define DP_ABORT_STKCMPCLR          (0x1U << 1)        // Сброс ошибки сравнения DP_CTRL_STAT_STICKYCMP
#define DP_ABORT_DAPABORT           (0x1U << 0)        // Прервать выполнение операции

/** DP_CTRL_STAT */

#define DP_CTRL_STAT_CSYSPWRUPACK   (0x1U << 31)    // Подтверждение включения питания системы
#define DP_CTRL_STAT_CSYSPWRUPREQ   (0x1U << 30)    // Включение питания системы
#define DP_CTRL_STAT_CDBGPWRUPACK   (0x1U << 29)    // Подтверждение включения модуля отладки
#define DP_CTRL_STAT_CDBGPWRUPREQ   (0x1U << 28)    // Включение питания модуля отладки
#define DP_CTRL_STAT_CDBGRSTACK     (0x1U << 27)    // Подтверждение сброса модуля отладки
#define DP_CTRL_STAT_CDBGRSTREQ     (0x1U << 26)    // Сброс модуля отладки
#define DP_CTRL_STAT_WDATAERR       (0x1U << 7)     // 1 - произошла ошибка записи
#define DP_CTRL_STAT_STICKYERR      (0x1U << 5)     // 1 - произошла системная ошибка
#define DP_CTRL_STAT_STICKYCMP      (0x1U << 4)     // 1 - произошла ошибка сравнения
#define DP_CTRL_STAT_STICKYORUN     (0x1U << 1)     // 1 - произошла ошибка переполнения (если ORUNDETECT == 1)
#define DP_CTRL_STAT_ORUNDETECT     (0x1U << 0)     // 0 - запретить (1 - разрешить) обнаружение переполнений

/** DP_SELECT */

#define DP_SELECT_APSEL(x)          (x << 24)       // Выбор текущего AP
#define DP_SELECT_APBANKSEL(x)      (x << 4)        // Выбор активного регистрового блока выбранного AP
#define DP_SELECT_CTRLSEL           (0x1U << 0)     // Выбор регистра DP по адресу 0x04 (после сброса значение 0 => DP_CTRL_STAT)
                                                    // Если установить 1, то будет выбран регистр DP_WCR

#define MEM_AP_APSEL        0x0     // Основной MEM-AP для доступа к памяти
#define MEM_AP_APBANKSEL    0x0     // Для доступа к регистрам CSW, TAR, RWD нужен банк регистров 0

/** DP_WCR */

#define DP_WCR_TURNROUND            (0x1 << 8)      // Определяет длительность смены направления линии данных (0x0 - 1 такт, 0x1 - 2 такта, 0x2 - 3 такта, 0x3 - 4 такта)
#define DP_WCR_WIREMODE             (0x1 << 6)      // Определяет режим работы синхронный (1)/асинхронный (0)
#define DP_WCR_PRESCALER            (0x1 << 0)      // Предделитель частоты дискретизации

/** AP_CSW */

#define AP_CSW_DBGSWENABLE      (0x1 << 31)     // Вкл/выкл отладки ПО
#define AP_CSW_PROT             (0x1 << 24)     // Управление доступом к шине
#define AP_CSW_SPIDEN           (0x1 << 23)     // RO Защищенная привилегированная отладка вкл/выкл
#define AP_CSW_MODE             (0x1 << 8)      // Должен быть всегда 0x0
#define AP_CSW_TRINPROG         (0x1 << 7)      // RO 1 - передача в процессе
#define AP_CSW_DEVICEEN         (0x1 << 6)      // RO 1 - MEM-AP доступен
#define AP_CSW_ADDRINC          (0x1 << 4)      // 1 - автоинкремент адреса в AP_TAR при каждой операции чтения/записи в AP_DRW
#define AP_CSW_SIZE             (0x1 << 0)      // RO b010 - поддерживается только 32-битный обмен данными


/*********************************************************************************************** Константные значения */

// Ответы ACK
#define SWD_ACK_OK      (0x1U)
#define SWD_ACK_WAIT    (0x2U)
#define SWD_ACK_FAIL    (0x4U)

// Стандартное значение регистра CSW, выбран MEM-AP, настроен доступ к отладке и памяти таргета
#define MEM_AP_DEFAULT  (0x23000002)

#define SOFT_SWD_TICK_DURATION  (4)         // Продолжительность 1 такта SWD в тактах процессора
#define SOFT_SWD_JTAG_TO_SWD    (0xE79E)    // Запрос на переключение порта отладки таргета с JTAG на SWD


/************************************************************************************************** Прототипы функций */

/** Инициализация программного SWD, включение тактирования портов GPIO, на которых реализованы SWDIO, SWCLK, SWRST;
*       конфигурация пинов SoftSWD */
void SoftSWD_Init();

/** Синхронизировать между собой мастер и таргет */
void SoftSWD_Sync_Target();

/** Аппаратный сброс таргета */
void SoftSWD_Reset_Target();

/** Прочитать DP_IDCODE */
uint32_t SoftSWD_Get_IDCODE();


/** Чтение значения регистра AP или DP */
uint32_t SoftSWD_ReadRegister(uint8_t DP_AP, uint8_t Addr);

/** Запись значения в регистр AP или DP */
void SoftSWD_WriteRegister(uint8_t DP_AP, uint8_t Addr, uint32_t register_value);

/** Функция-обертка статической функции настройки DP регистров для работы с MEM-AP */
void SoftSWD_set_MEM_AP();



/** Чтение из памяти таргета */
void SoftSWD_ReadMemory(uint32_t address, uint8_t* buffer, uint32_t size);

/** Запись в RAM память таргета */
void SoftSWD_WriteMemory_RAM(uint32_t address, uint8_t* buffer, uint32_t size);


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