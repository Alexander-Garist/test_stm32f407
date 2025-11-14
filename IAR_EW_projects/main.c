#include <stdio.h>
#include "CMSIS/stm32f4xx.h"

#include "systick.h"
#include "gpio.h"
#include "exti.h"

#include "i2c.h"
#include "BL24CM1A.h"

#include "spi.h"
#include "FM25Q08B.h"

//===========для GPIO, EXTI и SysTick==========================================================
#define TIME_SHORT      100      //Время моргания светодиодов в 3 режимах
#define TIME_MEDIUM     300
#define TIME_LONG       500

#define BUTTON_TRIGGER  EXTI_TRIGGER_FALLING    //Триггер внешнего прерывания при нажатии кнопки
#define DEBOUNCE_TIME   100                     //100 мс для отсеивания дребезга


static uint32_t time_delay = 100;               //Время задержки в тиках systick_counter
static uint32_t time_ms_one_interrupt = 1;      //Время в мс, за которое вызывается SysTick_Handler() 1 раз

volatile static uint32_t blink_mode = 0;    //Выбор режима моргания
//0 - моргают 4 светодиода быстро
//1 - моргает зеленый со средней частотой
//2 - моргает красный медленно
void set_time_delay(uint32_t time)                      //callback функция для установки частоты моргания
{
    time_delay = time;
}
void change_blink_mode(void (*callback)(uint32_t))      //Функция изменения режима моргания, вызывается в обработчике внешнего прерывания EXTI0_IRQHandler
{
    blink_mode++;                               //Нажатие кнопки переключает режим моргания на следующий
    if(blink_mode > 2) blink_mode %= 3;         //Защита от переполнения количества режимов моргания

    if(blink_mode == 0)callback(TIME_SHORT);    //time_delay = TIME_SHORT;
    if(blink_mode == 1)callback(TIME_MEDIUM);   //time_delay = TIME_MEDIUM;
    if(blink_mode == 2)callback(TIME_LONG);     //time_delay = TIME_LONG;
}
void LED_turnON_4_LED(void)                                                 //Включить все 4 светодиода
{
    GPIO_set_HIGH(GPIOD, 12);
    GPIO_set_HIGH(GPIOD, 13);
    GPIO_set_HIGH(GPIOD, 14);
    GPIO_set_HIGH(GPIOD, 15);
}
void LED_turnOFF_4_LED(void)                                                //Выключить все 4 светодиода
{
    GPIO_set_LOW(GPIOD, 12);
    GPIO_set_LOW(GPIOD, 13);
    GPIO_set_LOW(GPIOD, 14);
    GPIO_set_LOW(GPIOD, 15);
}

uint8_t button_state = 0;       //Состояние кнопки нажата/не нажата
uint32_t button_last_time = 0;  //Время последнего нажатия

void EXTI0_IRQHandler(void)                                                 //Обработчик внешнего прерывания (нажатия кнопки на выводе PA0)
{
    GPIO_set_HIGH(GPIOD, 15);                   //Индикация нажатия кнопки синим светодиодом
    uint32_t current_time = get_current_time(); //Момент времени начала обработки прерывания
    if(current_time - button_last_time > DEBOUNCE_TIME)
    {
        change_blink_mode(set_time_delay);      //Логика нажатия на кнопку - смена режима
        button_last_time = get_current_time();  //Обновить время последнего нажатия
    }
    EXTI_Clear_Flag(0);     //Сброс флага для выхода из прерывания
    LED_turnOFF_4_LED();    //Выключение всех светодиодов
}
//==========для I2C================================================================================================================================================
#define EEPROM_ADDRESS      0x50        //7-битный адрес подключенного модуля EEPROM, пины A0, A1 подтянуты к земле 1010000
#define START_ADDRESS       0x00        //Адрес в памяти EEPROM, куда записываются данные

#define length_All_Data             0x20000     //суммарное количество отправленных байт в память EEPROM 131072
#define length_Transmitted_Data     4096        //максимальное количество отправленных байт за 1 операцию отправки (16 страниц по 256 байт)
#define length_Received_Data        4096        //количество считанных байт из памяти EEPROM

#define MAX_NUMBER_ATTEMPTS_CONNECT_EEPROM  10  //Максимум попыток подключиться к EEPROM за один раз

#define data_2_byte_SIZE    (length_Transmitted_Data / 2)   //размер массива 2-байтных данных
#define send_SIZE 512
uint16_t data_2_byte[data_2_byte_SIZE];         //массив двухбайтных данных 2048 чисел = 4096 байт
//==========для SPI===============================================================================================================================================


