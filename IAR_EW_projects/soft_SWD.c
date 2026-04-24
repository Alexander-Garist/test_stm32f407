#include "soft_SWD.h"
#include "systick.h"

// Текущее состояние мастера (настроен на вход или выход)
// Исходно мастер настроен на выход для отправки запроса к таргету
SoftSWD_Direction Master_Direction = Master_Output;

/***************************************************************************************** Настройка программного SWD */
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
/**********************************************************************************************************************/


/******************************************************************************* Низкоуровневое управление пинами SWD */
// Пустой такт
static void SoftSWD_Clock_Cycle()
{
    delay_ticks(SOFT_SWD_TICK_DURATION);
    SOFT_SWD_CLK_HIGH();
    delay_ticks(SOFT_SWD_TICK_DURATION);
    SOFT_SWD_CLK_LOW();
}

// Переключить SWDIO на чтение из таргета
static void SoftSWD_Trn_Input()
{
    Master_Direction = Master_Input;
    SOFT_SWD_DATA_SET_INPUT();
    SoftSWD_Clock_Cycle();
}

// Переключить SWDIO на запись в таргет
static void SoftSWD_Trn_Output()
{
    Master_Direction = Master_Output;
    SoftSWD_Clock_Cycle();
    SOFT_SWD_DATA_SET_OUTPUT();
}

// Переключение направления линии данных
static void SoftSWD_Trn()
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
/**********************************************************************************************************************/


/******************************************************************************* Низкоуровневые функции чтения/записи */
// Запись бита данных в SWD
static void SoftSWD_WriteBit(uint8_t bit)
{
    if (bit) SOFT_SWD_DATA_HIGH();
    else SOFT_SWD_DATA_LOW();
    delay_ticks(SOFT_SWD_TICK_DURATION);
    SOFT_SWD_CLK_HIGH();
    delay_ticks(SOFT_SWD_TICK_DURATION);
    SOFT_SWD_CLK_LOW();
}

// Чтение бита данных
static uint8_t SoftSWD_ReadBit()
{
    uint8_t bit = 0;
    delay_ticks(SOFT_SWD_TICK_DURATION);
    bit = (SOFT_SWD_DATA_PORT->IDR & (1U << SOFT_SWD_DATA_PIN)) ? 1 : 0;
    SOFT_SWD_CLK_HIGH();
    delay_ticks(SOFT_SWD_TICK_DURATION);
    SOFT_SWD_CLK_LOW();

    return bit;
}

// Запись байта данных
static void SoftSWD_WriteByte(uint8_t byte)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        SoftSWD_WriteBit(byte & 0x1);
        byte >>= 0x1;
    }
}

// Запись 32 бит данных + 1 бит четности этих данных + 8 бит нулей для стабильности
static void SoftSWD_WriteData(uint32_t data)
{
    uint8_t parity_bit = 0;

    // отправка 32 бит данных
    for (uint8_t i = 0; i < 32; i++)
    {
        if (data & 0x1) parity_bit++;
        SoftSWD_WriteBit(data & 0x1);
        data >>= 0x1;
    }

    // отправка бита четности
    SoftSWD_WriteBit(parity_bit % 2);

    // отправка 1 байта нулей
    SoftSWD_WriteByte(0x0);
}

// Чтение 32 бит данных + 1 бита четности
static uint32_t SoftSWD_ReadData()
{
    uint32_t data = 0x0;
    uint8_t parity_bit = 0;
    uint8_t bit = 0;

    // Чтение 32 бит данных в переменную data + подсчет четности данных
    for (uint8_t i = 0; i < 32; i++)
    {
        bit = SoftSWD_ReadBit();
        if (bit)
        {
            data |= (0x1 << i);
            parity_bit++;
        }
    }
    parity_bit %= 2;

    // Если четность посчитанная совпала с четностью, полученной от таргета, значит данные получены правильно
    if (parity_bit == SoftSWD_ReadBit()) return data;
    else return 0xAAAABBBB;
}
/**********************************************************************************************************************/


/*************************************************************************** Функции для работы с запросами к таргету */
// Формирование запроса
static SoftSWD_Request SoftSWD_MakeRequest(uint8_t DP_AP, uint8_t RnW, uint8_t Addr)
{
    SoftSWD_Request req;
    uint8_t parity = 0;

    req.start = 1;
    req.DP_AP = DP_AP;
    req.RnW = RnW;

    req.Addr_3 = (Addr >> 3) & 0x01;
    req.Addr_2 = (Addr >> 2) & 0x01;

    req.stop = 0;
    req.park = 1;

    if (DP_AP)              parity++;
    if (RnW)                parity++;
    if (Addr & (0x1 << 2))  parity++;   // проверка 2 и 3 бита Addr => Addr нужно передавать в виде 0x0  0x4  0x8  0xC
    if (Addr & (0x1 << 3))  parity++;   //                                                          0000 0100 1000 1100

    parity %= 2;
    req.parity = parity;

    return req;
}

// Прием 3 бит подтверждения ACK.
static uint8_t SoftSWD_ReadACK()
{
    uint8_t ack = 0x0;
    for (uint8_t i = 0; i < 3; i++)
    {
        ack |= SoftSWD_ReadBit() << i;
    }
    return ack;
}

// Отправка запроса от мастера к таргету
static void SoftSWD_Send_Request(SoftSWD_Request req)
{
    SoftSWD_WriteBit(req.start);
    SoftSWD_WriteBit(req.DP_AP);
    SoftSWD_WriteBit(req.RnW);
    SoftSWD_WriteBit(req.Addr_2);
    SoftSWD_WriteBit(req.Addr_3);
    SoftSWD_WriteBit(req.parity);
    SoftSWD_WriteBit(req.stop);
    SoftSWD_WriteBit(req.park);
}

