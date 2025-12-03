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

/** Defines ***********************************************************************************************************/

/** Период моргания светодиодов в 3 режимах ***************************************************************************/
#define TIME_SHORT      100
#define TIME_MEDIUM     300
#define TIME_LONG       500

/** Триггер внешнего прерывания и задержка для фильтрации дребезга при нажатии кнопки *********************************/
#define BUTTON_TRIGGER  EXTI_TRIGGER_FALLING
#define DEBOUNCE_TIME   100

/** Defines для проверки I2C ******************************************************************************************/
#define EEPROM_ADDRESS						0x50							// 7-битный адрес подключенного модуля EEPROM, пины A0, A1 подтянуты к земле 1010000
#define START_ADDRESS						0x00							// Адрес в памяти EEPROM, куда записываются данные
#define length_All_Data						0x20000							// Суммарное количество отправленных байт в память EEPROM 131072
#define length_Transmitted_Data				4096							// Максимальное количество отправленных байт за 1 операцию отправки (16 страниц по 256 байт)
#define length_Received_Data				4096							// Количество считанных байт из памяти EEPROM
#define MAX_NUMBER_ATTEMPTS_CONNECT_EEPROM	10								// Максимум попыток подключиться к EEPROM за один раз
#define data_2_byte_SIZE					(length_Transmitted_Data / 2)   // Размер массива 2-байтных данных
#define send_SIZE							512								// Размер массива данных, заполненного буквами

/** Переменные ********************************************************************************************************/
static uint32_t time_delay = 100;				// Период моргания светодиодов в основном цикле while в тиках systick_counter

//Выбор режима моргания
volatile static uint32_t blink_mode = 0;		// 0 - моргает зеленый с коротким периодом
												// 1 - моргает оранжевый со средним периодом
												// 2 - моргает красный с длинным периодом

uint8_t button_state = 0;						// Состояние кнопки нажата/не нажата
uint32_t button_last_time = 0;					// Время последнего нажатия
uint16_t data_2_byte[data_2_byte_SIZE];         // Массив двухбайтных данных 2048 чисел = 4096 байт
extern uint32_t Amount_of_Commands;				// Количество команд для генератора сигналов, полученных из USART. Переменная объявлена в AD9833.c

/** Функции ***********************************************************************************************************/

	/**
	! callback функция для установки частоты моргания.
	- time - требуемый период моргания светоиодов.
	*/
void set_time_delay(uint32_t time)
{
    time_delay = time;
}

	/**
	! Функция изменения режима моргания, вызывается в обработчике внешнего прерывания EXTI0_IRQHandler.
	- указатель на callback функцию.
*/
void change_blink_mode(void (*callback)(uint32_t))
{
    blink_mode++;									// Нажатие кнопки переключает режим моргания на следующий
    if (blink_mode > 2) blink_mode %= 3;			// Защита от переполнения количества режимов моргания

    if (blink_mode == 0) callback(TIME_SHORT);		// Эквивалентно time_delay = TIME_SHORT;
    if (blink_mode == 1) callback(TIME_MEDIUM);		// Эквивалентно time_delay = TIME_MEDIUM;
    if (blink_mode == 2) callback(TIME_LONG);		// Эквивалентно time_delay = TIME_LONG;
}

	/**
	! Функция включения всех 4 пользовательских светодиодов на плате: PD12, PD13, PD14, PD15.
	*/
void LED_turnON_4_LED(void)
{
    GPIO_set_HIGH(GPIOD, 12);
    GPIO_set_HIGH(GPIOD, 13);
    GPIO_set_HIGH(GPIOD, 14);
    GPIO_set_HIGH(GPIOD, 15);
}

	/**
	! Функция выключения всех 4 пользовательских светодиодов на плате: PD12, PD13, PD14, PD15.
	*/
void LED_turnOFF_4_LED(void)
{
    GPIO_set_LOW(GPIOD, 12);
    GPIO_set_LOW(GPIOD, 13);
    GPIO_set_LOW(GPIOD, 14);
    GPIO_set_LOW(GPIOD, 15);
}

	/**
	! Обработчик внешнего прерывания (нажатия пользовательской кнопки на выводе PA0).
	*/
void EXTI0_IRQHandler(void)
{
    GPIO_set_HIGH(GPIOD, 15);								// Индикация нажатия кнопки синим светодиодом
	delay_ms(100);
    uint32_t current_time = get_current_ms();				// Момент времени начала обработки прерывания
    if (current_time - button_last_time > DEBOUNCE_TIME)
    {
        change_blink_mode(set_time_delay);					// Логика нажатия на кнопку - смена режима
        button_last_time = get_current_ms();				// Обновить время последнего нажатия
    }
    EXTI_Clear_Flag(0);										// Сброс флага для выхода из прерывания
    LED_turnOFF_4_LED();									// Выключение всех светодиодов
}

