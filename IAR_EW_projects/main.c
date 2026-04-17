/**
  * @file    main.c
  * @brief   Основной файл проекта, содержит проверку работоспособности функций из разных модулей.
  */

/** Includes **********************************************************************************************************/
#include <stdio.h>
#include <string.h>

#include "CMSIS/stm32f4xx.h"
#include "systick.h"
#include "gpio.h"
#include "exti.h"
#include "BL24CM1A.h"
#include "FM25Q08B.h"
#include "AD9833.h"
#include "usart.h"
#include "7_segment_indicator.h"
#include "LED.h"
#include "button.h"

#include "soft_SWD.h"
typedef enum
{
	TASK_OK = 0,
	TASK_ERROR = -1
}Task_Status_t;

/** Прототипы задач */
/*
void Task_Press_Button();																// Задача обработки нажатия кнопки
Task_Status_t Task_AD9833_ExecuteCommand(Signal_Parameters* out_signal, uint32_t TASK_period);	// Задача AD9833 по выполнению команд
Task_Status_t Task_AD9833_GetCommand(char* buffer, char* filtered_buffer);						// Задача AD9833 по парсингу команд из буфера USART
Task_Status_t Task_Indicator(uint32_t TASK_period);												// Задача индикатора
Task_Status_t Task_LED_Blink(uint32_t TASK_period);												// Задача LED
void Task_Enable_Peripherals(void);																// Задача включения периферии
Task_Status_t Task_Test_I2C(void);																// Задача проверки работоспособности модуля I2C
Task_Status_t Task_Test_SPI(void);																// Задача проверки работоспособности модуля SPI
*/

int main()
{
    Clock_Config_168MHz_HSI();
    SysTick_Init();
    SoftSWD_Init();

    uint8_t bit = 0x0;
    uint8_t ACK = 0x0;

	while (1)
	{
        uint32_t idcode = 0;
        uint32_t data_parity = 0;
        ACK = 0x0;

        // Синхронизация хоста с таргетом, переключение на SWD
        SOFT_SWD_LINE_RESET();
        SOFT_SWD_SWITCH_JTAG_TO_SWD();
        SOFT_SWD_LINE_RESET();

        for(int i = 0; i < 10; i++) SOFT_SWD_WRITE_BIT(0);

        // start byte
        SOFT_SWD_WRITE_BIT(1);
        SOFT_SWD_WRITE_BIT(0);
        SOFT_SWD_WRITE_BIT(1);
        SOFT_SWD_WRITE_BIT(0);
        SOFT_SWD_WRITE_BIT(0);
        SOFT_SWD_WRITE_BIT(1);
        SOFT_SWD_WRITE_BIT(0);
        SOFT_SWD_WRITE_BIT(1);

        SOFT_SWD_TRN_TO_INPUT();

        SOFT_SWD_READ_BIT(bit);
        ACK |= bit << 0;

        SOFT_SWD_READ_BIT(bit);
        ACK |= bit << 1;

        SOFT_SWD_READ_BIT(bit);
        ACK |= bit << 2;

        // Читаем данные (32 бита)
        for (int i = 0; i < 32; i++)
        {
            SOFT_SWD_READ_BIT(bit);
            idcode |= (bit << i);
            data_parity++;
        }
        uint8_t parity_bit;
        SOFT_SWD_READ_BIT(parity_bit);

        SOFT_SWD_TRN_TO_OUTPUT();

        for(int i = 0; i < 10; i++) SOFT_SWD_WRITE_BIT(0);

        //printf("ACK = %d\n", ACK);
        if (ACK == 0x1) GPIO_set_HIGH(GPIOD, 15);
        else GPIO_set_HIGH(GPIOD, 14);
        delay_ticks(500000);
        GPIO_set_LOW(GPIOD, 15);
        GPIO_set_LOW(GPIOD, 14);
	}
}

/*************************************************************************************************************************/

