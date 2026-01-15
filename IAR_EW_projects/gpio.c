/**
  * @file    gpio.c
  * @brief   Файл содержит реализации функций GPIO
  */

/** Includes **********************************************************************************************************/
#include "gpio.h"
#include "systick.h"

/******************************************* Статические функции ******************************************************/

// Включение тактирования порта GPIO
static void GPIO_RCC_Enable(GPIO_TypeDef* GPIO_port)
{
    uint32_t Address_Shift = (uint32_t)GPIO_port - (uint32_t)GPIOA;				// Расчет сдвига порта от GPIOA_BASE
    Address_Shift /= 1024;														// Сдвиг каждого порта от GPIOA_BASE составляет 0x0400, в десятичной системе 1024

	// Массив с масками для включения тактирования конкретного порта
    uint32_t RCC_Enable_Mask[] = {
        RCC_AHB1ENR_GPIOAEN,
        RCC_AHB1ENR_GPIOBEN,
        RCC_AHB1ENR_GPIOCEN,
        RCC_AHB1ENR_GPIODEN,
        RCC_AHB1ENR_GPIOEEN,
        RCC_AHB1ENR_GPIOFEN,
        RCC_AHB1ENR_GPIOGEN,
        RCC_AHB1ENR_GPIOHEN,
        RCC_AHB1ENR_GPIOIEN
    };
    RCC->AHB1ENR |= RCC_Enable_Mask[Address_Shift];								// Включение только нужного порта
}

// Инициализация вывода GPIO
static void GPIO_init_OUTPUT(GPIO_TypeDef* GPIO_port, uint8_t GPIO_pin)
{
	// Начальный сброс и установка значения регистра MODER
    GPIO_port->MODER &= ~(MODER_ANALOG << (GPIO_pin * 2));
    GPIO_port->MODER |= (MODER_OUTPUT << (GPIO_pin * 2));

	// Начальный сброс и установка значения регистра OTYPER
    GPIO_port->OTYPER &= ~(OTYPER_OPEN_DRAIN << GPIO_pin);
	GPIO_port->OTYPER |= (OTYPER_PUSH_PULL << GPIO_pin);

	// Начальный сброс и установка значения регистра OSPEEDR
    GPIO_port->OSPEEDR &= ~(OSPEEDR_VERY_HIGH << (GPIO_pin * 2));
    GPIO_port->OSPEEDR |= (OSPEEDR_MEDIUM << (GPIO_pin * 2));

	// Начальный сброс и установка значения регистра PUPDR
    GPIO_port->PUPDR &= ~(PUPDR_RESERVED << (GPIO_pin * 2));
	GPIO_port->PUPDR |= (PUPDR_NO_PUPD << (GPIO_pin * 2));

}

// Инициализация ввода GPIO
static void GPIO_init_INPUT(GPIO_TypeDef* GPIO_port, uint8_t GPIO_pin)
{
	// Начальный сброс и установка значения регистра MODER
	GPIO_port->MODER &= ~(MODER_ANALOG << (GPIO_pin * 2));
    GPIO_port->MODER |= (MODER_INPUT << (GPIO_pin * 2));

	// Начальный сброс и установка значения регистра PUPDR
    GPIO_port->PUPDR &= ~(PUPDR_RESERVED << (GPIO_pin * 2));
    GPIO_port->PUPDR |= (PUPDR_PD << (GPIO_pin * 2));
}

// Инициализация пина в режиме альтернативной функции
static void GPIO_init_AF_Mode(GPIO_TypeDef* GPIO_port, uint8_t GPIO_pin, uint8_t number_AF)
{
	// Начальный сброс и установка значения регистра MODER
    GPIO_port->MODER &= ~(MODER_ANALOG << (GPIO_pin * 2));
    GPIO_port->MODER |= (MODER_AF << (GPIO_pin * 2));

	// Начальный сброс и установка значения регистра AFR
	// AFR[0] настраивает пины 0-7
    if(GPIO_pin <= 7)
    {
        GPIO_port->AFR[0] &= ~(AFR_15 << (GPIO_pin * 4));
        GPIO_port->AFR[0] |= (number_AF << (GPIO_pin * 4));
    }

	// AFR[1] настраивает пины 8-15
    if(GPIO_pin > 7)
    {
        int shifted_pin = GPIO_pin - 8;							//	Сдвиг пинов, т.к. AFR[1] начинается не с 0, а с 8 пина
        GPIO_port->AFR[1] &= ~(AFR_15 << (shifted_pin * 4));
        GPIO_port->AFR[1] |= (number_AF << (shifted_pin * 4));
    }
    delay_ms(10);
}

/**************************************** Глобальные функции **********************************************************/

// Инициализация вывода + установка высокого уровня
void GPIO_set_HIGH(GPIO_TypeDef* GPIO_port, uint8_t GPIO_pin)
{
    GPIO_RCC_Enable(GPIO_port);				// Включение тактирования
    GPIO_init_OUTPUT(GPIO_port, GPIO_pin);	// Инициализация вывода GPIO
    GPIO_port->BSRR = (0x1 << GPIO_pin);	// Установка высокого уровня на выводе
}

// Инициализация вывода + установка низкого уровня
void GPIO_set_LOW(GPIO_TypeDef* GPIO_port, uint8_t GPIO_pin)
{
    GPIO_RCC_Enable(GPIO_port);					// Включение тактирования
    GPIO_init_OUTPUT(GPIO_port, GPIO_pin);		// Инициализация вывода GPIO
    GPIO_port->BSRR = (0x1 << (GPIO_pin + 16));	// Установка низкого уровня на выводе
}