int main()
{
    /** Инициализация портов GPIO на вход и выход, включение обработки внешних прерываний и системного таймера для неблокирующих задержек */

    GPIO_Button_Enable(GPIOA, 0);                       // Определение порта PA0 как вход с кнопкой
    EXTI_Enable_Pin(EXTI_PortA, 0, BUTTON_TRIGGER);     // Включить внешние прерывания для этого пина
    LED_turnOFF_4_LED();                                // В начальный момент времени все светодиоды выключены
    SysTick_Init();										// Включение SysTick

	/************** Настройка I2C и инициализация портов GPIO в режиме альтернативной функции *************************/

    GPIO_Enable_I2C(GPIOB, 7);		// Определение PB7 как SDA
    GPIO_Enable_I2C(GPIOB, 6);		// Определение PB6 как SCL
    I2C_Enable(I2C1);				// Включение модуля I2C1

	/************** Настройка SPI и инициализация портов GPIO в режиме альтернативной функции *************************/

    GPIO_set_HIGH(GPIOB, 12);								// Определение PB12 как OUTPUT
    GPIO_Enable_SPI(SPI2, SPI2_SCK_PORT, SPI2_SCK_PIN);     // Определение PB13 как SPI2_SCK
    GPIO_Enable_SPI(SPI2, SPI2_MISO_PORT, SPI2_MISO_PIN);   // Определение PB14 как SPI2_MISO
    GPIO_Enable_SPI(SPI2, SPI2_MOSI_PORT, SPI2_MOSI_PIN);   // Определение PB15 как SPI2_MOSI

	// Включение модуля SPI2
    SPI_Enable(SPI2);
    FM25Q08B_Reset(SPI2);

	/*************** Настройка USART и инициализация портов GPIO в режиме альтернативной функции **********************/

	// Включение модуля USART3 без структуры инициализации
	//USART_Enable(USART3, GPIOB, 10, GPIOB, 11, 115200);	// Включение модуля USART3; PB10 Tx; PB11 Rx; Baudrate 115200

	// Включение модуля USART3 с использованием структуры инициализации
	USART_Init_Struct Init_USART3;

	// Заполнение полей
	Init_USART3.USARTx = USART3;
	Init_USART3.GPIO_port_Tx = GPIOB;
	Init_USART3.GPIO_pin_Tx = 10;
	Init_USART3.GPIO_port_Rx = GPIOB;
	Init_USART3.GPIO_pin_Rx = 11;
	Init_USART3.baudrate = 115200;

	USART_Enable(&Init_USART3);


		/** Схема подключения AD9833 к STM32F407:
		CS	-> оранжевый	-> PA4
		FSY -> белый		-> PA3
		CLK -> черный		-> PA5 (SPI1_SCK)
		DAT -> фиолетовый	-> PA7 (SPI1_MOSI)
	*/

	GPIO_Enable_SPI(SPI1, GPIOA, 5);	// Определение PA5 как SPI1_SCK
	GPIO_Enable_SPI(SPI1, GPIOA, 7);	// Определение PA7 как SPI1_MOSI

	GPIO_set_HIGH(GPIOA, 3);	// PA3 -> FSY
	GPIO_set_HIGH(GPIOA, 4);	// PA4 -> CS

	SPI_Init_Mode_2(SPI1);		// Включение SPI1 (Mode 2: CPOL 1 CPHA 0)
	AD9833_Reset(SPI1);			// Сброс генератора



	/******************************* Проверка работоспособности USART *************************************************/

	USART_Send_String(USART3, "USART3 connected \r\n");

	// Создание экземпляра выходного сигнала, инициализация начальных параметров
	Signal_Parameters Output_Signal;
	Output_Signal.frequency = 1000;
	Output_Signal.amplitude = 128;
	Output_Signal.wave_form = 0;

	// Включение генератора AD9833, выходной сигнал имеет параметры 1 кГц, 50% амплитуда, синусоида
	AD9833_Module_Init(SPI1, &Output_Signal);

	char BUFFER_USART[MAX_BUFFER_SIZE];				// строка, хранящая буфер USART, т.е. еще не отфильтрованный от мусора
	uint32_t received_bytes = 0;					// количество принятых байт
	char BUFFER_USART_FILTERED[MAX_BUFFER_SIZE];	// отфильтрованный буфер USART

    /**************** Основной цикл: мигание светодиодов и обработка нажатий кнопки ***********************************/
	/** Основной цикл теперь будет содержать не только моргание светодиодами и обработку нажатий кнопки, но и прием/передачу команд для генератора сигналов через USART */

    while (1)
    {
		// пока флаг RXNE не поднят это означает что ничего еще не пришло по USART, значит проверяем есть ли команды в очереди
		while (!(USART3->SR & USART_SR_RXNE))
		{
			while (Amount_of_Commands)	// выполнить все команды, какие есть в очереди
			{
				AD9833_Execute_Command(USART3, &Output_Signal);	// после выполнения команды отправить команду обратно отправителю
				AD9833_Module_Init(SPI1, &Output_Signal);
				//AD9833_print_Signal_Parameters(&Output_Signal);
			}
		}

		// прием буфера без фильтрации
		received_bytes = 0;
		while (received_bytes < MAX_BUFFER_SIZE)
		{
			while (!(USART3->SR & USART_SR_RXNE)){}				// ожидание пока не придет в приемник ОДИН байт
			BUFFER_USART[received_bytes] = USART3->DR;			// запись пришедшего байта в буфер
			if (BUFFER_USART[received_bytes] == '!') break;		// конечный символ на случай если нужно принять менее 256 байт
			received_bytes++;
		}

		// фильтрация буфера и занесение команд в очередь
		AD9833_filter_Buffer_USART(BUFFER_USART, BUFFER_USART_FILTERED);
		AD9833_Parse_Commands_From_Buffer_USART(BUFFER_USART_FILTERED);
    }
}