// Задача AD9833 по выполнению команд из очереди
////Task_Status_t Task_AD9833_ExecuteCommand(Signal_Parameters* out_signal, uint32_t TASK_period)
////{
////	static uint8_t TASK_state;					// Состояние задачи
////	static uint32_t TASK_last_execution_time;	// Момент последнего выполнения задачи
////
////	if (is_time_passed_ms(TASK_last_execution_time, TASK_period)) TASK_state = 1;
////	else TASK_state = 0;
////
////	switch (TASK_state)
////	{
////		case 0: break;
////		case 1:
////		{
////			while (Amount_of_Commands)	// выполнить все команды генератора сигналов, какие есть в очереди
////			{
////				AD9833_Execute_Command(USART3, out_signal);	// после выполнения команды отправить команду обратно отправителю
////				AD9833_Module_Init(SPI1, out_signal);
////			}
////			TASK_last_execution_time = get_current_ms();
////		}
////	}
////	return TASK_OK;
////}

// Задача AD9833 по парсингу команд из буфера USART
////Task_Status_t Task_AD9833_GetCommand(char* buffer, char* filtered_buffer)
////{
////	// Фильтрация буфера и занесение команд в очередь
////	AD9833_filter_Buffer_USART(buffer, filtered_buffer);
////	AD9833_Parse_Commands_From_Buffer_USART(filtered_buffer);
////
////	return TASK_OK;
////}

// Задача индикатора
////Task_Status_t Task_Indicator(uint32_t TASK_period)
////{
////	static uint8_t TASK_state = 0;
////	static uint32_t TASK_last_execution_time;
////	// если времени прошло достаточно с последнего выполнения этой задачи, то нужно выполнить эту задачу снова
////	if (is_time_passed_ms(TASK_last_execution_time, TASK_period)) TASK_state = 1;
////
////	switch (TASK_state)
////	{
////		case 0: break;
////		case 1:
////		switch (Blink_Mode)
////		{
////			case 0: Seven_Segment_Indicate_Number(Green_Blinks_Counter);	break;
////			case 1: Seven_Segment_Indicate_Number(123456789);				break;
////			case 2: Seven_Segment_Indicate_String("ABCDEFGH1234567890");	break;
////		}
////		TASK_state = 0;
////		TASK_last_execution_time = get_current_ms();
////	}
////	return TASK_OK;
////}

// Задача LED (переключение режима моргания через нажатие кнопки)
////Task_Status_t Task_LED_Blink(uint32_t TASK_period)
////{
////	static uint8_t Task_State;
////	static uint8_t LED_State;
////	static uint32_t TASK_last_execution_time;
////
////	// если прошло нужное время после последнего выполнения задачи
////	if (is_time_passed_ms(TASK_last_execution_time, TASK_period))
////	{
////		Task_State = 1;
////	}
////	else
////	{
////		Task_State = 0;
////	}
////
////	switch (Task_State)
////	{
////		case 0: break;
////
////		case 1:
////		switch (LED_State)
////		{
////			case 0:
////			{
////				LED_State = 1;
////				switch (Blink_Mode)
////				{
////					case 0: GPIO_set_HIGH(GPIOD, 12);	Green_Blinks_Counter++;		break;
////					case 1: GPIO_set_HIGH(GPIOD, 13);	Orange_Blinks_Counter++;	break;
////					case 2: GPIO_set_HIGH(GPIOD, 14);	Red_Blinks_Counter++;		break;
////				}
////				if ((Green_Blinks_Counter > 999) || (Orange_Blinks_Counter > 999) || (Red_Blinks_Counter > 999))
////				{
////					Green_Blinks_Counter = 0;
////					Orange_Blinks_Counter = 0;
////					Red_Blinks_Counter = 0;
////				}
////				break;
////			}
////
////			case 1:
////			{
////				LED_State = 0;
////				LED_turnOFF_4_LED();
////				break;
////			}
////
////		}
////		TASK_last_execution_time = get_current_ms();
////	}
////	return TASK_OK;
////}


