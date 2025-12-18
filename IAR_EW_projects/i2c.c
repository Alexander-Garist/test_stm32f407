/**
  * @file    i2c.c
  * @brief   Файл содержит реализации функций I2C
  */

/** Includes **********************************************************************************************************/
#include <stdio.h>
#include "i2c.h"
#include "systick.h"

uint16_t data_2_byte[data_2_byte_SIZE];         // Массив двухбайтных данных 2048 чисел = 4096 байт

/***************************************** Статические функции ********************************************************/

// Включение тактирования модуля I2C
static void I2C_RCC_Enable(I2C_TypeDef* I2Cx)
{
    uint32_t Address_Shift = (uint32_t)I2Cx - (uint32_t)I2C1;   // Расчет сдвига порта от I2C1_BASE
    Address_Shift /= 1024;                                      // Сдвиг каждого порта от I2C1_BASE составляет 0x0400, в десятичной системе это 1024

	// Массив с масками для включения тактирования конкретного порта I2C
    uint32_t RCC_Enable_Mask[] = {
        RCC_APB1ENR_I2C1EN,
        RCC_APB1ENR_I2C2EN,
        RCC_APB1ENR_I2C3EN
    };
    RCC->APB1ENR |= RCC_Enable_Mask[Address_Shift];		// Включение только нужного порта
}

// Инициализация модуля I2C
static void I2C_Init_Pin(I2C_TypeDef* I2Cx)
{
    I2Cx->CR1 &= ~I2C_CR1_PE;       // Сброс флага PE для настройки I2C
    I2Cx->CR1 |= I2C_CR1_SWRST;     // Сброс I2C перед работой (установка регистра программного сброса)
    delay_ms(1);
    I2Cx->CR1 &= ~I2C_CR1_SWRST;	// Сброс регистра программного сброса

    I2Cx->CR2 = 16;     // Установка частоты периферийных устройств (0x10 == 16 МГц)
    I2Cx->CCR = 20;     // Установка частоты I2C 400 кГц     половина частоты APB / желаемая частота : 8 000 000 / 400 000 = 20
    I2Cx->TRISE = 6;    // Максимальное время подъема SCL

    I2Cx->CR1 |= I2C_CR1_PE;    // Включение I2C (Peripheral Enable)
    delay_ms(10);
}

/**************************** Функции ожидания установки флага и освобождения шины ************************************/

// Ожидание установки флага состояния SR1
static I2C_Status_t I2C_Wait_Flag_SR1(I2C_TypeDef* I2Cx, uint16_t I2C_flag, uint32_t I2C_timeout)
{
    uint32_t start_time = get_current_ms();
    while (!(I2Cx->SR1 & I2C_flag))
    {
        if (is_time_passed_ms(start_time, I2C_timeout)) return I2C_FLAG_TIMEOUT;
    }
    return I2C_OK;
}

// Ожидание освобождения шины I2C
static I2C_Status_t I2C_Wait_Bus_Busy(I2C_TypeDef* I2Cx, uint32_t I2C_timeout)
{
    uint32_t start_time = get_current_ms();
    while (I2Cx->SR2 & I2C_SR2_BUSY)
    {
        if (is_time_passed_ms(start_time, I2C_timeout)) return I2C_FLAG_TIMEOUT;
    }
    return I2C_OK;
}

/*********************************** Генерация START и STOP состояний *************************************************/

// Генерация состояния START
static I2C_Status_t I2C_Start(I2C_TypeDef* I2Cx, uint8_t I2C_address, uint8_t I2C_direction)
{
    //Генерация START
    I2Cx->CR1 |= I2C_CR1_START;
    if (I2C_Wait_Flag_SR1(I2Cx, I2C_SR1_SB, 10) != I2C_OK) return I2C_ERROR_START;

    //Отправка запроса
    I2Cx->DR = (I2C_address << 1) | I2C_direction;
    if (I2C_Wait_Flag_SR1(I2Cx, I2C_SR1_ADDR, 10) != I2C_OK) return I2C_ERROR_START;

    (void)I2Cx->SR2;	//Очистка ADDR
    return I2C_OK;
}