// Отправка запроса и получение подтверждения от таргета
static uint8_t SoftSWD_Send_Request_ACK(SoftSWD_Request req)
{
    uint8_t ACK = 0x0;

    // В любом случае направление должно быть OUTPUT в начале обмена с таргетом
    if (Master_Direction == Master_Input) SoftSWD_Trn();

    SoftSWD_Send_Request(req);      // отправка запроса
    SoftSWD_Trn();                  // переключение на прием
    ACK = SoftSWD_ReadACK();

    // Запрос был либо на чтение, либо на запись (бит RnW), если на запись => после получения ACK должно быть переключение опять на OUTPUT
    if (req.RnW == 0x0) SoftSWD_Trn();

    return ACK;
}
/**********************************************************************************************************************/


/****************************************************************************************** Работа с регистрами DP AP */
static void SoftSWD_ClearErrors(void)
{
    SoftSWD_Request req = SoftSWD_MakeRequest(0, 0, 0x00);
    SoftSWD_Send_Request_ACK(req);
    SoftSWD_WriteData(0x0000001E);
    SoftSWD_WriteByte(0x0); // Idle
}

// Запись значения в регистр AP или DP
static void SoftSWD_WriteRegister(uint8_t DP_AP, uint8_t Addr, uint32_t register_value)
{
    SoftSWD_Request req = SoftSWD_MakeRequest(DP_AP, 0x0, Addr);    // запрос создан
    SoftSWD_WriteByte(0x0);
    if (SoftSWD_Send_Request_ACK(req) == 0x1)                       // запрос отправлен, ACK получен
    {
        SoftSWD_WriteData(register_value);              // если ACK OK, то записывается значение в регистр
    }
    else
    {
        // Сбросить ошибки и попробовать еще 1 раз
        SoftSWD_ClearErrors();
        if (SoftSWD_Send_Request_ACK(req) == 0x1)
        {
            SoftSWD_WriteData(register_value);
        }
    }
}
/**********************************************************************************************************************/


/**************************************************************************************** Работа с буфером данных SWD */
// Разбить 32 бита в порядке little-endian на 4 байта
static void parse_words(uint32_t data_word, uint8_t* buffer)
{
    buffer[0] = data_word & 0xFF;
    buffer[1] = data_word >> 8 & 0xFF;
    buffer[2] = data_word >> 16 & 0xFF;
    buffer[3] = data_word >> 24 & 0xFF;
}
/**********************************************************************************************************************/



/************************************************************************************************* ГЛОБАЛЬНЫЕ ФУНКЦИИ */
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
    for (uint8_t i = 0; i < 50; i++)
    {
        SoftSWD_Clock_Cycle();
    }
}

// Переключение линии в режим SWD
void SoftSWD_JTAGtoSWD()
{
    uint16_t switch_seq = SOFT_SWD_JTAG_TO_SWD;
    for (int i = 0; i < 16; i++)
    {
        SoftSWD_WriteBit(switch_seq & 0x01);
        switch_seq >>= 1;
    }
}

// Чтение значения регистра AP или DP
uint32_t SoftSWD_ReadRegister(uint8_t DP_AP, uint8_t Addr)
{
    uint32_t register_value = 0x0;
    SoftSWD_Request req = SoftSWD_MakeRequest(DP_AP, 0x1, Addr);    // запрос создан
    SoftSWD_WriteByte(0x0);
    if (SoftSWD_Send_Request_ACK(req) == 0x1)                       // запрос отправлен, ACK получен
        register_value = SoftSWD_ReadData();                        // если ACK OK, происходит чтение регистра
    else
    {
        // Сбросить ошибки и попробовать еще 1 раз
        SoftSWD_ClearErrors();
        if (SoftSWD_Send_Request_ACK(req) == 0x1)  register_value = SoftSWD_ReadData();
    }
    SoftSWD_Trn();
    SoftSWD_WriteByte(0x0);
    return register_value;
}

// Чтение из памяти таргета (по указанному адресу памяти, заданное количество байт)
void SoftSWD_ReadMemory(uint32_t address, uint8_t* buffer, uint32_t size)
{
    SoftSWD_WriteRegister(0, 0x00, 0x0000001E); // DP ABORT
    SoftSWD_WriteRegister(0, 0x04, 0x50000000); // CTRL/STAT
    SoftSWD_WriteRegister(0, 0x08, 0x00000000); // AP 0, Bank 0

    // AP 0, Addr 0x00 (CSW). Значение 0x23000002
    SoftSWD_WriteRegister(1, 0x00, 0x23000012);

    // 1. запись адреса в TAR (AP 0x04)
    SoftSWD_WriteRegister(1, 0x04, address);

    // 2. запуск чтения через DRW (AP 0x0C)
    SoftSWD_ReadRegister(1, 0x0C);

    // 3. определить сколько операций чтения по 32 бита будет
    uint32_t operations = size / 4;// за 1 раз читаются 4 байта

    // можно читать количество байт, не кратное 4, тогда последней операцией чтения нужно забрать эти оставшиеся байты
    if (size % 4)
    {
        operations++;
    }

    uint32_t flash_word;    // переменная, в которую считываются 4 байта из регистра RDBUFF
    for (uint32_t i = 0; i < operations; i++)
    {
        // Если это не последнее слово, читается DRW, чтобы произошел автоинкремент
        if (i < operations - 1)
        {
            flash_word = SoftSWD_ReadRegister(1, 0x0C);
        }
        else
        {
            // Последнее слово просто забрать из RDBUFF
            flash_word = SoftSWD_ReadRegister(0, 0x0C);
        }

        // Записать в буфер со смещением
        parse_words(flash_word, buffer + (i * 4));
    }
}