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


// Задачи получают только период, с которым они должны выполняться

void Task_LED_1(uint32_t TASK_period);	// Задача моргания LED_1
void Task_LED_2(uint32_t TASK_period);	// Задача моргания LED_2
void Task_LED_3(uint32_t TASK_period);	// Задача моргания LED_3
void Task_LED_4(uint32_t TASK_period);	// Задача моргания LED_4

void AD9833_ExecuteCommand_Task(Signal_Parameters* out_signal, uint32_t TASK_period);	// Задача AD9833 по выполнению команд
void AD9833_GetCommand_Task(char* buffer, char* filtered_buffer);						// Задача AD9833 по парсингу команд из буфера USART
void Indicator_Task(uint32_t TASK_period);												// Задача индикатора
void LED_Blink_Task(uint32_t TASK_period);												// Задача LED

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

	// Объявление структуры инициализации USART3 и заполнение ее полей
	USART_Init_Struct Init_USART3;
	Init_USART3.USARTx = USART3;
	Init_USART3.GPIO_port_Tx = GPIOB;
	Init_USART3.GPIO_pin_Tx = 10;
	Init_USART3.GPIO_port_Rx = GPIOB;
	Init_USART3.GPIO_pin_Rx = 11;
	Init_USART3.baudrate = BRR_115200;

	// Включение модуля USART3 с использованием структуры инициализации
	USART_Enable(&Init_USART3);

	/** Настройка модуля AD9833, инициализация используемых портов GPIO ***********************************************/

	/** Схема подключения AD9833 к STM32F407:
		Вывод AD9833	Вывод STM32F407
		CS	----------> PA4 (output)
		FSY ----------> PA3 (output)
		CLK ----------> PA5 (SPI1_SCK)
		DAT ----------> PA7 (SPI1_MOSI)
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

	char BUFFER_USART[MAX_BUFFER_SIZE];				// строка, хранящая еще не отфильтрованный от возможного мусора буфер USART
	char BUFFER_USART_FILTERED[MAX_BUFFER_SIZE];	// отфильтрованный буфер USART

	/** Проверка работоспособности модуля SPI2 (подключен модуль памяти) **********************************************/
    /** Проверка чтения **********/
	/** Проверка стирания памяти */
    /** Проверка записи и чтения */
    /** Проверка работоспособности модуля I2C1 (подключен модуль памяти) **********************************************/

    /**************** Основной цикл: мигание светодиодов и обработка нажатий кнопки ***********************************/
	/** Основной цикл теперь будет содержать не только моргание светодиодами и обработку нажатий кнопки,
		но и прием/передачу команд для генератора сигналов через USART */
	while (1)
	{
		// Пока ничего не пришло по USART выполняются задачи индикатора, LED и AD9833 по выполнению команд
		while (!(USART3->SR & USART_SR_RXNE))
		{
			Indicator_Task(REFRESH_PERIOD);						// Задача индикатора должна выполняться каждые 10 мс
			LED_Blink_Task(Blink_Period);						// Задача моргания LED должна выполняться каждые 100/200/500 мс
			AD9833_ExecuteCommand_Task(&Output_Signal, 100);	// Каждые 100 мс проверяется, есть ли что-то в очереди команд AD9833
		}

		// Если что-то пришло в буфер USART выполнить задачи USART и AD9833 по парсингу новых команд
		USART_Receive(USART3, BUFFER_USART, '!');
		AD9833_GetCommand_Task(BUFFER_USART, BUFFER_USART_FILTERED);
	}
}

/*************************************************************************************************************************/

// Задача AD9833 по выполнению команд из очереди
void AD9833_ExecuteCommand_Task(Signal_Parameters* out_signal, uint32_t TASK_period)
{
	static uint8_t TASK_state;					// Состояние задачи
	static uint32_t TASK_last_execution_time;	// Момент последнего выполнения задачи

	if (is_time_passed_ms(TASK_last_execution_time, TASK_period)) TASK_state = 1;
	else TASK_state = 0;

	switch (TASK_state)
	{
		case 0: return;
		case 1:
		{
			while (Amount_of_Commands)	// выполнить все команды генератора сигналов, какие есть в очереди
			{
				AD9833_Execute_Command(USART3, out_signal);	// после выполнения команды отправить команду обратно отправителю
				AD9833_Module_Init(SPI1, out_signal);
			}
			TASK_last_execution_time = get_current_ms();
		}
	}
}

// Задача AD9833 по парсингу команд из буфера USART
void AD9833_GetCommand_Task(char* buffer, char* filtered_buffer)
{
	// Фильтрация буфера и занесение команд в очередь
	AD9833_filter_Buffer_USART(buffer, filtered_buffer);
	AD9833_Parse_Commands_From_Buffer_USART(filtered_buffer);
}

// Задача индикатора
void Indicator_Task(uint32_t TASK_period)
{
	static uint8_t TASK_state = 0;
	static uint32_t TASK_last_execution_time;
	// если времени прошло достаточно с последнего выполнения этой задачи, то нужно выполнить эту задачу снова
	if (is_time_passed_ms(TASK_last_execution_time, TASK_period)) TASK_state = 1;

	switch (TASK_state)
	{
		case 0: return;
		case 1:
		switch (Blink_Mode)
		{
			case 0: Seven_Segment_Indicate_Number(Green_Blinks_Counter);	break;
			case 1: Seven_Segment_Indicate_Number(123456789);				break;
			case 2: Seven_Segment_Indicate_String("ABCDEFGH1234567890");	break;
		}
		TASK_state = 0;
		TASK_last_execution_time = get_current_ms();
	}
}