// Инициализация вывода + переключение логического уровня
void GPIO_toggle_Pin(GPIO_TypeDef* GPIO_port, uint8_t GPIO_pin)
{
    GPIO_RCC_Enable(GPIO_port);				// Включение тактирования
    GPIO_init_OUTPUT(GPIO_port, GPIO_pin);	// Инициализация вывода GPIO
    GPIO_port->ODR ^= (0x1 << GPIO_pin);	// Переключение уровня на выводе
}

// Инициализация ввода с подключенной кнопкой
void GPIO_Button_Enable(GPIO_TypeDef* GPIO_port, uint8_t GPIO_pin)
{
    GPIO_RCC_Enable(GPIO_port);					// Включение тактирования
    GPIO_init_INPUT(GPIO_port, GPIO_pin);		// Инициализация ввода GPIO
}

// Инициализация GPIO в режиме I2C
void GPIO_Enable_I2C(GPIO_TypeDef* GPIO_port, uint8_t GPIO_pin)
{
    GPIO_RCC_Enable(GPIO_port);									// Включение тактирования
    GPIO_port->OTYPER |= (OTYPER_OPEN_DRAIN << GPIO_pin);		// Open-drain для I2C
    GPIO_port->OSPEEDR |= (OSPEEDR_VERY_HIGH << GPIO_pin);		// High speed
    GPIO_port->PUPDR &= ~(PUPDR_RESERVED << (GPIO_pin * 2));	// Начальный сброс
    GPIO_port->PUPDR |= (PUPDR_PU << (GPIO_pin * 2));			// Pull-UP
    GPIO_init_AF_Mode(GPIO_port, GPIO_pin, AFR_4);				// Настройка регистров MODER и AFR
}

// Инициализация GPIO в режиме SPI
void GPIO_Enable_SPI(SPI_TypeDef* SPIx, GPIO_TypeDef* GPIO_port, uint8_t GPIO_pin)
{
    GPIO_init_OUTPUT(GPIO_port, GPIO_pin);    // Включение тактирования + инициализация выхода

	// GPIO_init_AF_Mode настроит MODER и AFR
	switch ((uint32_t)SPIx)
	{
		case ((uint32_t)SPI1): GPIO_init_AF_Mode(GPIO_port, GPIO_pin, AFR_5); break;	// Для SPI1 AF5
		case ((uint32_t)SPI2): GPIO_init_AF_Mode(GPIO_port, GPIO_pin, AFR_5); break;	// Для SPI2 AF5
		case ((uint32_t)SPI3): GPIO_init_AF_Mode(GPIO_port, GPIO_pin, AFR_6); break;	// Для SPI3 AF6
	}
}

// Инициализация GPIO в режиме USART/UART
// Важный момент - приемник подтянуть к питанию, чтобы защититься от помех и случайных срабатываний приемника
void GPIO_Enable_USART(USART_TypeDef* USARTx, GPIO_TypeDef* GPIO_port_Tx, uint8_t GPIO_pin_Tx, GPIO_TypeDef* GPIO_port_Rx, uint8_t GPIO_pin_Rx)
{
	// Включение тактирования портов Tx Rx
	GPIO_RCC_Enable(GPIO_port_Tx);
	GPIO_RCC_Enable(GPIO_port_Rx);

	// Настройка пинов Tx Rx в режиме альтернативной функции
	// USART 1/2/3 => AF7
	// UART 4/5 USART6 => AF8
	switch ((uint32_t)USARTx)
	{
		case ((uint32_t)USART1):
		GPIO_init_AF_Mode(GPIO_port_Tx, GPIO_pin_Tx, AFR_7);
		GPIO_init_AF_Mode(GPIO_port_Rx, GPIO_pin_Rx, AFR_7);
		break;

		case ((uint32_t)USART2):
		GPIO_init_AF_Mode(GPIO_port_Tx, GPIO_pin_Tx, AFR_7);
		GPIO_init_AF_Mode(GPIO_port_Rx, GPIO_pin_Rx, AFR_7);
		break;

		case ((uint32_t)USART3):
		GPIO_init_AF_Mode(GPIO_port_Tx, GPIO_pin_Tx, AFR_7);
		GPIO_init_AF_Mode(GPIO_port_Rx, GPIO_pin_Rx, AFR_7);
		break;

		case ((uint32_t)UART4):
		GPIO_init_AF_Mode(GPIO_port_Tx, GPIO_pin_Tx, AFR_8);
		GPIO_init_AF_Mode(GPIO_port_Rx, GPIO_pin_Rx, AFR_8);
		break;

		case ((uint32_t)UART5):
		GPIO_init_AF_Mode(GPIO_port_Tx, GPIO_pin_Tx, AFR_8);
		GPIO_init_AF_Mode(GPIO_port_Rx, GPIO_pin_Rx, AFR_8);
		break;

		case ((uint32_t)USART6):
		GPIO_init_AF_Mode(GPIO_port_Tx, GPIO_pin_Tx, AFR_8);
		GPIO_init_AF_Mode(GPIO_port_Rx, GPIO_pin_Rx, AFR_8);
		break;
	}

	GPIO_port_Rx->PUPDR |= (PUPDR_PU << (GPIO_pin_Rx * 2));
}