// Генерация состояния STOP
static void I2C_Stop(I2C_TypeDef* I2Cx)
{
    I2Cx->CR1 |= I2C_CR1_STOP;
}

/******************************************** Глобальные функции ******************************************************/

// Включение модуля I2C
void I2C_Enable(I2C_TypeDef* I2Cx)
{
    I2C_RCC_Enable(I2Cx);   //Включение тактирования I2C
    I2C_Init_Pin(I2Cx);     //Сброс перед работой, настройка частоты
}

// Проверка готовности подключенного устройства
I2C_Status_t I2C_is_Device_Ready(I2C_TypeDef* I2Cx, uint8_t I2C_device_addr)
{
    I2C_Status_t status = I2C_OK;
    if (I2C_Start(I2Cx, I2C_device_addr, 0) != I2C_OK) status = I2C_DEVICE_NO_ANSWER;
    I2C_Stop(I2Cx);
    return status;
}

// Вывод статуса выполенения функций I2C для отладки
void I2C_Status_Report(I2C_Status_t I2C_Function_Status)
{
    switch (I2C_Function_Status)
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

/*************************************** Функции чтения/записи I2C ****************************************************/

// Отправка данных по шине I2C
I2C_Status_t I2C_Write(I2C_TypeDef* I2Cx, uint8_t I2C_device_addr, uint8_t* I2C_data, uint16_t I2C_size)
{
	//Ожидание освобождения шины I2C, шина освобождается после генерации состояния STOP на шине
    if (I2C_Wait_Bus_Busy(I2Cx, 10) != I2C_OK) return I2C_BUS_IS_BUSY;
	//Генерация состояния START и отправка запроса на запись подключенному устройству
    if (I2C_Start(I2Cx, I2C_device_addr, 0) != I2C_OK) return I2C_ERROR_START;

    //Отправка данных
    for (uint16_t i = 0; i < I2C_size; i++)
    {
        I2Cx->DR = I2C_data[i];
        I2C_Wait_Flag_SR1(I2Cx, I2C_SR1_TXE, 10);
    }

    I2C_Wait_Flag_SR1(I2Cx, I2C_SR1_BTF, 10);
    I2C_Stop(I2Cx);		//Генерация состояния STOP
	//Проверка окончания соединения по I2C (попытка генерации повторного состояния START)
    while (I2C_Start(I2Cx, I2C_device_addr, 0) != I2C_OK)
    {
        delay_ms(1);
    }
    I2C_Stop(I2Cx);
    return I2C_OK;
}

// Прием данных по шине I2C
I2C_Status_t I2C_Read(I2C_TypeDef* I2Cx, uint8_t I2C_device_addr, uint8_t* I2C_data, uint16_t I2C_size)
{
    if (I2C_data == NULL) return I2C_DATA_NULL;

    //Ожидание освобождения шины I2C, шина освобождается после генерации состояния STOP на шине
    if (I2C_Wait_Bus_Busy(I2Cx, 10) != I2C_OK) return I2C_BUS_IS_BUSY;
	//Генерация состояния START и отправка запроса на чтение подключенному устройству
    if (I2C_Start(I2Cx, I2C_device_addr, 1) != I2C_OK) return I2C_ERROR_START;

    if (I2C_size > 1){
        I2Cx->CR1 |= I2C_CR1_ACK;        //Чтение нескольких байт, установка Acknowledge Enable
    }
    else{
        I2Cx->CR1 &= ~I2C_CR1_ACK;      //Одиночное чтение, сброс Acknowledge Enable
    }
    (void)I2Cx->SR2;

    for (uint16_t i = 0; i < I2C_size; i++)
    {
        if (i == I2C_size - 1)       //При чтении последнего байта сбросить флаг ACK и остановить прием данных
        {
            I2Cx->CR1 &= ~I2C_CR1_ACK;
            I2C_Stop(I2Cx);
        }
        I2C_Wait_Flag_SR1(I2Cx, I2C_SR1_RXNE, 10);
        I2C_data[i] = I2Cx->DR;
    }
    return I2C_OK;
}