// Задача LED
void LED_Blink_Task(uint32_t TASK_period)
{
	static uint8_t Task_State;
	static uint8_t LED_State;
	static uint32_t TASK_last_execution_time;

	// если прошло нужное время после последнего выполнения задачи
	if (is_time_passed_ms(TASK_last_execution_time, TASK_period))
	{
		Task_State = 1;
	}
	else
	{
		Task_State = 0;
	}

	switch (Task_State)
	{
		case 0: return;

		case 1:
		switch (LED_State)
		{
			case 0:
			{
				LED_State = 1;
				switch (Blink_Mode)
				{
					case 0: GPIO_set_HIGH(GPIOD, 12);	Green_Blinks_Counter++;		break;
					case 1: GPIO_set_HIGH(GPIOD, 13);	Orange_Blinks_Counter++;	break;
					case 2: GPIO_set_HIGH(GPIOD, 14);	Red_Blinks_Counter++;		break;
				}
				if ((Green_Blinks_Counter > 999) || (Orange_Blinks_Counter > 999) || (Red_Blinks_Counter > 999))
				{
					Green_Blinks_Counter = 0;
					Orange_Blinks_Counter = 0;
					Red_Blinks_Counter = 0;
				}
				break;
			}

			case 1:
			{
				LED_State = 0;
				LED_turnOFF_4_LED();
				break;
			}

		}
		TASK_last_execution_time = get_current_ms();
	}
}

void Task_LED_1(uint32_t TASK_period)
{
	static uint8_t Task_State;
	static uint8_t LED_State;
	static uint32_t TASK_last_execution_time;
	static uint32_t counter;

	// если прошло нужное время после последнего выполнения задачи
	if (is_time_passed_ms(TASK_last_execution_time, TASK_period))
	{
		Task_State = 1;
	}
	else
	{
		Task_State = 0;
	}

	switch (Task_State)
	{
		case 0: return;

		case 1:
		switch (LED_State)
		{
			case 0:
			{
				LED_State = 1;
				GPIO_set_HIGH(GPIOD, 12);
				counter++;
				//printf("LED1: %d\n", counter);
				break;
			}

			case 1:
			{
				LED_State = 0;
				GPIO_set_LOW(GPIOD, 12);
				break;
			}
		}
		TASK_last_execution_time = get_current_ms();
	}
}

void Task_LED_2(uint32_t TASK_period)
{
	static uint8_t Task_State;
	static uint8_t LED_State;
	static uint32_t TASK_last_execution_time;
	static uint32_t counter;

	// если прошло нужное время после последнего выполнения задачи
	if (is_time_passed_ms(TASK_last_execution_time, TASK_period))
	{
		Task_State = 1;
	}
	else
	{
		Task_State = 0;
	}

	switch (Task_State)
	{
		case 0: return;

		case 1:
		switch (LED_State)
		{
			case 0:
			{
				LED_State = 1;
				GPIO_set_HIGH(GPIOD, 13);
				counter++;
				//printf("LED2: %d\n", counter);
				break;
			}

			case 1:
			{
				LED_State = 0;
				GPIO_set_LOW(GPIOD, 13);
				break;
			}
		}
		TASK_last_execution_time = get_current_ms();
	}
}

void Task_LED_3(uint32_t TASK_period)
{
	static uint8_t Task_State;
	static uint8_t LED_State;
	static uint32_t TASK_last_execution_time;
	static uint32_t counter;

	// если прошло нужное время после последнего выполнения задачи
	if (is_time_passed_ms(TASK_last_execution_time, TASK_period))
	{
		Task_State = 1;
	}
	else
	{
		Task_State = 0;
	}

	switch (Task_State)
	{
		case 0: return;

		case 1:
		switch (LED_State)
		{
			case 0:
			{
				LED_State = 1;
				GPIO_set_HIGH(GPIOD, 14);
				counter++;
				//printf("LED3: %d\n", counter);
				break;
			}

			case 1:
			{
				LED_State = 0;
				GPIO_set_LOW(GPIOD, 14);
				break;
			}
		}
		TASK_last_execution_time = get_current_ms();
	}
}

void Task_LED_4(uint32_t TASK_period)
{
	static uint8_t Task_State;
	static uint8_t LED_State;
	static uint32_t TASK_last_execution_time;
	static uint32_t counter;

	// если прошло нужное время после последнего выполнения задачи
	if (is_time_passed_ms(TASK_last_execution_time, TASK_period))
	{
		Task_State = 1;
	}
	else
	{
		Task_State = 0;
	}

	switch (Task_State)
	{
		case 0: return;

		case 1:
		switch (LED_State)
		{
			case 0:
			{
				LED_State = 1;
				GPIO_set_HIGH(GPIOD, 15);
				counter++;
				//printf("LED4: %d\n", counter);
				break;
			}

			case 1:
			{
				LED_State = 0;
				GPIO_set_LOW(GPIOD, 15);
				break;
			}
		}
		TASK_last_execution_time = get_current_ms();
	}
}