//////void Task_Press_Button(void)
//////{
//////	// Проверка на дребезг. Отключается обработка внешних прерываний, определяется причина прерывания
//////	//	(нажатие или отпускание кнопки), запоминаются моменты нажатия и отпускания
//////	if (Interrupt_EXTI0_Occured)
//////	{
//////		EXTI_Disable_Pin(BUTTON_EXTI_PORT, BUTTON_GPIO_PIN);
//////
//////		if (is_time_passed_ms(Last_Interrupt_Time_ms, DEBOUNCE_TIME))
//////		{
//////			if (GPIO_Read_Pin(BUTTON_GPIO_PORT, BUTTON_GPIO_PIN))
//////			{
//////				// Кнопка нажата
//////				Button_Last_Press_Time_ms = get_current_ms();
//////				button_is_released = FALSE;
//////			}
//////			else
//////			{
//////				// Кнопка отпущена
//////				Button_Last_Release_Time_ms = get_current_ms();
//////				button_is_released = TRUE;
//////			}
//////
//////			Interrupt_EXTI0_Occured = FALSE;
//////		}
//////	}
//////
//////	// Если после вызова прерывания прошло 30 мс, значит прерывание уже обработано => включается обработка прерываний
//////	if (is_time_passed_ms(Last_Interrupt_Time_ms, INTERRUPT_ENABLE_TIME))
//////	{
//////		EXTI_Enable_Pin(BUTTON_EXTI_PORT, BUTTON_GPIO_PIN, BUTTON_TRIGGER);
//////	}
//////
//////	// После отпускания кнопки можно определить тип нажатия
//////	if (button_is_released)
//////	{
//////		// Флаг button_is_released сбрасывается, т.к. сейчас однозначно будет определена длительность нажатия
//////		button_is_released = FALSE;
//////		Button_Press_Duration_ms = Button_Last_Release_Time_ms - Button_Last_Press_Time_ms;
//////
//////		// Длинное нажатие => точно одиночное длинное
//////		if (Button_Press_Duration_ms >= LONG_PRESS_TIME)
//////		{
//////			Button_State = SINGLE_LONG_PRESS;
//////			Short_Press_Release_ms = FALSE;
//////		}
//////
//////		// Короткое нажатие => или одиночное, или двойное
//////		else
//////		{
//////			if (Button_Press_Duration_ms > SHORT_PRESS_TIME)
//////			{
//////				// Если в Short_Press_Release_ms НЕ сохранено какое-то ненулевое время, значит это только первое короткое нажатие, возможно будет еще второе
//////				if (!Short_Press_Release_ms)
//////				{
//////					Short_Press_Release_ms = Button_Last_Release_Time_ms;
//////				}
//////				// Если уже что-то было сохранено, значит произошло двойное нажатие
//////				else
//////				{
//////					Button_State = DOUBLE_PRESS;
//////					Short_Press_Release_ms = FALSE;
//////				}
//////			}
//////		}
//////	}
//////
//////
//////	// Если не произошло второго короткого нажатия за время RELEASED_TIME => нажатие одиночное короткое
//////	if ((Short_Press_Release_ms) && (is_time_passed_ms(Short_Press_Release_ms, RELEASED_TIME)))
//////	{
//////		Button_State = SINGLE_SHORT_PRESS;
//////		Short_Press_Release_ms = FALSE;
//////	}
//////
//////	// Конечный автомат для выбора нужного обработчика определенного нажатия
//////	switch (Button_State)
//////	{
//////		case (RELEASED): break;
//////		case (SINGLE_SHORT_PRESS):	Button_Single_Short_Press_Handler();	break;
//////		case (SINGLE_LONG_PRESS):	Button_Single_Long_Press_Handler();		break;
//////		case (DOUBLE_PRESS):		Button_Double_Press_Handler();			break;
//////	}
//////}

