#ifndef __SOFT_SWD_H__
#define __SOFT_SWD_H__

#include "gpio.h"

// В файле core_cm4.h нет определения DWT, поэтому пока впихнул его прямо сюда
typedef struct {
  __IO uint32_t CTRL;      /*!< Offset: 0x000 (R/W)  DWT Control Register */
  __IO uint32_t CYCCNT;    /*!< Offset: 0x004 (R/W)  DWT Cycle Count Register */
  __IO uint32_t CPICNT;    /*!< Offset: 0x008 (R/W)  DWT CPI Count Register */
  __IO uint32_t EXCCNT;    /*!< Offset: 0x00C (R/W)  DWT Exception Overlap Count Register */
  __IO uint32_t SLEEPCNT;  /*!< Offset: 0x010 (R/W)  DWT Sleep Count Register */
  __IO uint32_t LSUCNT;    /*!< Offset: 0x014 (R/W)  DWT LSU Count Register */
  __IO uint32_t FOLDCNT;   /*!< Offset: 0x018 (R/W)  DWT Folded-instruction Count Register */
  __I  uint32_t PCSR;      /*!< Offset: 0x01C (R/ )  DWT Program Counter Sample Register */
} DWT_Type;

#define DWT_BASE    (0xE0001000UL)
#define DWT         ((DWT_Type *) DWT_BASE)

// Маски, если их тоже нет в вашем CMSIS
//#define CoreDebug_DEMCR_TRCENA_Msk (1UL << 24)
#define DWT_CTRL_CYCCNTENA_Msk     (1UL << 0)













/** Тактовая частота системного таймера 16 МГц, для программного SWD это слишком быстро,
*   пусть частота SWD будет 0.1 МГц, тогда 1 такт длится 10 мкс (160 тактов системного таймера)
*   Тогда для управления тактовым сигналом нужно отсчитывать не мс, а именно такты SysTick.
*   Нужна функция которая проверяет прошло ли 160 тактов после последнего переключения пина CLK
*/
#define SOFT_SWD_CLK_CYCLE  160

// PC1 => SWCLK
#define SOFT_SWD_CLK_PORT           GPIOC
#define SOFT_SWD_CLK_PIN            1

// PC0 => SWDIO
#define SOFT_SWD_DATA_PORT          GPIOC
#define SOFT_SWD_DATA_PIN           0

// PC3 => Target RESET
#define SOFT_SWD_TARGET_RESET_PORT  GPIOC
#define SOFT_SWD_TARGET_RESET_PIN   3

// Инициализация пинов GPIO как программный SWD
void SoftSWD_Init();

// Низкоуровневые функции для управления пинами программного SWD ("вручную" дергать пины данных и тактирования high/low)
/*
void SoftSWD_CLK_High();
void SoftSWD_CLK_Low();
void SoftSWD_DATA_High();
void SoftSWD_DATA_High();
*/















// Функции протокола SWD (выполнить сброс/ переключиться с JTAG на SWD, функции приема/передачи данных)









#endif /* __SOFT_SWD_H__ */