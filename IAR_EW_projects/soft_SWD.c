#include "soft_SWD.h"
#include "systick.h"

/** В дальнейшем если появятся какие-то настройки, то они должны быть оформлены в структуру */
// Текущее состояние мастера (настроен на вход или выход)
// Исходно мастер настроен на выход для отправки запроса к таргету
static SoftSWD_Direction Master_Direction = Master_Output;

/***************************************************************************************** Настройка программного SWD */

/** Включение тактирования нужного порта GPIO */
static void SoftSWD_RCC_Enable()
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN;
}

/** Настройка пинов программного SWD */
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

    SOFT_SWD_TARGET_RESET_PORT->BSRR = 0x1 << SOFT_SWD_TARGET_RESET_PIN;    // Пин RESET находится в высоком состоянии для работы таргета
}
/**********************************************************************************************************************/


/******************************************************************************* Низкоуровневое управление пинами SWD */

/** Пустой такт */
static void SoftSWD_Clock_Cycle()
{
    delay_ticks(SOFT_SWD_TICK_DURATION);
    SOFT_SWD_CLK_HIGH();
    delay_ticks(SOFT_SWD_TICK_DURATION);
    SOFT_SWD_CLK_LOW();
}

/** Переключение направления линии данных */
static void SoftSWD_Trn()
{
    if (Master_Direction == Master_Output)
    {
        Master_Direction = Master_Input;
        SOFT_SWD_DATA_SET_INPUT();
        SoftSWD_Clock_Cycle();
    }
    else
    {
        Master_Direction = Master_Output;
        SoftSWD_Clock_Cycle();
        SOFT_SWD_DATA_SET_OUTPUT();
    }
}

/** Сброс линии SWD (нужен перед началом работы для синхронизации программатора и таргета) */
static void SoftSWD_Line_Reset(void)
{
    SOFT_SWD_DATA_HIGH();
    for (uint8_t i = 0; i < 50; i++)
    {
        SoftSWD_Clock_Cycle();
    }
}
/**********************************************************************************************************************/


/******************************************************************************* Низкоуровневые функции чтения/записи */

/** Запись бита данных в SWD */
static void SoftSWD_WriteBit(uint8_t bit)
{
    if (bit) SOFT_SWD_DATA_HIGH();
    else SOFT_SWD_DATA_LOW();
    delay_ticks(SOFT_SWD_TICK_DURATION);
    SOFT_SWD_CLK_HIGH();
    delay_ticks(SOFT_SWD_TICK_DURATION);
    SOFT_SWD_CLK_LOW();
}

/** Чтение бита данных */
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

/** Запись байта данных */
static void SoftSWD_WriteByte(uint8_t byte)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        SoftSWD_WriteBit(byte & 0x1);
        byte >>= 0x1;
    }
}

static void SoftSWD_Idle_Byte()
{
    SoftSWD_WriteByte(0x0);
}

/** Запись 32 бит данных + 1 бит четности этих данных + 8 бит нулей для стабильности */
static void SoftSWD_WriteData(uint32_t data)
{
    uint8_t parity_bit = 0;
    uint32_t temp_data = data;

    // отправка 32 бит данных
    for (uint8_t i = 0; i < 32; i++)
    {
        if (temp_data & 0x1) parity_bit++;
        SoftSWD_WriteBit(temp_data & 0x1);
        temp_data >>= 0x1;
    }

    // отправка бита четности
    SoftSWD_WriteBit(parity_bit % 2);

    // отправка 1 байта нулей
    SoftSWD_Idle_Byte();
}

/** Чтение 32 бит данных + 1 бита четности */
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

/** Переключение линии в режим SWD */
static void SoftSWD_JTAGtoSWD()
{
    uint16_t switch_seq = SOFT_SWD_JTAG_TO_SWD;
    for (int i = 0; i < 16; i++)
    {
        SoftSWD_WriteBit(switch_seq & 0x01);
        switch_seq >>= 1;
    }
}


/**********************************************************************************************************************/


/*************************************************************************** Функции для работы с запросами к таргету */

/** Формирование запроса используя структуру */
static SoftSWD_Request SoftSWD_MakeRequest_WithStruct(uint8_t DP_AP, uint8_t RnW, uint8_t Register_Address)
{
    SoftSWD_Request req;
    uint8_t parity = 0;

    req.start = 1;
    req.DP_AP = DP_AP;
    req.RnW = RnW;

    req.Addr_3 = (Register_Address >> 3) & 0x01;
    req.Addr_2 = (Register_Address >> 2) & 0x01;

    req.stop = 0;
    req.park = 1;

    if (DP_AP)              parity++;
    if (RnW)                parity++;
    if (Register_Address & (0x1 << 2))  parity++;
    if (Register_Address & (0x1 << 3))  parity++;

    parity %= 2;
    req.parity = parity;

    return req;
}