// Включение периферии
////void Task_Enable_Peripherals(void)
////{
////	/** Включение системного таймера **********************************************************************************/
////	SysTick_Init();
////
////	/** Инициализация портов GPIO User_Button и встроенных LED ********************************************************/
////
////	GPIO_Button_Enable(GPIOA, 0);                       // Определение порта PA0 как вход с кнопкой
////	LED_turnOFF_4_LED();                                // В начальный момент времени все светодиоды выключены
////
//// 	/** Включение обработки внешних прерываний для PA0 (User_Button) и системного таймера для неблокирующих задержек **/
////
////    EXTI_Enable_Pin(EXTI_PortA, 0, BUTTON_TRIGGER);     // Включить внешние прерывания для этого пина
////
////	/** Настройка модуля I2C1 и инициализация портов GPIO в режиме альтернативной функции I2C *************************/
////
////    GPIO_Enable_I2C(GPIOB, 7);		// Определение PB7 как SDA
////    GPIO_Enable_I2C(GPIOB, 6);		// Определение PB6 как SCL
////
////	// Включение модуля I2C1
////    I2C_Enable(I2C1);
////
////	/** Настройка модуля SPI2 и инициализация портов GPIO в режиме альтернативной функции SPI *************************/
////
////    GPIO_set_HIGH(GPIOB, 12);								// Определение PB12 как OUTPUT
////    GPIO_Enable_SPI(SPI2, SPI2_SCK_PORT, SPI2_SCK_PIN);     // Определение PB13 как SPI2_SCK
////    GPIO_Enable_SPI(SPI2, SPI2_MISO_PORT, SPI2_MISO_PIN);   // Определение PB14 как SPI2_MISO
////    GPIO_Enable_SPI(SPI2, SPI2_MOSI_PORT, SPI2_MOSI_PIN);   // Определение PB15 как SPI2_MOSI
////
////	// Включение модуля SPI2 и программный сброс модуля Flash памяти
////    SPI_Enable(SPI2);
////    FM25Q08B_Reset(SPI2);
////
////	/** Настройка модуля USART3 и инициализация портов GPIO в режиме альтернативной функции USART *********************/
////
////	// Объявление структуры инициализации USART3 и заполнение ее полей
////	USART_Init_Struct Init_USART3;
////	Init_USART3.USARTx = USART3;
////	Init_USART3.GPIO_port_Tx = GPIOB;
////	Init_USART3.GPIO_pin_Tx = 10;
////	Init_USART3.GPIO_port_Rx = GPIOB;
////	Init_USART3.GPIO_pin_Rx = 11;
////	Init_USART3.baudrate = BRR_115200;
////
////	// Включение модуля USART3 с использованием структуры инициализации
////	USART_Enable(&Init_USART3);
////
////	/** Настройка модуля AD9833, инициализация используемых портов GPIO ***********************************************/
////
////	/** Схема подключения AD9833 к STM32F407:
////		Вывод AD9833	Вывод STM32F407
////		CS	----------> PA4 (output)
////		FSY ----------> PA3 (output)
////		CLK ----------> PA5 (SPI1_SCK)
////		DAT ----------> PA7 (SPI1_MOSI)
////	*/
/////*
////	GPIO_Enable_SPI(SPI1, GPIOA, 5);	// Определение PA5 как SPI1_SCK
////	GPIO_Enable_SPI(SPI1, GPIOA, 7);	// Определение PA7 как SPI1_MOSI
////
////	GPIO_set_HIGH(GPIOA, 3);	// PA3 -> FSY
////	GPIO_set_HIGH(GPIOA, 4);	// PA4 -> CS
////
////	SPI_Init_Mode_2(SPI1);		// Включение SPI1 (Mode 2: CPOL 1 CPHA 0)
////	AD9833_Reset(SPI1);			// Сброс генератора
////*/
////}

