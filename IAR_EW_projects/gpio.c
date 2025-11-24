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
static void GPIO_init_OUTPUT(GPIO_TypeDef* GPIO_port, int GPIO_pin)
{
    GPIO_port->MODER &= ~(0x3 << (GPIO_pin * 2));				// Начальный сброс
    GPIO_port->MODER |= (0x1 << (GPIO_pin * 2));				// MODER 01 => OUTPUT
    GPIO_port->OTYPER &= ~(0x1 << GPIO_pin);					// OTYPER 0 => push-pull
    GPIO_port->OSPEEDR &= ~(0x3 << (GPIO_pin * 2));				// Начальный сброс
    GPIO_port->OSPEEDR |= (0x1 << (GPIO_pin * 2));				// OSPEED 01 => medium speed
    GPIO_port->PUPDR &= ~(0x3 << (GPIO_pin * 2));				// PUPDR 00 => NO pull-up NO pull-down
}

// Инициализация ввода GPIO
static void GPIO_init_INPUT(GPIO_TypeDef* GPIO_port, int GPIO_pin)
{
    GPIO_port->MODER &= ~(0x1 << (GPIO_pin * 2));             	// MODER 00 => INPUT
    GPIO_port->PUPDR &= ~(0x3 << (GPIO_pin * 2));				// Начальный сброс
    GPIO_port->PUPDR |= (0x2 << (GPIO_pin * 2));				// PUPDR 10 pull-DOWN
}

// Инициализация пина в режиме альтернативной функции
static void GPIO_init_AF_Mode(GPIO_TypeDef* GPIO_port, int GPIO_pin, int number_AF)
{
    GPIO_port->MODER &= ~(0x3 << (GPIO_pin * 2));     // Начальный сброс
    GPIO_port->MODER |= (0x2 << (GPIO_pin * 2));      // MODER 0x2 => AF mode

    if(GPIO_pin <= 7)    // AFR[0] настраивает пины 0-7
    {
        GPIO_port->AFR[0] &= ~(0xF << (GPIO_pin * 4));            // Начальный сброс
        GPIO_port->AFR[0] |= (number_AF << (GPIO_pin * 4));       // Установка выбранной альтернативной функции
    }
    if(GPIO_pin > 7)     // AFR[1] настраивает пины 8-15
    {
        int shifted_pin = GPIO_pin - 8;                          //	Сдвиг пинов, т.к. AFR[1] начинается не с 0, а с 8 пина
        GPIO_port->AFR[1] &= ~(0xF << (shifted_pin * 4));        // Начальный сброс
        GPIO_port->AFR[1] |= (number_AF << (shifted_pin * 4));   // Установка выбранной альтернативной функции
    }
    delay_ms(10);
}

/**************************************** Глобальные функции **********************************************************/

// Инициализация вывода + установка высокого уровня
void GPIO_set_HIGH(GPIO_TypeDef* GPIO_port, int GPIO_pin)
{
    GPIO_RCC_Enable(GPIO_port);				// Включение тактирования
    GPIO_init_OUTPUT(GPIO_port, GPIO_pin);	// Инициализация вывода GPIO
    GPIO_port->BSRR = (1 << GPIO_pin);		// Установка высокого уровня на выводе
}

// Инициализация вывода + установка низкого уровня
void GPIO_set_LOW(GPIO_TypeDef* GPIO_port, int GPIO_pin)
{
    GPIO_RCC_Enable(GPIO_port);					// Включение тактирования
    GPIO_init_OUTPUT(GPIO_port, GPIO_pin);		// Инициализация вывода GPIO
    GPIO_port->BSRR = (1 << (GPIO_pin + 16));	// Установка низкого уровня на выводе
}

// Инициализация вывода + переключение логического уровня
void GPIO_toggle_Pin(GPIO_TypeDef* GPIO_port, int GPIO_pin)
{
    GPIO_RCC_Enable(GPIO_port);					// Включение тактирования
    GPIO_init_OUTPUT(GPIO_port, GPIO_pin);		// Инициализация вывода GPIO
    GPIO_port->ODR ^= (1 << GPIO_pin);			// Переключение уровня на выводе
}

// Инициализация ввода
void GPIO_Button_Enable(GPIO_TypeDef* GPIO_port, int GPIO_pin)
{
    GPIO_RCC_Enable(GPIO_port);					// Включение тактирования
    GPIO_init_INPUT(GPIO_port, GPIO_pin);		// Инициализация ввода GPIO
}

// Инициализация GPIO в режиме I2C
void GPIO_Enable_I2C(GPIO_TypeDef* GPIO_port, int GPIO_pin)
{
    GPIO_RCC_Enable(GPIO_port);						// Включение тактирования
    GPIO_port->OTYPER |= (0x1 << GPIO_pin);			// Open-drain для I2C
    GPIO_port->OSPEEDR |= (0x3 << GPIO_pin);		// High speed
    GPIO_port->PUPDR &= ~(0x3 << (GPIO_pin * 2));	// Начальный сброс
    GPIO_port->PUPDR |= (0x1 << (GPIO_pin * 2));	// Pull-UP
    GPIO_init_AF_Mode(GPIO_port, GPIO_pin, 4);		// Настройка регистров MODER и AFR
}

// Инициализация GPIO в режиме SPI
void GPIO_Enable_SPI(SPI_TypeDef* SPIx, GPIO_TypeDef* GPIO_port, int GPIO_pin)
{
    GPIO_init_OUTPUT(GPIO_port, GPIO_pin);    // Включение тактирования + инициализация выхода

    // GPIO_init_AF_Mode настроит MODER и AFR
    if(SPIx == SPI1) GPIO_init_AF_Mode(GPIO_port, GPIO_pin, 5);   // Для SPI1 AF5
    if(SPIx == SPI2) GPIO_init_AF_Mode(GPIO_port, GPIO_pin, 5);   // Для SPI2 AF5
    if(SPIx == SPI3) GPIO_init_AF_Mode(GPIO_port, GPIO_pin, 6);   // Для SPI3 AF6
}