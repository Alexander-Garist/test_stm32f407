// flash.h для работы с модулем Flash

#ifndef __FLASH_H__
#define __FLASH_H__

// 1. Функция разблокировки периферии Flash памяти STM32
void Flash_Unlock(void);

// 2. Функция блокировки Flash памяти обратно
void Flash_Lock(void);

// 3. Функция стирания Сектора 11 (Очистка памяти перед записью образца)
void Flash_Erase_Sample_Sector(void);

// 4. Функция сохранения упакованного буфера из ОЗУ во Flash память
void Save_Sample_To_Flash(uint8_t *packed_buffer);


#endif /* __FLASH_H__ */