#include "gpio.h"
#include "systick.h"

//Статические функции, нужны для сокрытия их реализации в других модулях
static void GPIO_RCC_Enable(GPIO_TypeDef* port)                             //Функция включения тактирования конкретного порта GPIO
{
    uint32_t Address_Shift = (uint32_t)port - (uint32_t)GPIOA;      //Расчет сдвига порта от GPIOA_BASE
    Address_Shift /= 1024;                                          //Сдвиг каждого порта от GPIOA_BASE составляет 0x0400
                                                                    //В десятичной системе это 1024
    uint32_t RCC_Enable_Mask[] = {                          //Массив с масками для включения тактирования конкретного порта
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
    RCC->AHB1ENR |= RCC_Enable_Mask[Address_Shift];         //Включение только нужного порта
}

static void GPIO_init_OUTPUT(GPIO_TypeDef* port, int pin)                   //Инициализация GPIO выхода
{
    port->MODER &= ~(0x3 << (pin * 2));             //MODER 00 INPUT    MODER 10 AF mode
    port->MODER |= (0x1 << (pin * 2));              //MODER 01 OUTPUT   MODER 11 Analog

    port->OTYPER &= ~(0x1 << pin);                  //OTYPER 0 push-pull 1 open-drain

    port->OSPEEDR &= ~(0x3 << (pin * 2));           //OSPEEDR 00 LOW  01 MEDIUM  10 HIGH 11 VERY HIGH
    port->OSPEEDR |= (0x1 << (pin * 2));

    port->PUPDR &= ~(0x3 << (pin * 2));             //PUPDR 00 NO pull-up, NO pull-down, 01 pull-UP, 10 pull-DOWN
}

static void GPIO_init_INPUT(GPIO_TypeDef* port, int pin)                    //Инициализация GPIO входа
{
    port->MODER &= ~(0x1 << (pin * 2));             //MODER 00 INPUT

    port->PUPDR &= ~(0x3 << (pin * 2));             //PUPDR 10 pull-DOWN
    port->PUPDR |= (0x2 << (pin * 2));
}

static void GPIO_init_AF_Mode(GPIO_TypeDef* port, int pin, int number_AF)   //Инициализация порта GPIO в режиме альтернативной функции, настройка MODER и AFR, остальные регистры нужно настраивать отдельно
{                                                                           //конкретно для I2C нужна AF 4, поэтому в AFR[x] нужно поместить 0100
    port->MODER &= ~(0x3 << (pin * 2));     //Сначала сброс
    port->MODER |= (0x2 << (pin * 2));      //MODER 0x0 INPUT    0x1 OUTPUT   0x2 AF mode  0x3 Analog

    if(pin <= 7)    //AFRLx пины 0-7
    {
        port->AFR[0] &= ~(0xF << (pin * 4));            //Сначала обнулить
        port->AFR[0] |= (number_AF << (pin * 4));       //Теперь установить нужные биты AFR (0100 для I2C)
    }
    if(pin > 7)     //AFRHx пины 8-15
    {
        int shifted_pin = pin - 8;                          //Сдвиг пинов, т.к. AFRH начинается не с 0, а с 8 пина
        port->AFR[1] &= ~(0xF << (shifted_pin * 4));        //Сначала обнулить
        port->AFR[1] |= (number_AF << (shifted_pin * 4));   //Теперь установить нужные биты AFR (0100 для I2C)
    }
    delay_ms(10);
}


//Функции для использования в других модулях
//OUTPUT
void GPIO_set_HIGH(GPIO_TypeDef* port, int pin)                             //Установить высокий уровень на выходе GPIO
{
    GPIO_RCC_Enable(port);
    GPIO_init_OUTPUT(port, pin);

    port->BSRR = (1 << pin);                       //BSRRL  1 на выходе высокий уровень
}
void GPIO_set_LOW(GPIO_TypeDef* port, int pin)                              //Установить низкий уровень на выходе GPIO
{
    GPIO_RCC_Enable(port);
    GPIO_init_OUTPUT(port, pin);

    port->BSRR = (1 << (pin + 16));                       //BSRRH - 1 на выходе низкий уровень
}
void GPIO_toggle_Pin(GPIO_TypeDef* port, int pin)                           //Переключить состояние вывода GPIO (HIGH/LOW)
{
    GPIO_RCC_Enable(port);
    GPIO_init_OUTPUT(port, pin);

    port->ODR ^= (1 << pin);
}

//INPUT
void GPIO_Button_Enable(GPIO_TypeDef* port, int pin)                            //Определить пин как вход, к которому подключена кнопка
{
    GPIO_RCC_Enable(port);
    GPIO_init_INPUT(port, pin);
}

//ALTERNATE FUNCTION MODE
void GPIO_Enable_I2C(GPIO_TypeDef* port, int pin)                               //Включить пин в режиме альтернативной функции I2C
{
    GPIO_RCC_Enable(port);

    port->OTYPER |= (0x1 << pin);           //open-drain для I2C
    port->OSPEEDR |= (0x3 << pin);          //high speed

    port->PUPDR &= ~(0x3 << (pin * 2));     //Сначала сброс
    port->PUPDR |= (0x1 << (pin * 2));      //pull-up

    GPIO_init_AF_Mode(port, pin, 4);        //MODER и AFR
}
void GPIO_Enable_SPI(SPI_TypeDef* SPIx, GPIO_TypeDef* port, int pin)            //Включить пин в режиме альтернативной функции SPI
{
    GPIO_init_OUTPUT(port, pin);    //включение тактирования + настройка регистров как для выхода
    //MODER     0x1
    //OTYPER    0x0
    //OSPEEDR   0x1
    //PUPDR     0x0

    //далее GPIO_init_AF_Mode настроит MODER и AFR
    if(SPIx == SPI1) GPIO_init_AF_Mode(port, pin, 5);   //для SPI1 AF5
    if(SPIx == SPI2) GPIO_init_AF_Mode(port, pin, 5);   //для SPI2 AF5
    if(SPIx == SPI3) GPIO_init_AF_Mode(port, pin, 6);   //для SPI3 AF6
}