// Проверка работоспособности модуля I2C
////Task_Status_t Task_Test_I2C(void)
////{
////	Task_Status_t Task_Status = TASK_OK;	// Статус выполнения задачи
////	uint8_t Received_Data[LENGTH_RECEIVED_DATA] = { 0 };
////
////	// Попытки подключиться к EEPROM
////	uint8_t Error_Counter = 0;
////	while (Error_Counter < MAX_NUMBER_ATTEMPTS_CONNECT_EEPROM)
////	{
////		if (I2C_is_Device_Ready(I2C1, EEPROM_ADDRESS) == I2C_OK)		// Получилось подключиться => запись/чтение и выход из попыток
////		{
////			GPIO_set_HIGH(GPIOD, 15);   delay_ms(150);					// Синий моргнул - EEPROM готова
////			GPIO_set_LOW(GPIOD, 15);    delay_ms(150);
////
////			// Передача данных порциями по 4096 байт в цикле
////			for (uint32_t counter = 0; counter < LENGTH_ALL_DATA / 2; counter += DATA_2_BYTE_SIZE)
////			{
////				// Заполнить массив передаваемых данных
////				for (uint32_t i = 0; i < DATA_2_BYTE_SIZE; i++)	// 2048 * 2-byte
////				{
////					data_2_byte[i] = i + counter;
////				}
////				uint8_t* Transmitted_Data = (uint8_t*)data_2_byte;				// 2-байтные данные должны отправляться как однобайтные
////				uint32_t Transmission_ADDRESS = START_ADDRESS + counter * 2;    // Адрес отправки одной порции данных
////
////				I2C_Status_t status_Transmission = BL24CM1A_Write(I2C1, EEPROM_ADDRESS, Transmission_ADDRESS, Transmitted_Data, LENGTH_TRANSMITTED_DATA);
////				if (status_Transmission != I2C_OK) Task_Status = TASK_ERROR;
////			}
////			LED_turnON_4_LED();
////			delay_ms(500);
////			LED_turnOFF_4_LED();
////
////			// Записать 2 страницы по 256 байт буквами с адреса 0x180 по адрес 0x380
////			uint8_t Transmitted_Data[SEND_SIZE];
////			for (uint16_t i = 0; i < SEND_SIZE; i++)
////			{
////				Transmitted_Data[i] = 0x4A;
////			}
////
////			I2C_Status_t status_Transmission = BL24CM1A_Write(I2C1, EEPROM_ADDRESS, 0x180, Transmitted_Data, SEND_SIZE);
////
////			if (status_Transmission != I2C_OK) Task_Status = TASK_ERROR;
////
////			// Прием данных порциями по 4096 байт в цикле
////			for (uint32_t counter = 0; counter < LENGTH_ALL_DATA / 2; counter += DATA_2_BYTE_SIZE)
////			{
////				uint32_t Reception_ADDRESS = START_ADDRESS + counter * 2;       // Адрес чтения одной порции данных
////
////				I2C_Status_t status_Reception = BL24CM1A_Read(I2C1, EEPROM_ADDRESS, Reception_ADDRESS, Received_Data, LENGTH_RECEIVED_DATA);
////				if (status_Reception == I2C_OK) GPIO_set_HIGH(GPIOD, 15);
////				else Task_Status = TASK_ERROR;
////
////				// Вывести номер операции чтения
////				//printf("\nПрочитано 4096 байт, блок %d\n", counter / 2048);
////				// Вывести первые 10 байт
/////*				for (int i = 0; i < LENGTH_RECEIVED_DATA; i++)
////				{
////					if (i % 32 == 0) printf("\n");
////					printf("%02X ", Received_Data[i]);
////				}
////*/
////			}
////			GPIO_set_HIGH(GPIOD, 13);   // Загорелся оранжевый - чтение окончено
////			break;						// Чтение/запись выполнены, выход из цикла попыток подключиться
////		}
////		if (I2C_is_Device_Ready(I2C1, EEPROM_ADDRESS) != I2C_OK)		// Не получилось подключиться за 10 попыток => выйти из цикла попыток и перейти к основному циклу
////		{
////			GPIO_set_HIGH(GPIOD, 14);   delay_ms(300);					// Красный моргнул - EEPROM не готова
////			GPIO_set_LOW(GPIOD, 14);    delay_ms(300);
////			Error_Counter++;
////			Task_Status = TASK_ERROR;
////		}
////	}
////	return Task_Status;
////}

