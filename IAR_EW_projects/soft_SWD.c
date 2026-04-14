#include "soft_SWD.h"
#include "systick.h"

// Включение тактирования нужного порта GPIO
static void SoftSWD_RCC_Enable()
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
}

// Настройка пинов программного SWD
static void SoftSWD_Pin_Enable()
{
    /** CLK **/
    SOFT_SWD_CLK_PORT->MODER &= ~(MODER_ANALOG << (SOFT_SWD_CLK_PIN * 2));
    SOFT_SWD_CLK_PORT->MODER |= (MODER_OUTPUT << (SOFT_SWD_CLK_PIN * 2));

    SOFT_SWD_CLK_PORT->OTYPER &= ~(OTYPER_OPEN_DRAIN << SOFT_SWD_CLK_PIN);
	SOFT_SWD_CLK_PORT->OTYPER |= (OTYPER_PUSH_PULL << SOFT_SWD_CLK_PIN);

    SOFT_SWD_CLK_PORT->OSPEEDR &= ~(OSPEEDR_VERY_HIGH << (SOFT_SWD_CLK_PIN * 2));
    SOFT_SWD_CLK_PORT->OSPEEDR |= (OSPEEDR_HIGH << (SOFT_SWD_CLK_PIN * 2));

    SOFT_SWD_CLK_PORT->PUPDR &= ~(PUPDR_RESERVED << (SOFT_SWD_CLK_PIN * 2));
	SOFT_SWD_CLK_PORT->PUPDR |= (PUPDR_NO_PUPD << (SOFT_SWD_CLK_PIN * 2));


    /** DATA **/
    SOFT_SWD_DATA_PORT->MODER &= ~(MODER_ANALOG << (SOFT_SWD_DATA_PIN * 2));
    SOFT_SWD_DATA_PORT->MODER |= (MODER_OUTPUT << (SOFT_SWD_DATA_PIN * 2));

    SOFT_SWD_DATA_PORT->OTYPER &= ~(OTYPER_OPEN_DRAIN << SOFT_SWD_DATA_PIN);
	SOFT_SWD_DATA_PORT->OTYPER |= (OTYPER_OPEN_DRAIN << SOFT_SWD_DATA_PIN);

    SOFT_SWD_DATA_PORT->OSPEEDR &= ~(OSPEEDR_VERY_HIGH << (SOFT_SWD_DATA_PIN * 2));
    SOFT_SWD_DATA_PORT->OSPEEDR |= (OSPEEDR_HIGH << (SOFT_SWD_DATA_PIN * 2));

    SOFT_SWD_DATA_PORT->PUPDR &= ~(PUPDR_RESERVED << (SOFT_SWD_DATA_PIN * 2));
	SOFT_SWD_DATA_PORT->PUPDR |= (PUPDR_PU << (SOFT_SWD_DATA_PIN * 2));
}

// Инициализация программного SWD
void SoftSWD_Init()
{
    SoftSWD_RCC_Enable();   // Включить тактирование
    SoftSWD_Pin_Enable();   // Настройка пинов
}

void DWT_Init()
{
    // 1. Разрешаем использование блока трассировки (TRCENA)
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    // 2. Сбрасываем текущее значение счетчика циклов
    DWT->CYCCNT = 0;

    // 3. Запускаем сам счетчик (CYCCNTENA)
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/******************** Ручное управление пинами программного SWD **************/
static void SoftSWD_CLK_High()
{
    SOFT_SWD_CLK_PORT->BSRR = (0x1 << SOFT_SWD_CLK_PIN);
}

static void SoftSWD_CLK_Low()
{
    SOFT_SWD_CLK_PORT->BSRR = (0x1 << (SOFT_SWD_CLK_PIN + 16));
}

static void SoftSWD_DATA_High()
{
    SOFT_SWD_DATA_PORT->BSRR = (0x1 << SOFT_SWD_DATA_PIN);
}

static void SoftSWD_DATA_Low()
{
    SOFT_SWD_DATA_PORT->BSRR = (0x1 << (SOFT_SWD_DATA_PIN + 16));
}

static void SoftSWD_RESET_High()
{
    SOFT_SWD_TARGET_RESET_PORT->BSRR = (0x1 << SOFT_SWD_TARGET_RESET_PIN);
}

static void SoftSWD_RESET_Low()
{
    SOFT_SWD_TARGET_RESET_PORT->BSRR = (0x1 << (SOFT_SWD_TARGET_RESET_PIN + 16));
}

void SoftSWD_Clock_Cycle()
{
    uint32_t start = get_current_SYSTICK_VAL();
    // 1 такт:
    SoftSWD_CLK_High();
          // задержку заменить на неблокирующую!
    SoftSWD_CLK_Low();
}

void SoftSWD_WriteBit(uint8_t sended_bit)
{
    if (sended_bit) SoftSWD_DATA_High();
    else SoftSWD_DATA_Low();
}

void SoftSWD_ReadBit()
{
    SoftSWD_CLK_High();
    // прочитать состояние пина
    SoftSWD_CLK_Low();
}

void SoftSWD_Reset_Target()
{
    SoftSWD_RESET_High();
    delay_ms(20);
    SoftSWD_RESET_Low();
}


/*****************************************************************************/


//GPIO_port->BSRR = (0x1 << GPIO_pin);          // Установка высокого уровня на выводе
//GPIO_port->BSRR = (0x1 << (GPIO_pin + 16));	// Установка низкого уровня на выводе