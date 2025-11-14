#ifndef I2C_H
#define I2C_H

#include "CMSIS/stm32f4xx.h"
#include "systick.h"

typedef enum
{
    I2C_OK =                0,  //Функция успешно отработала
    I2C_DATA_NULL =         1,  //Ошибка массива данных для записи/чтения
    I2C_FLAG_TIMEOUT =      2,  //Ошибка ожидания установки флага
    I2C_BUS_IS_BUSY =       3,  //Шина I2C занята
    I2C_DEVICE_NO_ANSWER =  4,  //Нет ответа от подключенного устройства
    I2C_ERROR_WRITE =       5,  //Ошибка передачи данных
    I2C_ERROR_READ =        6,  //Ошибка приема данных
    I2C_ERROR_START =       7,  //Ошибка генерации состояния START
    I2C_ERROR_STOP =        8   //Ошибка генерации состояния STOP
}I2C_Status_t;

void I2C_Enable_Pin(I2C_TypeDef* I2Cx);                                         //Разрешение использовать конкретный I2C (I2C1, I2C2, I2C3), нужно использовать после настройки GPIO
I2C_Status_t I2C_is_Device_Ready(I2C_TypeDef* I2Cx, uint8_t device_addr);         //Проверка готовности подключенного устройства

//Только для чтения/записи памяти по I2C
I2C_Status_t I2C_Write_MEMORY(I2C_TypeDef* I2Cx, uint8_t device_addr, uint16_t reg_addr, uint8_t* data, uint16_t size);    //Запись в память
I2C_Status_t I2C_Read_MEMORY(I2C_TypeDef* I2Cx, uint8_t device_addr, uint16_t reg_addr, uint8_t* data, uint16_t size);     //Чтение из памяти

//Общий случай общения по I2C
I2C_Status_t I2C_Write(I2C_TypeDef* I2Cx, uint8_t device_addr, uint8_t* data, uint16_t size);    //Запись
I2C_Status_t I2C_Read(I2C_TypeDef* I2Cx, uint8_t device_addr, uint8_t* data, uint16_t size);     //Чтение

void I2C_Status_Report(I2C_Status_t Function_Status);
#endif