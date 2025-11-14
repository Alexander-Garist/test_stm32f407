/**
  * @file    i2c.c
  * @brief   Файл содержит реализации функций I2C
  */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "i2c.h"

/********************** Статические функции ***********************************/
static void I2C_RCC_Enable(I2C_TypeDef* I2Cx)   //Включение тактирования порта I2C (1,2 или 3)
{
    uint32_t Address_Shift = (uint32_t)I2Cx - (uint32_t)I2C1;   //Расчет сдвига порта от I2C1_BASE
    Address_Shift /= 1024;                                      //Сдвиг каждого порта от I2C1_BASE составляет 0x0400, в десятичной системе это 1024

    uint32_t RCC_Enable_Mask[] = {      //Массив с масками для включения тактирования конкретного порта I2C
        RCC_APB1ENR_I2C1EN,
        RCC_APB1ENR_I2C2EN,
        RCC_APB1ENR_I2C3EN
    };
    RCC->APB1ENR |= RCC_Enable_Mask[Address_Shift];     //Включение только нужного порта
}
static void I2C_Init_Pin(I2C_TypeDef* I2Cx)     //Инициализация выбранного порта I2C
{
    I2Cx->CR1 &= ~I2C_CR1_PE;       //сброс флага PE для настройки I2C
    I2Cx->CR1 |= I2C_CR1_SWRST;     //Сброс I2C перед работой
    delay_ms(1);
    I2Cx->CR1 &= ~I2C_CR1_SWRST;

    I2Cx->CR2 = 16;     //Установка частоты периферийных устройств (0x10 == 16 МГц)
    I2Cx->CCR = 20;     //Установка частоты I2C 100 кГц     половина частоты APB / желаемая частота : 8 000 000 / 100 000 = 80 (0x50)
    I2Cx->TRISE = 6;    //Максимальное время подъема SCL

    I2Cx->CR1 |= I2C_CR1_PE;    //Включение I2C (Peripheral Enable)
    delay_ms(10);
}

//===Функции ожидания установки флага и освобождения шины (без возврата статуса ошибки)================================================

static I2C_Status_t I2C_Wait_Flag_SR1(I2C_TypeDef* I2Cx, uint16_t flag, uint32_t timeout)     //Ожидание установки флага состояния SR1
{
    uint32_t start_time = get_current_time();
    while(!(I2Cx->SR1 & flag))
    {
        if(is_time_passed(start_time, timeout)) return I2C_FLAG_TIMEOUT;
    }
    return I2C_OK;
}
static I2C_Status_t I2C_Wait_Bus_Busy(I2C_TypeDef* I2Cx, uint32_t timeout)                    //Ожидание освобождения шины
{
    uint32_t start_time = get_current_time();
    while(I2Cx->SR2 & I2C_SR2_BUSY)
    {
        if(is_time_passed(start_time, timeout)) return I2C_FLAG_TIMEOUT;
    }
    return I2C_OK;
}

//===Генерация START и STOP состояний===================================================================================================
static I2C_Status_t I2C_Start(I2C_TypeDef* I2Cx, uint8_t address, uint8_t direction)  //Начало передачи по I2C
{
    //Генерация START
    I2Cx->CR1 |= I2C_CR1_START;
    if(I2C_Wait_Flag_SR1(I2Cx, I2C_SR1_SB, 10) != I2C_OK) return I2C_ERROR_START;

    //Отправка запроса
    I2Cx->DR = (address << 1) | direction;
    if(I2C_Wait_Flag_SR1(I2Cx, I2C_SR1_ADDR, 10) != I2C_OK) return I2C_ERROR_START;

    (void)I2Cx->SR2;//Очистка ADDR
    return I2C_OK;
}
static void I2C_Stop(I2C_TypeDef* I2Cx)                                             //Окончание передачи по I2C
{
    I2Cx->CR1 |= I2C_CR1_STOP;
}

