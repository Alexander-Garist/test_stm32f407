/**
  * @file    main.c
  * @brief   Основной файл проекта, содержит проверку работоспособности функций из разных модулей.
  */

/** Includes **********************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "CMSIS/stm32f4xx.h"
#include "systick.h"
#include "gpio.h"
#include "exti.h"
#include "i2c.h"
#include "BL24CM1A.h"
#include "spi.h"
#include "FM25Q08B.h"
#include "AD9833.h"
#include "usart.h"

#include "7_segment_indicator.h"
#include "LED.h"
#include "button.h"


int main()
{
    /** Инициализация портов GPIO User_Button и встроенных LED ********************************************************/

	GPIO_Button_Enable(GPIOA, 0);                       // Определение порта PA0 как вход с кнопкой
	LED_turnOFF_4_LED();                                // В начальный момент времени все светодиоды выключены

 	/** Включение обработки внешних прерываний для PA0 (User_Button) и системного таймера для неблокирующих задержек **/

    EXTI_Enable_Pin(EXTI_PortA, 0, BUTTON_TRIGGER);     // Включить внешние прерывания для этого пина
    SysTick_Init();										// Включение SysTick

	/** Настройка модуля I2C1 и инициализация портов GPIO в режиме альтернативной функции I2C *************************/

    GPIO_Enable_I2C(GPIOB, 7);		// Определение PB7 как SDA
    GPIO_Enable_I2C(GPIOB, 6);		// Определение PB6 как SCL

	// Включение модуля I2C1
    I2C_Enable(I2C1);

	/** Настройка модуля SPI2 и инициализация портов GPIO в режиме альтернативной функции SPI *************************/

    GPIO_set_HIGH(GPIOB, 12);								// Определение PB12 как OUTPUT
    GPIO_Enable_SPI(SPI2, SPI2_SCK_PORT, SPI2_SCK_PIN);     // Определение PB13 как SPI2_SCK
    GPIO_Enable_SPI(SPI2, SPI2_MISO_PORT, SPI2_MISO_PIN);   // Определение PB14 как SPI2_MISO
    GPIO_Enable_SPI(SPI2, SPI2_MOSI_PORT, SPI2_MOSI_PIN);   // Определение PB15 как SPI2_MOSI

	// Включение модуля SPI2 и программный сброс модуля Flash памяти
    SPI_Enable(SPI2);
    FM25Q08B_Reset(SPI2);

	/** Настройка модуля USART3 и инициализация портов GPIO в режиме альтернативной функции USART *********************/

	// Объявление структуры инициализации USART3
	USART_Init_Struct Init_USART3;

	// Заполнение полей структуры инициализации
	Init_USART3.USARTx = USART3;
	Init_USART3.GPIO_port_Tx = GPIOB;
	Init_USART3.GPIO_pin_Tx = 10;
	Init_USART3.GPIO_port_Rx = GPIOB;
	Init_USART3.GPIO_pin_Rx = 11;
	Init_USART3.baudrate = 115200;

	// Включение модуля USART3 с использованием структуры инициализации
	USART_Enable(&Init_USART3);

	/** Настройка модуля AD9833, инициализация используемых портов GPIO ***********************************************/

	/** Схема подключения AD9833 к STM32F407:
		Вывод AD9833	Цвет провода		Вывод STM32F407
		CS	---------> оранжевый	-----> PA4 (output)
		FSY ---------> белый		-----> PA3 (output)
		CLK ---------> черный		-----> PA5 (SPI1_SCK)
		DAT ---------> фиолетовый	-----> PA7 (SPI1_MOSI)
	*/

	GPIO_Enable_SPI(SPI1, GPIOA, 5);	// Определение PA5 как SPI1_SCK
	GPIO_Enable_SPI(SPI1, GPIOA, 7);	// Определение PA7 как SPI1_MOSI

	GPIO_set_HIGH(GPIOA, 3);	// PA3 -> FSY
	GPIO_set_HIGH(GPIOA, 4);	// PA4 -> CS

	SPI_Init_Mode_2(SPI1);		// Включение SPI1 (Mode 2: CPOL 1 CPHA 0)
	AD9833_Reset(SPI1);			// Сброс генератора

	/** Проверка работоспособности модуля USART3 (подключен модуль генератора сигналов AD9833) ************************/

	char message[] = "USART3 connected \r\n";
	USART_Transmit(USART3, message, strlen(message));

	// Создание экземпляра выходного сигнала, инициализация начальных параметров
	Signal_Parameters Output_Signal;
	Output_Signal.frequency = 123;
	Output_Signal.amplitude = 64;
	Output_Signal.wave_form = 2;

	// Включение генератора AD9833, выходной сигнал имеет параметры 1 кГц, 50% амплитуда, синусоида
	AD9833_Module_Init(SPI1, &Output_Signal);

	char BUFFER_USART[MAX_BUFFER_SIZE];				// строка, хранящая буфер USART, т.е. еще не отфильтрованный от мусора
	char BUFFER_USART_FILTERED[MAX_BUFFER_SIZE];	// отфильтрованный буфер USART

	/** Проверка работоспособности модуля SPI2 (подключен модуль памяти) **********************************************/

	uint8_t unique_id[8];
    FM25Q08B_Read_Unique_ID(SPI2, unique_id);