///////** Формирование запроса используя битовые маски */
//////static uint8_t SoftSWD_MakeRequest_WithMasks(uint8_t DP_AP, uint8_t RnW, uint8_t Register_Address)
//////{
//////    uint8_t req = REQUEST_BASE;
//////    uint8_t parity = 0;
//////
//////    if (DP_AP_MASK(DP_AP)) parity++;
//////    if (RnW_MASK(RnW)) parity++;
//////    if (ADDR2_MASK(Register_Address)) parity++;
//////    if (ADDR3_MASK(Register_Address)) parity++;
//////    parity %= 2;
//////
//////    req |= DP_AP_MASK(DP_AP) | RnW_MASK(RnW) | ADDR2_MASK(Register_Address) | ADDR3_MASK(Register_Address) | PARITY_MASK(parity);
//////
//////    return req;
//////}

/** Прием 3 бит подтверждения ACK */
static uint8_t SoftSWD_ReadACK()
{
    uint8_t ack = 0x0;
    for (uint8_t i = 0; i < 3; i++)
    {
        ack |= SoftSWD_ReadBit() << i;
    }
    return ack;
}

/** Отправка запроса от мастера к таргету */
static void SoftSWD_Send_Request(SoftSWD_Request req)
{
    // Направление должно быть OUTPUT
    if (Master_Direction != Master_Output) SoftSWD_Trn();

    SoftSWD_WriteBit(req.start);
    SoftSWD_WriteBit(req.DP_AP);
    SoftSWD_WriteBit(req.RnW);
    SoftSWD_WriteBit(req.Addr_2);
    SoftSWD_WriteBit(req.Addr_3);
    SoftSWD_WriteBit(req.parity);
    SoftSWD_WriteBit(req.stop);
    SoftSWD_WriteBit(req.park);
}

/** Отправка запроса и получение подтверждения от таргета */
static uint8_t SoftSWD_Send_Request_ACK(SoftSWD_Request req)
{
    uint8_t ACK = 0x0;

    // В любом случае направление должно быть OUTPUT в начале обмена с таргетом
    if (Master_Direction != Master_Output) SoftSWD_Trn();

    SoftSWD_Send_Request(req);      // отправка запроса
    SoftSWD_Trn();                  // переключение на прием
    ACK = SoftSWD_ReadACK();

    // Запрос был либо на чтение, либо на запись (бит RnW), если на запись => после получения ACK должно быть переключение опять на OUTPUT
    if (req.RnW == 0x0) SoftSWD_Trn();

    return ACK;
}
/**********************************************************************************************************************/


/****************************************************************************************** Работа с регистрами DP AP */

/** Сброс ошибок (запись в регистр DP_ABORT) */
void SoftSWD_ClearErrors(void)
{
    SoftSWD_Request req = SoftSWD_MakeRequest_WithStruct(DP, WRITE, DP_ABORT);
    SoftSWD_Send_Request_ACK(req);
    SoftSWD_WriteData(DP_ABORT_ORUNERRCLR |
                      DP_ABORT_WDERRCLR |
                      DP_ABORT_STKERRCLR |
                      DP_ABORT_STKCMPCLR
                          );
}

///** Запись значения в регистр AP или DP */
//static void SoftSWD_WriteRegister(uint8_t DP_AP, uint8_t Addr, uint32_t register_value)
//{
//    SoftSWD_Request req = SoftSWD_MakeRequest_WithStruct(DP_AP, WRITE, Addr);       // Запрос сформирован
//    SoftSWD_Idle_Byte();                                                            // Отправка 8 бит нулей перед отправкой запроса
//    if (SoftSWD_Send_Request_ACK(req) == 0x1)           // Запрос отправлен, ACK получен
//    {
//        SoftSWD_WriteData(register_value);              // если ACK OK, то записывается значение в регистр
//    }
//    else
//    {
//        // Сбросить ошибки и попробовать еще 1 раз
//        SoftSWD_ClearErrors();
//        if (SoftSWD_Send_Request_ACK(req) == 0x1)
//        {
//            SoftSWD_WriteData(register_value);
//        }
//    }
//}