//===========================Публичные функции===========================================================================================
void I2C_Enable_Pin(I2C_TypeDef* I2Cx)                                          //Разрешение использовать конкретный I2C (I2C1, I2C2, I2C3)
{
    I2C_RCC_Enable(I2Cx);   //Включение тактирования I2C
    I2C_Init_Pin(I2Cx);     //Сброс перед работой, настройка частоты
}
I2C_Status_t I2C_is_Device_Ready(I2C_TypeDef* I2Cx, uint8_t device_addr)          //Проверка готовности подключенного устройства
{
    I2C_Status_t status = I2C_OK;
    if(I2C_Start(I2Cx, device_addr, 0) != I2C_OK) status = I2C_DEVICE_NO_ANSWER;

    I2C_Stop(I2Cx);
    return status;
}
void I2C_Status_Report(I2C_Status_t Function_Status)                              //Вывод статуса выполнения функции в терминал
{
    switch(Function_Status)
    {
        case (I2C_OK):                  printf("\n I2C_OK \n");                 break;
        case (I2C_DATA_NULL):           printf("\n I2C_DATA_NULL \n");          break;
        case (I2C_FLAG_TIMEOUT):        printf("\n I2C_FLAG_TIMEOUT \n");       break;
        case (I2C_BUS_IS_BUSY):         printf("\n I2C_BUS_IS_BUSY \n");        break;
        case (I2C_DEVICE_NO_ANSWER):    printf("\n I2C_DEVICE_NO_ANSWER \n");   break;
        case (I2C_ERROR_WRITE):         printf("\n I2C_ERROR_WRITE \n");        break;
        case (I2C_ERROR_READ):          printf("\n I2C_ERROR_READ \n");         break;
        case (I2C_ERROR_START):         printf("\n I2C_ERROR_START \n");        break;
    }
}
//=============================ФУНКЦИИ ЧТЕНИЯ ЗАПИСИ=======================================================================================
I2C_Status_t I2C_Write(I2C_TypeDef* I2Cx, uint8_t device_addr, uint8_t* data, uint16_t size)     //Запись
{

    if(I2C_Wait_Bus_Busy(I2Cx, 10) != I2C_OK) return I2C_BUS_IS_BUSY;           //Ожидание освобождения шины I2C, шина освобождается после генерации состояния STOP на шине
    if(I2C_Start(I2Cx, device_addr, 0) != I2C_OK) return I2C_ERROR_START;       //Генерация состояния START и отправка запроса на запись подключенному устройству


    //Отправка данных
    for(uint16_t i = 0; i < size; i++)
    {
        I2Cx->DR = data[i];
        I2C_Wait_Flag_SR1(I2Cx, I2C_SR1_TXE, 10);
    }
    I2C_Wait_Flag_SR1(I2Cx, I2C_SR1_BTF, 10);
    I2C_Stop(I2Cx);
    while(I2C_Start(I2Cx, device_addr, 0) != I2C_OK)
    {
        delay_ms(1);
    }
    I2C_Stop(I2Cx);
    return I2C_OK;
}



I2C_Status_t I2C_Read(I2C_TypeDef* I2Cx, uint8_t device_addr, uint8_t* data, uint16_t size)      //Чтение
{
    if(data == NULL) return I2C_DATA_NULL;

    //запрос на чтение
    if(I2C_Wait_Bus_Busy(I2Cx, 10) != I2C_OK) return I2C_BUS_IS_BUSY;          //Ожидание освобождения шины I2C, шина освобождается после генерации состояния STOP на шине
    if(I2C_Start(I2Cx, device_addr, 1) != I2C_OK) return I2C_ERROR_START;       //Старт для чтения

    if(size > 1){
        I2Cx->CR1 |= I2C_CR1_ACK;        //Чтение нескольких байт, Acknowledge Enable
    }
    else{
        I2Cx->CR1 &= ~I2C_CR1_ACK;      //Одиночное чтение, сброс Acknowledge Disable
    }
    (void)I2Cx->SR2;

    for(uint16_t i = 0; i < size; i++)
    {
        if(i == size - 1)       //При чтении последнего байта нужно сбросить флаг ACK и остановить прием данных
        {
            I2Cx->CR1 &= ~I2C_CR1_ACK;
            I2C_Stop(I2Cx);
        }
        I2C_Wait_Flag_SR1(I2Cx, I2C_SR1_RXNE, 10);
        data[i] = I2Cx->DR;
    }
    return I2C_OK;
}
