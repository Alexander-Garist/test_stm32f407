// flash.h для работы с модулем Flash STM32F407G

#ifndef __FLASH_H__
#define __FLASH_H__

#define FLASH_SECTOR_0_START_ADDRESS    0x08000000  // 16 KB 0x4000
#define FLASH_SECTOR_1_START_ADDRESS    0x08004000  // 16 KB 0x4000
#define FLASH_SECTOR_2_START_ADDRESS    0x08008000  // 16 KB 0x4000
#define FLASH_SECTOR_3_START_ADDRESS    0x0800C000  // 16 KB 0x4000
#define FLASH_SECTOR_4_START_ADDRESS    0x08010000  // 64 KB 0x10000
#define FLASH_SECTOR_5_START_ADDRESS    0x08020000  // 128 KB 0x20000
#define FLASH_SECTOR_6_START_ADDRESS    0x08040000  // 128 KB 0x20000
#define FLASH_SECTOR_7_START_ADDRESS    0x08060000  // 128 KB 0x20000
#define FLASH_SECTOR_8_START_ADDRESS    0x08080000  // 128 KB 0x20000
#define FLASH_SECTOR_9_START_ADDRESS    0x080A0000  // 128 KB 0x20000
#define FLASH_SECTOR_10_START_ADDRESS   0x080C0000  // 128 KB 0x20000
#define FLASH_SECTOR_11_START_ADDRESS   0x080E0000  // 128 KB 0x20000

#define FLASH_END_ADDRESS               0x080FFFFF

#define FLASH_SECTOR_0_SIZE     0x4000
#define FLASH_SECTOR_1_SIZE     0x4000
#define FLASH_SECTOR_2_SIZE     0x4000
#define FLASH_SECTOR_3_SIZE     0x4000
#define FLASH_SECTOR_4_SIZE     0x10000
#define FLASH_SECTOR_5_SIZE     0x20000
#define FLASH_SECTOR_6_SIZE     0x20000
#define FLASH_SECTOR_7_SIZE     0x20000
#define FLASH_SECTOR_8_SIZE     0x20000
#define FLASH_SECTOR_9_SIZE     0x20000
#define FLASH_SECTOR_10_SIZE    0x20000
#define FLASH_SECTOR_11_SIZE    0x20000



// Определить сколько и каких секторов нужно стереть + стереть требуемую память
void Erase_Memory(uint32_t memory_address, uint32_t size);

// Запись данных из ОЗУ во Flash память по адресу
void Save_To_Flash(uint32_t memory_address, uint8_t *data, uint32_t size);

#endif /* __FLASH_H__ */