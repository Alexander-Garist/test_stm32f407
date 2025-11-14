/**
  * @file    gpio.c
  * @brief   Файл содержит реализации функций GPIO
  */

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include "systick.h"

/********************** Статические функции ***********************************/

/**
	! Статическая функция GPIO_RCC_Enable включает тактирование выбранного порта
		GPIO.
	- port - порт GPIO
*/
static void GPIO_RCC_Enable(GPIO_TypeDef* port)
{
    uint32_t Address_Shift = (uint32_t)port - (uint32_t)GPIOA;					// Расчет сдвига порта от GPIOA_BASE
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

/**
	! Статическая функция GPIO_init_OUTPUT инициализирует выбранный пин GPIO как
		вывод.
	- port - порт GPIO
	- pin - пин GPIO
*/
static void GPIO_init_OUTPUT(GPIO_TypeDef* port, int pin)
{
    port->MODER &= ~(0x3 << (pin * 2));				// Начальный сброс
    port->MODER |= (0x1 << (pin * 2));				// MODER 01 => OUTPUT

    port->OTYPER &= ~(0x1 << pin);					// OTYPER 0 => push-pull

    port->OSPEEDR &= ~(0x3 << (pin * 2));			// Начальный сброс
    port->OSPEEDR |= (0x1 << (pin * 2));			// OSPEED 01 => medium speed

    port->PUPDR &= ~(0x3 << (pin * 2));				// PUPDR 00 => NO pull-up NO pull-down
}

/**
	! Статическая функция GPIO_init_INPUT инициализирует выбранный пин GPIO как
		ввод.
	- port - порт GPIO
	- pin - пин GPIO
*/
static void GPIO_init_INPUT(GPIO_TypeDef* port, int pin)
{
    port->MODER &= ~(0x1 << (pin * 2));             // MODER 00 => INPUT

    port->PUPDR &= ~(0x3 << (pin * 2));				// Начальный сброс
    port->PUPDR |= (0x2 << (pin * 2));				// PUPDR 10 pull-DOWN
}

/**
	! Статическая функция GPIO_init_AF_Mode инициализирует выбранный пин GPIO
		в режиме альтернативной функции number_AF и настраивает регистры MODER
		и AFR.
	- port - порт GPIO
	- pin - пин GPIO
	- number_AF - номер альтернативной функции
*/
static void GPIO_init_AF_Mode(GPIO_TypeDef* port, int pin, int number_AF)
{
    port->MODER &= ~(0x3 << (pin * 2));     // Начальный сброс
    port->MODER |= (0x2 << (pin * 2));      // MODER 0x2 => AF mode

    if(pin <= 7)    // AFR[0] настраивает пины 0-7
    {
        port->AFR[0] &= ~(0xF << (pin * 4));            // Начальный сброс
        port->AFR[0] |= (number_AF << (pin * 4));       // Установка выбранной альтернативной функции
    }
    if(pin > 7)     // AFR[1] настраивает пины 8-15
    {
        int shifted_pin = pin - 8;                          //	Сдвиг пинов, т.к. AFR[1] начинается не с 0, а с 8 пина
        port->AFR[1] &= ~(0xF << (shifted_pin * 4));        // Начальный сброс
        port->AFR[1] |= (number_AF << (shifted_pin * 4));   // Установка выбранной альтернативной функции
    }
    delay_ms(10);
}


/********************** Глобальные функции ************************************/

/**************** Инициализация GPIO как вывода *******************************/
void GPIO_set_HIGH(GPIO_TypeDef* port, int pin)
{
    GPIO_RCC_Enable(port);			// Включение тактирования
    GPIO_init_OUTPUT(port, pin);	// Инициализация вывода GPIO

    port->BSRR = (1 << pin);		// Установка высокого уровня на выводе
}

void GPIO_set_LOW(GPIO_TypeDef* port, int pin)
{
    GPIO_RCC_Enable(port);			// Включение тактирования
    GPIO_init_OUTPUT(port, pin);	// Инициализация вывода GPIO

    port->BSRR = (1 << (pin + 16));	// Установка низкого уровня на выводе
}

void GPIO_toggle_Pin(GPIO_TypeDef* port, int pin)
{
    GPIO_RCC_Enable(port);			// Включение тактирования
    GPIO_init_OUTPUT(port, pin);	// Инициализация вывода GPIO

    port->ODR ^= (1 << pin);		// Переключение уровня на выводе
}

/**************** Инициализация GPIO как ввода ********************************/
void GPIO_Button_Enable(GPIO_TypeDef* port, int pin)
{
    GPIO_RCC_Enable(port);			// Включение тактирования
    GPIO_init_INPUT(port, pin);		// Инициализация ввода GPIO
}

/************ Инициализация GPIO в режиме альтернативной функции **************/
void GPIO_Enable_I2C(GPIO_TypeDef* port, int pin)
{
    GPIO_RCC_Enable(port);					// Включение тактирования

    port->OTYPER |= (0x1 << pin);           // Open-drain для I2C
    port->OSPEEDR |= (0x3 << pin);          // High speed

    port->PUPDR &= ~(0x3 << (pin * 2));     // Начальный сброс
    port->PUPDR |= (0x1 << (pin * 2));      // Pull-UP

    GPIO_init_AF_Mode(port, pin, 4);        // Настройка регистров MODER и AFR
}

void GPIO_Enable_SPI(SPI_TypeDef* SPIx, GPIO_TypeDef* port, int pin)
{
    GPIO_init_OUTPUT(port, pin);    // Включение тактирования + инициализация выхода

    // GPIO_init_AF_Mode настроит MODER и AFR
    if(SPIx == SPI1) GPIO_init_AF_Mode(port, pin, 5);   // Для SPI1 AF5
    if(SPIx == SPI2) GPIO_init_AF_Mode(port, pin, 5);   // Для SPI2 AF5
    if(SPIx == SPI3) GPIO_init_AF_Mode(port, pin, 6);   // Для SPI3 AF6
}