int main()
{
    //===Инициализация портов GPIO на вход и выход=================
    //===Включение обработки внешних прерываний====================
    //===Включение системного таймера для неблокирующих задержек===
    GPIO_Button_Enable(GPIOA, 0);                       //Определение порта PA0 как вход с кнопкой
    EXTI_Enable_Pin(EXTI_PortA, 0, BUTTON_TRIGGER);     //Включить внешние прерывания для этого пина
    LED_turnOFF_4_LED();                                //В начальный момент времени все светодиоды выключены
    SysTick_Init(SystemCoreClock, time_ms_one_interrupt);              //Включение SysTick, аргумент - требуемый период увеличения счетчика тиков

    //============I2C==============================================
    //===Инициализация портов GPIO в режиме альтернативной функции=
    GPIO_Enable_I2C(GPIOB, 7);   //Определение PB7 как SDA
    GPIO_Enable_I2C(GPIOB, 6);   //Определение PB6 как SCL
    //===Включение модуля I2C на выбранных портах GPIO=============
    I2C_Enable_Pin(I2C1);
    //=============================================================

    //===========SPI=====================================================
    GPIO_set_HIGH(GPIOB, 12);       //Определение PB12 как OUTPUT
    //===Инициализация портов GPIO в режиме альтернативной функции=======
    GPIO_Enable_SPI(SPI2, SPI2_SCK_PORT, SPI2_SCK_PIN);     //Определение PB13 как SPI2_SCK
    GPIO_Enable_SPI(SPI2, SPI2_MISO_PORT, SPI2_MISO_PIN);   //Определение PB14 как SPI2_MISO
    GPIO_Enable_SPI(SPI2, SPI2_MOSI_PORT, SPI2_MOSI_PIN);   //Определение PB15 как SPI2_MOSI
    //===Включение модуля SPI============================================
    SPI_Enable_Pin(SPI2);
    FM25Q08B_Reset(SPI2);
    //=======================================================
    uint8_t unique_id[8];
    FM25Q08B_Read_Unique_ID(SPI2, unique_id);

    /*   printf("Unique chip ID: ");
    for(int i = 0; i < 8; i++)
    {
    printf("%02X",unique_id[i]);
}
    printf("\n");

    uint32_t jedec_id = FM25Q08B_Read_JEDEC_ID(SPI2);
    printf("JEDEC ID: %06X\n",jedec_id);

    uint16_t manufacturer_device_ID = FM25Q08B_Read_Manufacturer_ID(SPI2);
    printf("Manufacturer/Device ID: %04X\n", manufacturer_device_ID);
    */   //=======================================================


    uint8_t transmitted_data[FLASH_PAGE_SIZE * 2];      //256 чисел последовательно от 0x00 до 0xFF
    uint8_t received_data[FLASH_PAGE_SIZE * 2];

    for(uint16_t i = 0; i < FLASH_PAGE_SIZE; i++)
    {
        transmitted_data[i] = (uint8_t)i;//(uint8_t)i;
        received_data[i] = 55;  //изначально в received_data мусор
    }
    for(uint16_t i = FLASH_PAGE_SIZE; i < FLASH_PAGE_SIZE * 2; i++)
    {
        transmitted_data[i] = 0xAA;//(uint8_t)i;
        received_data[i] = 55;  //изначально в received_data мусор
    }


    //чтение
    FM25Q08B_Status_t reception_status, transmission_status, erasion_status;
    reception_status = FM25Q08B_Read(SPI2, FLASH_START_ADDRESS, received_data, FLASH_PAGE_SIZE * 2);
    if(reception_status == FM25Q08B_OK)  GPIO_set_HIGH(GPIOD, 12);

    /*   printf("\nдо записи 1 стр \n");
    for(uint16_t i = 0; i < 32; i++)
    {
    if(i % 32 ==0)printf("\n");
    printf("%02X ", received_data[i]);
}

    printf("\nдо записи 2 стр \n");
    for(uint16_t i = FLASH_PAGE_SIZE; i < FLASH_PAGE_SIZE + 32; i++)
    {
    if(i % 32 ==0)printf("\n");
    printf("%02X ", received_data[i]);
}
    */   //стирание и чтение
    //erasion_status = FM25Q08B_Sector_Erase_4KB(SPI2, FLASH_START_ADDRESS);
    //if(erasion_status == FM25Q08B_OK)  GPIO_set_HIGH(GPIOD, 13);

    erasion_status = FM25Q08B_Chip_Erase(SPI2);
    if(erasion_status == FM25Q08B_OK)  GPIO_set_HIGH(GPIOD, 13);

    reception_status = FM25Q08B_Read(SPI2, FLASH_START_ADDRESS, received_data, FLASH_PAGE_SIZE);
    if(reception_status == FM25Q08B_OK)  GPIO_set_HIGH(GPIOD, 12);

    /*   printf("\nпосле стирания received_data \n");
    for(uint16_t i = 0; i < 32; i++)
    {
    if(i % 32 ==0)printf("\n");
    printf("%02X ", received_data[i]);
}
    */
    //запись и чтение
    transmission_status = FM25Q08B_Write(SPI2, FLASH_START_ADDRESS, transmitted_data, FLASH_PAGE_SIZE * 2);
    if(transmission_status == FM25Q08B_OK)  GPIO_set_HIGH(GPIOD, 15);

    reception_status = FM25Q08B_Read(SPI2, FLASH_START_ADDRESS, received_data, FLASH_PAGE_SIZE + FLASH_PAGE_SIZE);
    if(reception_status == FM25Q08B_OK)  GPIO_set_HIGH(GPIOD, 12);

    /*   printf("\nпосле записи received_data 1 страница\n");
    for(uint16_t i = 0; i < 64; i++)
    {
    if(i % 32 ==0)printf("\n");
    printf("%02X ", received_data[i]);
}

    printf("\nпосле записи received_data 2 страница\n");
    for(uint16_t i = FLASH_PAGE_SIZE; i < FLASH_PAGE_SIZE + 64; i++)
    {
    if(i % 32 ==0)printf("\n");
    printf("%02X ", received_data[i]);
}

    */



    delay_ms(1000);
    LED_turnOFF_4_LED();

    if((reception_status!=FM25Q08B_OK)|| (transmission_status!=FM25Q08B_OK)|| (erasion_status!=FM25Q08B_OK))GPIO_set_HIGH(GPIOD, 14);
    delay_ms(1000);

    //==========================================================
    uint8_t Received_Data[length_Received_Data] = { 0 };

    uint8_t Error_Counter = 0;
    while(Error_Counter < MAX_NUMBER_ATTEMPTS_CONNECT_EEPROM)                   //Попытки подключиться к EEPROM
    {
        if(I2C_is_Device_Ready(I2C1, EEPROM_ADDRESS) == I2C_OK)                 //получилось подключиться => запись/чтение и выход из попыток
        {
            GPIO_set_HIGH(GPIOD, 15);   delay_ms(150);      //Синий моргнул - EEPROM готова
            GPIO_set_LOW(GPIOD, 15);    delay_ms(150);

            //Передача данных порциями по 4096 байт в цикле
            for(uint32_t counter = 0; counter < length_All_Data / 2; counter += data_2_byte_SIZE)
            {
                //printf("%d\n", counter);
                //заполнить массив передаваемых данных
                for(uint32_t i = 0; i < data_2_byte_SIZE; i++)//2048 * 2-byte
                {
                    data_2_byte[i] = i + counter;
                }

                uint8_t* Transmitted_Data = (uint8_t*)data_2_byte;  //2-байтные данные должны отправляться как однобайтные
                uint32_t Transmission_ADDRESS = START_ADDRESS + counter * 2;    //Адрес отправки одной порции данных

                I2C_Status_t status_Transmission = BL24CM1A_Write(I2C1, EEPROM_ADDRESS, Transmission_ADDRESS, Transmitted_Data, length_Transmitted_Data);
            }
            LED_turnON_4_LED();
            delay_ms(500);
            LED_turnOFF_4_LED();


            //записать 2 страницы по 256 байт буквами с адреса 0x180 по адрес 0x380

            uint8_t Transmitted_Data[send_SIZE];
            for(uint16_t i = 0; i < send_SIZE; i++)
            {
                Transmitted_Data[i] = 0x4A;
            }

            I2C_Status_t status_Transmission = BL24CM1A_Write(I2C1, EEPROM_ADDRESS, 0x180, Transmitted_Data, send_SIZE);

            if(status_Transmission != I2C_OK) GPIO_set_HIGH(GPIOD, 14);



            //Прием данных порциями по 4096 байт в цикле
            for(uint32_t counter = 0; counter < length_All_Data / 2; counter += data_2_byte_SIZE)
            {
                uint32_t Reception_ADDRESS = START_ADDRESS + counter * 2;       //Адрес чтения одной порции данных

                I2C_Status_t status_Reception = BL24CM1A_Read(I2C1, EEPROM_ADDRESS, Reception_ADDRESS, Received_Data, length_Received_Data);
                if(status_Reception == I2C_OK) GPIO_set_HIGH(GPIOD, 15);
                else GPIO_set_HIGH(GPIOD, 14);

                //вывести номер операции чтения
                //printf("\nПрочитано 4096 байт, блок %d\n", counter / 2048);
                //вывести первые 10 байт
                for(int i = 0; i < length_Received_Data; i++)
                {
                    if(i % 32 == 0) printf("\n");
                    //printf("%02X ", Received_Data[i]);
                }
            }
            GPIO_set_HIGH(GPIOD, 13);   //загорелся оранжевый - чтение окончено
            break;
        }
        if(I2C_is_Device_Ready(I2C1, EEPROM_ADDRESS) != I2C_OK)                 //не получилось за 10 попыток => просто выйти из цикла попыток и перейти к основному циклу
        {
            GPIO_set_HIGH(GPIOD, 14);   delay_ms(150);      //Красный моргнул - EEPROM не готова
            GPIO_set_LOW(GPIOD, 14);    delay_ms(150);
            Error_Counter++;
        }
    }



    //==========Основной цикл мигание светодиодов и обработка нажатий кнопки=============================================================================================
    uint32_t timeON = get_current_time();       //Момент отсчета времени для основного цикла
    while(1)
    {
        if(is_time_passed(timeON, time_delay))
        {
            LED_turnOFF_4_LED();
        }
        if(is_time_passed(timeON, time_delay * 2))
        {
            switch(blink_mode)
            {
                case 0: GPIO_set_HIGH(GPIOD, 12);   break;
                case 1: GPIO_set_HIGH(GPIOD, 13);   break;
                case 2: GPIO_set_HIGH(GPIOD, 14);   break;
            }
            timeON = get_current_time();
        }
    }
}