/***********************************************************************************************************************
*   Программирование Flash памяти таргета через программный SWD
*       Знает про SWD из библиотеки soft_SWD.h
*       Знает про Flash конкретного таргета из библиотеки конкретного таргета (пока только N32g45x_Flash.h)
***********************************************************************************************************************/

#ifndef __FLASH_PROGRAMMER_H__
#define __FLASH_PROGRAMMER_H__

#include "soft_SWD.h"
#include "N32G45x_Flash.h"
//#include "STM32Fxx_Flash.h" для каждого вида целевых МК нужна своя библиотека, где будут прописаны регистры, маски и адреса


#define DHCSR_DBGKEY    (0xA05FUL << 16)  // я хз куда запихнуть этот ключ, он постоянный для ядер Cortex (как серий M, так и A и R)
#define DHCSR_CMD_HALT  (DHCSR_DBGKEY | CoreDebug_DHCSR_C_DEBUGEN_Msk | CoreDebug_DHCSR_C_HALT_Msk)
#define DHCSR_CMD_RUN   (DHCSR_DBGKEY | CoreDebug_DHCSR_C_DEBUGEN_Msk)



/** Стирает всю Flash */
void Erase_Flash_All();

/** Стирает только необходимое для записи количество страниц */
void Erase_Flash_size(uint32_t start_address, uint32_t size);

/** Запись в flash таргета */
void Program_Flash(uint32_t start_address, uint8_t* program_data, uint32_t program_size);

/** Подключение к таргету и получение IDCODE*/
uint32_t Connect_Target_GetIDCODE();

/** Halt ядра таргета */
void Target_Halt();

/** Запуск выполнения прошивки таргета */
void Target_Run();











#endif /* __FLASH_PROGRAMMER_H__ */