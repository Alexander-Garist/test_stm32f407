/**
  * @file    main.c
  * @brief   Основной файл проекта, содержит проверку работоспособности функций из разных модулей.
  */

/** Includes **********************************************************************************************************/
#include <stdio.h>
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

extern uint32_t USART3_Received_Number;			// число, получаемое по USART3
extern char USART3_rec_char;
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

	/*************** Настройка USART и инициализация портов GPIO в режиме альтернативной функции **********************/

	// Включение модуля USART3 без структуры инициализации
	//USART_Enable(USART3, GPIOB, 10, GPIOB, 11, 115200);	// Включение модуля USART3; PB10 Tx; PB11 Rx; Baudrate 115200

	// Включение модуля USART3 с использованием структуры инициализации

	// Объявление структуры инициализации
	USART_Init_Struct Init_USART3;

	// Заполнение полей
	Init_USART3.USARTx = USART3;
	Init_USART3.GPIO_port_Tx = GPIOB;
	Init_USART3.GPIO_pin_Tx = 10;
	Init_USART3.GPIO_port_Rx = GPIOB;
	Init_USART3.GPIO_pin_Rx = 11;
	Init_USART3.baudrate = 115200;

	USART_Enable_with_struct(&Init_USART3);

	/******************************* Проверка работоспособности USART *************************************************/

	USART_Send_String(USART3, " USART3 connected \r\n");

/*
	for (int i = 0; i < 10; i++)
	{
		USART_Send_Number(USART3, i);
		USART_Send_String(USART3, " Hello world\r\n");
	}
*/


    /**************** Основной цикл: мигание светодиодов и обработка нажатий кнопки ***********************************/

	// включили генератор сигналов, задали параметры (частота, амплитуда, форма сигнала)
	// AD9833_Module_Init(SPI_TypeDef* SPIx, uint32_t frequency, uint8_t amplitude, uint16_t mode);

	// генератор принял по SPI параметры, запомнил FREG0 или FREG1 и форму сигнала, на MCP41010 установлена амплитуда
	// дальше эти параметры будут меняться через USART3 (пришла команда FREQUENCY:AMPLITUDE:WAVE, обработчик принятых данных извлек из нее 3 числа и записал новые параметры сигнала)
	// эти новые параметры сигнала надо отправить генератору по SPI

	char command[10];			// команда это последовательность символов длиной 5 байт
	uint8_t command_index = 0;	// номер байта команды, который сейчас записывается

    while (1)
    {


		while (command_index < 10)	// пока команда не принята полностью
		{
			while (!(USART3->SR & USART_SR_RXNE)){}	// ожидание пока не придет в приемник ОДИН байт

			command[command_index] = USART3->DR;	// запись пришедшего байта
			command_index++;
		}
		command_index = 0;

		// команда уже записана, нужно извлечь числа
		uint32_t freq = 0;
		uint32_t amp = 0;
		uint32_t mode = 0;
		uint8_t divider1_position = 0;	// первый разделитель
		uint8_t divider2_position = 0;	// второй разделитель

		while (command[command_index] != ':')
		{
			command_index++;
		}
		divider1_position = command_index;
		command_index++;
		while (command[command_index] != ':')
		{
			command_index++;
		}
		divider2_position = command_index;

		for (int i = divider1_position; i > 0; i--)
		{

		}



		// отправить команду обратно
		for (int i = 0; i < 10; i++)
		{
			USART_Send_Char(USART3, command[i]);
		}
		USART_Send_Char(USART3, '\r');
		USART_Send_Char(USART3, '\n');









    }
}