/** Запись значения в регистр AP или DP */
void SoftSWD_WriteRegister(uint8_t DP_AP, uint8_t Addr, uint32_t register_value)
{
    SoftSWD_Request req = SoftSWD_MakeRequest_WithStruct(DP_AP, WRITE, Addr);       // Запрос сформирован
    SoftSWD_Idle_Byte();                                                            // Отправка 8 бит нулей перед отправкой запроса

    switch (SoftSWD_Send_Request_ACK(req))
    {
        // В случае получения ACK_OK происходит запись в регистр как и планировалось
        case SWD_ACK_OK:
        {
            SoftSWD_WriteData(register_value);
            break;
        }

        // В случае получения ACK_WAIT необходимо повторить попытку записи в регистр, т.к. прямо сейчас таргет занят
        case SWD_ACK_WAIT:
        {
            uint32_t attempts = 1000;
            while (attempts)
            {
                attempts--;
                if (SoftSWD_Send_Request_ACK(req) == SWD_ACK_OK)
                {
                    SoftSWD_WriteData(register_value);
                    break;
                }
            }   // end while

            // Если после нескольких попыток запись получилась, нужно выйти из switch, иначе произойдет переход в case SWD_ACK_FAIL
            if (SoftSWD_Send_Request_ACK(req) == SWD_ACK_OK) SoftSWD_WriteData(register_value); break;
        }   // end case SWD_ACK_WAIT

        case SWD_ACK_FAIL:
        {
            // Сбросить ошибки и попробовать еще 1 раз
            SoftSWD_ClearErrors();
            if (SoftSWD_Send_Request_ACK(req) == SWD_ACK_OK)
            {
                SoftSWD_WriteData(register_value);
            }
            break;
        }
    }
}

/** Чтение значения регистра AP или DP */
uint32_t SoftSWD_ReadRegister(uint8_t DP_AP, uint8_t Addr)
{
    uint32_t register_value = 0x0;
    SoftSWD_Request req = SoftSWD_MakeRequest_WithStruct(DP_AP, READ, Addr);    // запрос создан
    SoftSWD_Idle_Byte();
    if (SoftSWD_Send_Request_ACK(req) == 0x1)                       // запрос отправлен, ACK получен
        register_value = SoftSWD_ReadData();                        // если ACK OK, происходит чтение регистра
    else
    {
        // Сбросить ошибки и попробовать еще 1 раз
        SoftSWD_ClearErrors();
        if (SoftSWD_Send_Request_ACK(req) == 0x1)  register_value = SoftSWD_ReadData();
    }
    SoftSWD_Trn();
    SoftSWD_Idle_Byte();
    return register_value;
}
/**********************************************************************************************************************/


/**************************************************************************************** Работа с буфером данных SWD */

/** Разбить 32 бита в порядке little-endian на 4 байта */
static void parse_words(uint32_t data_word, uint8_t* buffer)
{
    buffer[0] = data_word & 0xFF;
    buffer[1] = data_word >> 8 & 0xFF;
    buffer[2] = data_word >> 16 & 0xFF;
    buffer[3] = data_word >> 24 & 0xFF;
}

/** Упаковка 4 байт в одно 32-битное слово */
static uint32_t pack_words(uint8_t* buffer)
{
    uint32_t data = 0;
    data |= (uint32_t)buffer[0];
    data |= (uint32_t)buffer[1] << 8;
    data |= (uint32_t)buffer[2] << 16;
    data |= (uint32_t)buffer[3] << 24;
    return data;
}
/**********************************************************************************************************************/



/************************************************************************************************* ГЛОБАЛЬНЫЕ ФУНКЦИИ */

/** Инициализация программного SWD */
void SoftSWD_Init()
{
    SoftSWD_RCC_Enable();   // Включение тактирования нужного порта GPIO
    SoftSWD_Pin_Enable();   // Настройка пинов программного SWD
}

/** Синхронизация мастера и таргета */
void SoftSWD_Sync_Target()
{
    SoftSWD_Line_Reset();
    SoftSWD_JTAGtoSWD();
    SoftSWD_Line_Reset();
}

/** Аппаратный сброс таргета */
void SoftSWD_Reset_Target()
{
    SOFT_SWD_RESET_TARGET_LOW();    // RESET подтянуть к GND => таргет сбрасывается
    delay_ms(10);                   // Подождать 10 мс
    SOFT_SWD_RESET_TARGET_HIGH();   // RESET подтянуть к питанию => таргет выходит из состояния сброса
    delay_ms(10);                   // Подождать еще 10 мс чтобы таргет успел проснуться
}

