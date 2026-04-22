#include "soft_SWD.h"
#include "systick.h"

// Текущее состояние мастера (настроен на вход или выход)
SoftSWD_Direction Master_Direction = Master_Output;

// Включение тактирования нужного порта GPIO
static void SoftSWD_RCC_Enable()
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN;
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
    SOFT_SWD_CLK_PORT->OSPEEDR |= (OSPEEDR_VERY_HIGH << (SOFT_SWD_CLK_PIN * 2));

    SOFT_SWD_CLK_PORT->PUPDR &= ~(PUPDR_RESERVED << (SOFT_SWD_CLK_PIN * 2));
	SOFT_SWD_CLK_PORT->PUPDR |= (PUPDR_NO_PUPD << (SOFT_SWD_CLK_PIN * 2));


    /** DATA **/
    SOFT_SWD_DATA_PORT->MODER &= ~(MODER_ANALOG << (SOFT_SWD_DATA_PIN * 2));
    SOFT_SWD_DATA_PORT->MODER |= (MODER_OUTPUT << (SOFT_SWD_DATA_PIN * 2));

    SOFT_SWD_DATA_PORT->OTYPER &= ~(OTYPER_OPEN_DRAIN << SOFT_SWD_DATA_PIN);
	SOFT_SWD_DATA_PORT->OTYPER |= (OTYPER_PUSH_PULL << SOFT_SWD_DATA_PIN);

    SOFT_SWD_DATA_PORT->OSPEEDR &= ~(OSPEEDR_VERY_HIGH << (SOFT_SWD_DATA_PIN * 2));
    SOFT_SWD_DATA_PORT->OSPEEDR |= (OSPEEDR_VERY_HIGH << (SOFT_SWD_DATA_PIN * 2));

    SOFT_SWD_DATA_PORT->PUPDR &= ~(PUPDR_RESERVED << (SOFT_SWD_DATA_PIN * 2));
	SOFT_SWD_DATA_PORT->PUPDR |= (PUPDR_PU << (SOFT_SWD_DATA_PIN * 2));


    /** RESET **/
    SOFT_SWD_TARGET_RESET_PORT->MODER &= ~(MODER_ANALOG << (SOFT_SWD_TARGET_RESET_PIN * 2));
    SOFT_SWD_TARGET_RESET_PORT->MODER |= (MODER_OUTPUT << (SOFT_SWD_TARGET_RESET_PIN * 2));

    SOFT_SWD_TARGET_RESET_PORT->OTYPER &= ~(OTYPER_OPEN_DRAIN << SOFT_SWD_TARGET_RESET_PIN);
	SOFT_SWD_TARGET_RESET_PORT->OTYPER |= (OTYPER_OPEN_DRAIN << SOFT_SWD_TARGET_RESET_PIN);

    SOFT_SWD_TARGET_RESET_PORT->PUPDR &= ~(PUPDR_RESERVED << (SOFT_SWD_TARGET_RESET_PIN * 2));
	SOFT_SWD_TARGET_RESET_PORT->PUPDR |= (PUPDR_PU << (SOFT_SWD_TARGET_RESET_PIN * 2));

    SOFT_SWD_TARGET_RESET_PORT->BSRR = 0x1 << SOFT_SWD_TARGET_RESET_PIN;
}

// Инициализация программного SWD
void SoftSWD_Init()
{
    SoftSWD_RCC_Enable();   // Включение тактирования нужного порта GPIO
    SoftSWD_Pin_Enable();   // Настройка пинов программного SWD
}

// Сброс линии SWD (нужен перед началом работы для синхронизации программатора и таргета)
void SoftSWD_Line_Reset(void)                   // работает
{
    SOFT_SWD_DATA_HIGH();
    for (uint8_t i = 0; i < 50; i++)
    {
        SOFT_SWD_CLOCK_CYCLE();
    }
}

void SoftSWD_JTAGtoSWD()                        // работает
{
    uint16_t switch_seq = SOFT_SWD_JTAG_TO_SWD;
    for (int i = 0; i < 16; i++)
    {
        SoftSWD_WriteBit(switch_seq & 0x01);
        switch_seq >>= 1;
    }
}

// Запись бита данных в SWD
void SoftSWD_WriteBit(uint8_t bit)              // работает
{
    if (bit) SOFT_SWD_DATA_HIGH();
    else SOFT_SWD_DATA_LOW();
    SOFT_SWD_DELAY_TICKS(SOFT_SWD_TICK_DURATION / 2);
    SOFT_SWD_CLK_HIGH();
    SOFT_SWD_DELAY_TICKS(SOFT_SWD_TICK_DURATION / 2);
    SOFT_SWD_CLK_LOW();
}

// Чтение бита данных
uint8_t SoftSWD_ReadBit()                       // работает
{
    uint8_t bit = 0;
    SOFT_SWD_DELAY_TICKS(SOFT_SWD_TICK_DURATION / 2);
    bit = (SOFT_SWD_DATA_PORT->IDR & (1U << SOFT_SWD_DATA_PIN)) ? 1 : 0;
    SOFT_SWD_CLK_HIGH();
    SOFT_SWD_DELAY_TICKS(SOFT_SWD_TICK_DURATION / 2);
    SOFT_SWD_CLK_LOW();

    return bit;
}

// Запись байта данных
void SoftSWD_WriteByte(uint8_t byte)            // работает
{
    for (uint8_t i = 0; i < 8; i++)
    {
        SoftSWD_WriteBit(byte & 0x1);
        byte >>= 0x1;
    }
}

// Чтение 3 бит ACK
uint8_t SoftSWD_ReadACK()
{
    uint8_t ack = 0x0;
    for (uint8_t i = 0; i < 3; i++)
    {
        ack |= SoftSWD_ReadBit() << i;
    }
    return ack;
}

// Отправка N бит



// Переключить SWDIO на чтение из таргета
void SoftSWD_Trn_Input()                        // работает
{
    SOFT_SWD_DATA_SET_INPUT();
    SOFT_SWD_CLOCK_CYCLE();
}

// Переключить SWDIO на запись в таргет
void SoftSWD_Trn_Output()                       // работает
{
    SOFT_SWD_CLOCK_CYCLE();
    SOFT_SWD_DATA_SET_OUTPUT();
}

// Подключение к таргету
void SoftSWD_Connect()
{
    SoftSWD_Line_Reset();
    SoftSWD_JTAGtoSWD();
    SoftSWD_Line_Reset();
}


/** Переключение направления линии данных */
// в будущем должен быть static
void SoftSWD_Trn()
{
    if (Master_Direction)
    {
        Master_Direction = Master_Input;
        SoftSWD_Trn_Input();
    }
    else
    {
        Master_Direction = Master_Output;
        SoftSWD_Trn_Output();
    }
}