// Проверка работоспособности модуля SPI
////Task_Status_t Task_Test_SPI(void)
////{
////	Task_Status_t Task_Status = TASK_OK;	// Статус выполнения задачи
////
////	uint8_t unique_id[8];
////	FM25Q08B_Read_Unique_ID(SPI2, unique_id);
/////*
////	printf("Unique chip ID: ");
////	for(int i = 0; i < 8; i++)
////	{
////		printf("%02X",unique_id[i]);
////	}
////	printf("\n");
////
////	uint32_t jedec_id = FM25Q08B_Read_JEDEC_ID(SPI2);
////	printf("JEDEC ID: %06X\n",jedec_id);
////
////	uint16_t manufacturer_device_ID = FM25Q08B_Read_Manufacturer_ID(SPI2);
////	printf("Manufacturer/Device ID: %04X\n", manufacturer_device_ID);
////*/
////
////	//uint8_t transmitted_data[FLASH_PAGE_SIZE];
////	uint8_t received_data[FLASH_PAGE_SIZE];
////
////	// Изначально в received_data мусор для проверки корректности заполнения массива данных
////	for (uint16_t i = 0; i < FLASH_PAGE_SIZE; i++)
////	{
////		received_data[i] = 55;
////	}
////
////	/************************************* Проверка чтения ************************************************************/
////
////	FM25Q08B_Status_t reception_status, transmission_status, erasion_status;
////	reception_status = FM25Q08B_Read(
////									 SPI2,
////									 FLASH_START_ADDRESS,	// Адрес памяти модуля памяти, с которого начинается чтение
////									 received_data,			// Указатель на массив данных, в который записываются считанные данные
////									 FLASH_PAGE_SIZE		// Количество считанных байт
////										 );
////	if (reception_status != FM25Q08B_OK)  Task_Status = TASK_ERROR;
/////*
////	printf("Проверка чтения\n");
////	for (uint32_t i = 0; i < 10; i++)
////	{
////		printf("received_data[%d] = 0x%02X\n", i, received_data[i]);
////	}
////*/
////	/************************************** Проверка стирания памяти **************************************************/
////
////	erasion_status = FM25Q08B_Chip_Erase(SPI2);
////	if (erasion_status != FM25Q08B_OK)  Task_Status = TASK_ERROR;
////
////	reception_status = FM25Q08B_Read(
////									 SPI2,
////									 FLASH_START_ADDRESS,	// Адрес памяти модуля памяти, с которого начинается чтение
////									 received_data, 		// Указатель на массив данных, в который записываются считанные данные
////									 FLASH_PAGE_SIZE		// Количество считанных байт
////	);
////	if (reception_status != FM25Q08B_OK)  Task_Status = TASK_ERROR;
/////*
////	printf("Проверка стирания памяти\n");
////	for (uint32_t i = 0; i < 10; i++)
////	{
////		printf("received_data[%d] = 0x%02X\n", i, received_data[i]);
////	}
////*/
////	/************************************* Проверка записи и чтения ***************************************************/
////
////	//printf("Проверка записи\n");
////
////	uint8_t transmitted_data_test[FLASH_PAGE_SIZE];
////	uint8_t received_data_test[FLASH_PAGE_SIZE];
////
////	// Одна страница 256 байт (0x100)
////	uint32_t page_addr_offset = 0x100;
////
////	/** В цикле заполняется массив отправляемых данных 256 байт (1 страница),
////	отправляется и считываются данные в массив принятых данных.
////	Каждая следующая страница имеет свой начальный адрес, поэтому в функциях FM25Q08B_Write и FM25Q08B_Read
////	начальный адрес памяти вычисляется по формуле:
////	page_address = flash_start_address + page_number * page_addr_offset
////	где page_address - начальный адрес страницы
////	flash_start_address - начальный адрес памяти (0x000000)
////	page_number - номер текущей страницы (0 - 4095)
////	page_addr_offset - объем одной страницы (256 байт == 0x100)
////	*/
////	for(uint16_t page_number = 0; page_number < 4096; page_number++)
////	{
////		// Заполнение массива отправляемых данных
////		for(uint16_t i = 0; i < FLASH_PAGE_SIZE; i++)
////		{
////			transmitted_data_test[i] = page_number;
////		}
////
////		transmission_status = FM25Q08B_Write(
////											 SPI2,
////											 FLASH_START_ADDRESS + page_addr_offset * page_number,	// Адрес памяти модуля памяти, с которого начинается запись
////											 transmitted_data_test,									// Указатель на массив данных, которые записываются в память
////											 FLASH_PAGE_SIZE										// Количество записанных байт
////		);
////		if (transmission_status != FM25Q08B_OK)  Task_Status = TASK_ERROR;
////
////		reception_status = FM25Q08B_Read(
////										 SPI2,
////										 FLASH_START_ADDRESS + page_addr_offset * page_number,	// Адрес памяти модуля памяти, с которого начинается чтение
////										 received_data_test,										// Указатель на массив данных, в который записываются считанные данные
////										 FLASH_PAGE_SIZE											// Количество считанных байт
////											 );
////		if (reception_status != FM25Q08B_OK)  Task_Status = TASK_ERROR;
/////*
////		printf("Номер страницы: %d\n", page_number);
////		for (uint8_t i = 0; i < 3; i++)
////		{
////			printf("received_data[%d] = 0x%02X\n", i, received_data_test[i]);
////		}
////		printf("\n");
////*/
////	}
////	delay_ms(1000);
////	LED_turnOFF_4_LED();
////
////	// Если есть ошибки - загорится красный светодиод на 1 секунду
////	if (Task_Status != TASK_OK)
////	{
////		GPIO_set_HIGH(GPIOD, 14);
////		delay_ms(1000);
////	}
////	return Task_Status;
////}