/** Получение 32-битного IDCODE таргета (чтение регистра DP_IDCODE) */
uint32_t SoftSWD_Get_IDCODE()
{
    SoftSWD_Sync_Target();
    return (SoftSWD_ReadRegister(DP, DP_IDCODE));
}

/** Настройка DP регистров для работы программного SWD */
static void SoftSWD_set_DP_registers(uint32_t APsel, uint8_t APbanksel)
{
    // Включение питания системы и модуля отладки
    SoftSWD_WriteRegister(DP, DP_CTRL_STAT, DP_CTRL_STAT_CSYSPWRUPREQ |
                                            DP_CTRL_STAT_CDBGPWRUPREQ);

    // Выбор текущего AP и активного регистрового блока выбранного AP
    SoftSWD_WriteRegister(DP, DP_SELECT, DP_SELECT_APSEL(APsel) |
                                         DP_SELECT_APBANKSEL(APbanksel));
}

/** Функция-обертка статической функции настройки DP регистров для работы с MEM-AP */
void SoftSWD_set_MEM_AP()
{
    SoftSWD_set_DP_registers(MEM_AP_APSEL, MEM_AP_APBANKSEL);
}




/** Чтение из памяти таргета (по указанному адресу памяти, заданное количество байт) */
void SoftSWD_ReadMemory(uint32_t address, uint8_t* buffer, uint32_t size)
{
    // 1. Включение питания и выбор порта MEM-AP (запись в CTRL/STAT и выбор AP 0, Bank 0)
    SoftSWD_set_MEM_AP();

    // AP 0, Addr 0x00 (CSW). Значение 0x23000002
    SoftSWD_WriteRegister(AP, AP_CSW, MEM_AP_DEFAULT | AP_CSW_ADDRINC);

    // 1. запись адреса в TAR (AP 0x04)
    SoftSWD_WriteRegister(AP, AP_TAR, address);

    // 2. запуск чтения через DRW (AP 0x0C)
    SoftSWD_ReadRegister(AP, AP_DRW);

    // 3. определить сколько операций чтения по 32 бита будет
    uint32_t operations = (size + 3) / 4; // за 1 раз читаются 4 байта

    uint32_t flash_word;    // переменная, в которую считываются 4 байта из регистра RDBUFF
    for (uint32_t i = 0; i < operations; i++)
    {
        // Если это не последнее слово, читается DRW, чтобы произошел автоинкремент
        if (i < operations - 1)
        {
            flash_word = SoftSWD_ReadRegister(AP, AP_DRW);
        }
        else
        {
            // Последнее слово просто забрать из RDBUFF
            flash_word = SoftSWD_ReadRegister(DP, DP_RDBUFF);
        }

        // Записать в буфер со смещением
        parse_words(flash_word, buffer + (i * 4));
    }
}

/** Запись в память (RAM) таргета */
void SoftSWD_WriteMemory_RAM(uint32_t address, uint8_t* buffer, uint32_t size)
{
    // 1. Включение питания и выбор порта MEM-AP (запись в CTRL/STAT и выбор AP 0, Bank 0)
    SoftSWD_set_MEM_AP();

    // 2. Настройка CSW: 32-bit + Auto-increment (0x23000012)
    SoftSWD_WriteRegister(AP, AP_CSW, MEM_AP_DEFAULT | AP_CSW_ADDRINC);

    // 3. Установка начального адреса в TAR (AP 0x04)
    SoftSWD_WriteRegister(AP, AP_TAR, address);

    // 4. Определение количества 32-битных слов
    uint32_t operations = size / 4;
    if (size % 4) operations++; // Округляем вверх, если байты не кратны 4

    // 5. Цикл записи
    for (uint32_t i = 0; i < operations; i++)
    {
        // Упаковка байт из буфера в 32-битное слово
        uint32_t data_to_write = pack_words(buffer + (i * 4));

        // Запись в DRW (AP 0x0C). Автоинкремент TAR происходит сразу после каждой успешной транзакции данных.
        SoftSWD_WriteRegister(AP, AP_DRW, data_to_write);
    }

    // прочитать регистр DP_CTRL_STAT
    uint32_t ctrl_stat = SoftSWD_ReadRegister(DP, DP_CTRL_STAT);
    if ((ctrl_stat >> 5) & 0x1) GPIO_set_HIGH(GPIOD, 14);
}



