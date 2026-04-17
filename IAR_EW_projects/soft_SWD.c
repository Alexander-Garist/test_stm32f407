#include "soft_SWD.h"
#include "systick.h"

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
    SOFT_SWD_DATA_PORT->OSPEEDR |= (OSPEEDR_MEDIUM << (SOFT_SWD_DATA_PIN * 2));

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
void SoftSWD_Line_Reset(void)
{
    SOFT_SWD_DATA_HIGH();

    for (int i = 0; i < 55; i++)
    {
        SoftSWD_WriteBit(1);
    }

    SoftSWD_WriteBit(0);
    SoftSWD_WriteBit(0);
}

void SoftSWD_JTAGtoSWD()
{
    uint16_t switch_seq = SOFT_SWD_JTAG_TO_SWD; // 0xE79E  b 1110 0111 1001 1110
                                           // с учетом LSB:  0111 1001 1110 0111
    for (int i = 0; i < 16; i++)
    {
        SoftSWD_WriteBit(switch_seq & 0x01);
        switch_seq >>= 1;
    }
}

// Аппаратный сброс таргета
void SoftSWD_Target_Reset()
{
    SOFT_SWD_RESET_TARGET_LOW();
    delay_ms(20);
    SOFT_SWD_RESET_TARGET_HIGH();
    delay_ms(50);   // Подождать 50 мс после сброса таргета перед тем, как что-то делать
}

// Запись бита данных в SWD
void SoftSWD_WriteBit(uint8_t bit)
{
    SOFT_SWD_DATA_SET_OUTPUT();

    // 1. Установить сигнал CLK в 0
    SOFT_SWD_CLK_HIGH();

    // 2. Установить сигнал DATA в 0 или 1
    if (bit) SOFT_SWD_DATA_HIGH();
    else SOFT_SWD_DATA_LOW();

    // 3. Блокирующая задержка длиной в половину такта
    delay_ticks(500);

    // 4. Установить сигнал CLK в 1
    SOFT_SWD_CLK_LOW();

    // 5. Блокирующая задержка длиной в половину такта
    delay_ticks(500);
}

// Чтение бита данных
uint8_t SoftSWD_ReadBit()
{
    uint8_t bit;

    //SOFT_SWD_DATA_SET_INPUT();  // макрос меняет MODER на вход

    SOFT_SWD_CLK_LOW();
    // половина такта SWD
    delay_ticks(500);


    // Задержка перед чтением
    delay_ticks(100);

    if (SOFT_SWD_DATA_PORT->IDR & (0x1 << SOFT_SWD_DATA_PIN)) bit = 1;
    else bit = 0;

    delay_ticks(400);

    // 5. Установить сигнал CLK в 0
    SOFT_SWD_CLK_HIGH();

    return bit;
}

/** 1 пустой такт, во время которого линия данных находится в состоянии high-Z (т.к. open-drain, достаточно подать 1)*/
void SoftSWD_TurnAround()
{
    //SOFT_SWD_DATA_SET_INPUT();
    SOFT_SWD_CLK_LOW();
    delay_us(5);
    SOFT_SWD_CLK_HIGH();
    delay_us(5);
    SOFT_SWD_CLK_LOW();
}

// Функция формирования пакета для отправки
uint8_t swd_form_packet(uint8_t request)
{
    uint8_t parity = 0;
    // Считаем четность бит 1, 2, 3, 4 (APnDP, RnW, A2, A3)
    for (int i = 1; i <= 4; i++) {
        if (request & (1 << i)) parity++;
    }

    if (parity % 2 != 0)
    {
        request |= (1 << 5); // Устанавливаем бит четности, если сумма нечетная
    }

    else
    {
        request &= ~(1 << 5); // Сброс бита четности, если сумма четная

    }
    return request;
}


/**
 * Отправляет 8-битный запрос и получает 3-битный ACK
 * - request Сформированный 8-битный байт (уже с битом четности)
 * return: uint8_t Значение ACK (1 - OK, 2 - WAIT, 4 - FAULT, 7 - No Response)
 */
uint8_t SoftSWD_TransferPacket(uint8_t request)
{
    uint8_t ack = 0;

    // 1. Передача запроса (Host -> Target)
    for (int i = 0; i < 8; i++)
    {
        SoftSWD_WriteBit(request & 0x01);
        request >>= 1;
    }

    // 2. Фаза Turnaround (TRN)
    // Хост отпускает линию на 1 такт, чтобы передать управление Таргету
    SoftSWD_TurnAround();

    // 3. Чтение подтверждения ACK (Target -> Host)
    // В SWD ACK состоит из 3-х бит, передаваемых LSB first
    for (int i = 0; i < 3; i++)
    {
        if (SoftSWD_ReadBit())
        {
            ack |= (1 << i);
        }
    }

    /* Если это была операция ЗАПИСИ, то сразу после ACK идет еще один TRN, а затем данные.
       Если это была операция ЧТЕНИЯ, то данные идут сразу за ACK. */

    return ack;
}


uint32_t SoftSWD_ReadIDCODE(void) {
    uint32_t idcode = 0;
    uint32_t data_parity = 0;

    // 1. Инициализация (сброс - переключение - сброс)
    SoftSWD_Line_Reset();
    SoftSWD_JTAGtoSWD();
    SoftSWD_Line_Reset();

    // 2. Формируем запрос: DP, Read, Addr 0x0 (IDCODE)
    // SWD_REQUEST(ap_n_dp, r_n_w, addr)
//    uint8_t req = swd_form_packet(SWD_REQUEST(SOFT_SWD_REQ_DP, SOFT_SWD_REQ_READ, 0x00));

    // 3. Передаем запрос (8 бит)
    for (int i = 0; i < 8; i++) {
        SoftSWD_WriteBit((0xA5 >> i) & 0x01);
    }

    // 4. Turnaround (Хост отдает линию)
    SoftSWD_TurnAround();

    // 5. Читаем ACK (3 бита)
    uint8_t ack = 0;
    for (int i = 0; i < 3; i++) {
        if (SoftSWD_ReadBit()) ack |= (1 << i);
    }

    if (ack != 1) { // 1 = OK (001 LSB first)
        // Ошибка: таргет не ответил или занят (WAIT/FAULT)
        SoftSWD_TurnAround(); // Вернуть управление и выйти
        return 0;
    }

    // 6. Читаем данные (32 бита)
    for (int i = 0; i < 32; i++) {
        if (SoftSWD_ReadBit()) {
            idcode |= (1UL << i);
            data_parity++;
        }
    }

    // 7. Читаем бит четности данных (1 бит)
    uint8_t parity_bit = SoftSWD_ReadBit();

    // 8. Финальный Turnaround (Таргет отдает линию)
    SoftSWD_TurnAround();

    // 9. Обязательные IDLE такты (8 шт) - завершают транзакцию в CoreSight
    for (int i = 0; i < 8; i++) {
        SoftSWD_WriteBit(0);
    }

    // Простая проверка четности (необязательно для теста, но полезно)
    if ((data_parity % 2) != parity_bit) {
        // Ошибка четности данных
    }

    return idcode;
}






/******************************************************************************************************************/