/*
    printf("Unique chip ID: ");
    for(int i = 0; i < 8; i++)
    {
    printf("%02X",unique_id[i]);
	}
    printf("\n");

    uint32_t jedec_id = FM25Q08B_Read_JEDEC_ID(SPI2);
    printf("JEDEC ID: %06X\n",jedec_id);

    uint16_t manufacturer_device_ID = FM25Q08B_Read_Manufacturer_ID(SPI2);
    printf("Manufacturer/Device ID: %04X\n", manufacturer_device_ID);
*/

    //uint8_t transmitted_data[FLASH_PAGE_SIZE];
    uint8_t received_data[FLASH_PAGE_SIZE];

    for (uint16_t i = 0; i < FLASH_PAGE_SIZE; i++)
    {
        received_data[i] = 55;  // Изначально в received_data мусор
    }

    /** Проверка чтения **********/

	    FM25Q08B_Status_t reception_status, transmission_status, erasion_status;
    reception_status = FM25Q08B_Read(
		SPI2,
		FLASH_START_ADDRESS,	// Адрес памяти модуля памяти, с которого начинается чтение
		received_data,			// Указатель на массив данных, в который записываются считанные данные
		FLASH_PAGE_SIZE			// Количество считанных байт
	);
    if (reception_status == FM25Q08B_OK)  GPIO_set_HIGH(GPIOD, 12);
/*
	printf("Проверка чтения\n");
	for (uint32_t i = 0; i < 10; i++)
	{
		printf("received_data[%d] = 0x%02X\n", i, received_data[i]);
	}
*/

	/** Проверка стирания памяти */

	erasion_status = FM25Q08B_Chip_Erase(SPI2);
    if (erasion_status == FM25Q08B_OK)  GPIO_set_HIGH(GPIOD, 13);

    reception_status = FM25Q08B_Read(
		SPI2,
		FLASH_START_ADDRESS,	// Адрес памяти модуля памяти, с которого начинается чтение
		received_data, 			// Указатель на массив данных, в который записываются считанные данные
		FLASH_PAGE_SIZE			// Количество считанных байт
	);
    if (reception_status == FM25Q08B_OK)  GPIO_set_HIGH(GPIOD, 12);
/*
	printf("Проверка стирания памяти\n");
	for (uint32_t i = 0; i < 10; i++)
	{
		printf("received_data[%d] = 0x%02X\n", i, received_data[i]);
	}
*/

    /** Проверка записи и чтения */

	//printf("Проверка записи\n");

	uint8_t transmitted_data_test[FLASH_PAGE_SIZE];
	uint8_t received_data_test[FLASH_PAGE_SIZE];

	// Одна страница 256 байт (0x100)
	uint32_t page_addr_offset = 0x100;

	/** В цикле заполняется массив отправляемых данных 256 байт (1 страница),
		отправляется и считываются данные в массив принятых данных.
		Каждая следующая страница имеет свой начальный адрес, поэтому в функциях FM25Q08B_Write и FM25Q08B_Read
		начальный адрес памяти вычисляется по формуле:
		page_address = flash_start_address + page_number * page_addr_offset
		где page_address - начальный адрес страницы
			flash_start_address - начальный адрес памяти (0x000000)
			page_number - номер текущей страницы (0 - 4095)
			page_addr_offset - объем одной страницы (256 байт == 0x100)
	*/
	for(uint16_t page_number = 0; page_number < 4096; page_number++)
	{
		// Заполнение массива отправляемых данных
		for(uint16_t i = 0; i < FLASH_PAGE_SIZE; i++)
		{
			transmitted_data_test[i] = page_number;
		}

		transmission_status = FM25Q08B_Write(
			SPI2,
			FLASH_START_ADDRESS + page_addr_offset * page_number,	// Адрес памяти модуля памяти, с которого начинается запись
			transmitted_data_test,									// Указатель на массив данных, которые записываются в память
			FLASH_PAGE_SIZE											// Количество записанных байт
		);
		if (transmission_status == FM25Q08B_OK)  GPIO_set_HIGH(GPIOD, 15);

		reception_status = FM25Q08B_Read(
			SPI2,
			FLASH_START_ADDRESS + page_addr_offset * page_number,	// Адрес памяти модуля памяти, с которого начинается чтение
			received_data_test,										// Указатель на массив данных, в который записываются считанные данные
			FLASH_PAGE_SIZE											// Количество считанных байт
		);
		if (reception_status == FM25Q08B_OK)  GPIO_set_HIGH(GPIOD, 12);
/*
		printf("Номер страницы: %d\n", page_number);
		for (uint8_t i = 0; i < 3; i++)
		{
			printf("received_data[%d] = 0x%02X\n", i, received_data_test[i]);
		}
		printf("\n");
*/
	}
    delay_ms(1000);
    LED_turnOFF_4_LED();

	// Если есть ошибки - загорится красный светодиод
    if ((reception_status != FM25Q08B_OK)
		|| (transmission_status != FM25Q08B_OK)
		|| (erasion_status != FM25Q08B_OK))
	{
		GPIO_set_HIGH(GPIOD, 14);
	}
    delay_ms(1000);

    /** Проверка работоспособности модуля I2C1 (подключен модуль памяти) **********************************************/

	uint8_t Received_Data[length_Received_Data] = { 0 };

	// Попытки подключиться к EEPROM
    uint8_t Error_Counter = 0;
    while (Error_Counter < MAX_NUMBER_ATTEMPTS_CONNECT_EEPROM)
    {
        if (I2C_is_Device_Ready(I2C1, EEPROM_ADDRESS) == I2C_OK)		// Получилось подключиться => запись/чтение и выход из попыток
        {
            GPIO_set_HIGH(GPIOD, 15);   delay_ms(150);					// Синий моргнул - EEPROM готова
            GPIO_set_LOW(GPIOD, 15);    delay_ms(150);

            // Передача данных порциями по 4096 байт в цикле
            for (uint32_t counter = 0; counter < length_All_Data / 2; counter += data_2_byte_SIZE)
            {
                // Заполнить массив передаваемых данных
                for (uint32_t i = 0; i < data_2_byte_SIZE; i++)	// 2048 * 2-byte
                {
                    data_2_byte[i] = i + counter;
                }
                uint8_t* Transmitted_Data = (uint8_t*)data_2_byte;				// 2-байтные данные должны отправляться как однобайтные
                uint32_t Transmission_ADDRESS = START_ADDRESS + counter * 2;    // Адрес отправки одной порции данных

                I2C_Status_t status_Transmission = BL24CM1A_Write(I2C1, EEPROM_ADDRESS, Transmission_ADDRESS, Transmitted_Data, length_Transmitted_Data);
            }
            LED_turnON_4_LED();
            delay_ms(500);
            LED_turnOFF_4_LED();

            // Записать 2 страницы по 256 байт буквами с адреса 0x180 по адрес 0x380
            uint8_t Transmitted_Data[send_SIZE];
            for (uint16_t i = 0; i < send_SIZE; i++)
            {
                Transmitted_Data[i] = 0x4A;
            }

            I2C_Status_t status_Transmission = BL24CM1A_Write(I2C1, EEPROM_ADDRESS, 0x180, Transmitted_Data, send_SIZE);

            if (status_Transmission != I2C_OK) GPIO_set_HIGH(GPIOD, 14);

            // Прием данных порциями по 4096 байт в цикле
            for (uint32_t counter = 0; counter < length_All_Data / 2; counter += data_2_byte_SIZE)
            {
                uint32_t Reception_ADDRESS = START_ADDRESS + counter * 2;       //Адрес чтения одной порции данных

                I2C_Status_t status_Reception = BL24CM1A_Read(I2C1, EEPROM_ADDRESS, Reception_ADDRESS, Received_Data, length_Received_Data);
                if (status_Reception == I2C_OK) GPIO_set_HIGH(GPIOD, 15);
                else GPIO_set_HIGH(GPIOD, 14);

                // Вывести номер операции чтения
                //printf("\nПрочитано 4096 байт, блок %d\n", counter / 2048);
                // Вывести первые 10 байт
/*				for (int i = 0; i < length_Received_Data; i++)
                {
                    if (i % 32 == 0) printf("\n");
                    printf("%02X ", Received_Data[i]);
                }
*/
            }
            GPIO_set_HIGH(GPIOD, 13);   // Загорелся оранжевый - чтение окончено
            break;						// Чтение/запись выполнены, выход из цикла попыток подключиться
        }
        if (I2C_is_Device_Ready(I2C1, EEPROM_ADDRESS) != I2C_OK)		// Не получилось подключиться за 10 попыток => выйти из цикла попыток и перейти к основному циклу
        {
            GPIO_set_HIGH(GPIOD, 14);   delay_ms(300);					// Красный моргнул - EEPROM не готова
            GPIO_set_LOW(GPIOD, 14);    delay_ms(300);
            Error_Counter++;
        }
    }

	/** Проверка работоспособности модуля семисегментного индикатора **************************************************/

    /**************** Основной цикл: мигание светодиодов и обработка нажатий кнопки ***********************************/
	/** Основной цикл теперь будет содержать не только моргание светодиодами и обработку нажатий кнопки, но и прием/передачу команд для генератора сигналов через USART */

	uint32_t start = get_current_ms();       			// Момент отсчета времени для основного цикла
	Last_Refresh_Time = get_current_ms();


	while (1)
	{
		// пока флаг RXNE не поднят это означает что ничего еще не пришло по USART, значит проверяем есть ли команды в очереди
		// если же RXNE поднят, значит нужно принять байты из USART
		while (!(USART3->SR & USART_SR_RXNE))
		{
			if (is_time_passed_ms(Last_Refresh_Time, Refresh_Period))	// отображение числа на индикаторе вызывается каждую 1 мс
			{
				switch (Blink_Mode)
				{
					case 0: Seven_Segment_Indicate_Number(Green_Blinks_Counter);	break;
					case 1: Seven_Segment_Indicate_Number(123456789);				break;
					case 2: Seven_Segment_Indicate_String("ABCDEFGH1234567890");	break;
				}
			}

			// Программа выйдет из этого блока только если что-то придет в приемник USART
			if (is_time_passed_ms(start, Blink_Period))		// Первая половина цикла: светодиоды выключены
			{
				LED_turnOFF_4_LED();
			}

			if (is_time_passed_ms(start, Blink_Period * 2))	// Вторая половина цикла: светодиоды включаются в соответствии с выбранным режимом
			{
				switch (Blink_Mode)
				{
					case 0: GPIO_set_HIGH(GPIOD, 12);	Green_Blinks_Counter++;		break;
					case 1: GPIO_set_HIGH(GPIOD, 13);	Orange_Blinks_Counter++;	break;
					case 2: GPIO_set_HIGH(GPIOD, 14);	Red_Blinks_Counter++;		break;
				}
				start = get_current_ms();
			}

			if ((Green_Blinks_Counter > 999) || (Orange_Blinks_Counter > 999) || (Red_Blinks_Counter > 999))
			{
				Green_Blinks_Counter = 0;
				Orange_Blinks_Counter = 0;
				Red_Blinks_Counter = 0;
			}

			while (Amount_of_Commands)	// выполнить все команды, какие есть в очереди
			{
				AD9833_Execute_Command(USART3, &Output_Signal);	// после выполнения команды отправить команду обратно отправителю

				AD9833_Module_Init(SPI1, &Output_Signal);
				//AD9833_print_Signal_Parameters(&Output_Signal);
			}
		}

		// RXNE поднят, прием байт
		USART_Receive(USART3, BUFFER_USART, '!');

		// фильтрация буфера и занесение команд в очередь
		AD9833_filter_Buffer_USART(BUFFER_USART, BUFFER_USART_FILTERED);
		AD9833_Parse_Commands_From_Buffer_USART(BUFFER_USART_FILTERED);
	}
}