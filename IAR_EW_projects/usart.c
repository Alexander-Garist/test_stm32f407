/**
  * @file    usart.c
  * @brief   Файл содержит реализации функций UART
  */

/** Includes **********************************************************************************************************/
#include "usart.h"
#include "gpio.h"
#include "systick.h"
#include "string.h"

/** Defines ***********************************************************************************************************/
#define freq_APB1	16000000
#define freq_APB2	16000000

// Структура для хранения данных каждого USART
typedef struct {
    volatile char rx_buffer[USART_BUFFER_SIZE];
    volatile uint8_t rx_index;
    volatile uint8_t command_ready;
} USART_Data_t;

// Данные для каждого USART
static USART_Data_t usart1_data = {0};
static USART_Data_t usart2_data = {0};
static USART_Data_t usart3_data = {0};
static USART_Data_t usart4_data = {0};
static USART_Data_t usart5_data = {0};
static USART_Data_t usart6_data = {0};
/***************************************** Статические функции ********************************************************/

// Получение указателя на данные конкретного USART
static USART_Data_t* USART_Get_Data(USART_TypeDef* USARTx)
{
    switch ((uint32_t)USARTx)
    {
        case ((uint32_t)USART1): return &usart1_data;
        case ((uint32_t)USART2): return &usart2_data;
        case ((uint32_t)USART3): return &usart3_data;
        case ((uint32_t)UART4):  return &usart4_data;
        case ((uint32_t)UART5):  return &usart5_data;
        case ((uint32_t)USART6): return &usart6_data;
        default: return NULL;
    }
}

// Включение тактирования модуля UART/USART
static void USART_RCC_Enable(USART_TypeDef* USARTx)
{
	if (USARTx == USART1)	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
	if (USARTx == USART2)	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
	if (USARTx == USART3)	RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
	if (USARTx == UART4)	RCC->APB1ENR |= RCC_APB1ENR_UART4EN;
	if (USARTx == UART5)	RCC->APB1ENR |= RCC_APB1ENR_UART5EN;
	if (USARTx == USART6)	RCC->APB2ENR |= RCC_APB2ENR_USART6EN;
}

// Настройка регистра BRR и включение модуля USARTx
static void USART_Init(USART_TypeDef* USARTx, uint32_t baudrate)
{
	/** Настройка регистра BRR (Baud rate register)
	*	APB1 для USART 2/3 UART 4/5
	*	APB2 для USART 1/6
	*/
	switch ((uint32_t)USARTx)
	{
		case ((uint32_t)USART1):	USARTx->BRR = freq_APB2 / baudrate;		break;
		case ((uint32_t)USART2):	USARTx->BRR = freq_APB1 / baudrate; 	break;
		case ((uint32_t)USART3):	USARTx->BRR = freq_APB1 / baudrate; 	break;
		case ((uint32_t)UART4):		USARTx->BRR = freq_APB1 / baudrate; 	break;
		case ((uint32_t)UART5):		USARTx->BRR = freq_APB1 / baudrate; 	break;
		case ((uint32_t)USART6):	USARTx->BRR = freq_APB2 / baudrate; 	break;
	}

	// Включение USARTx: TX, RX и прерывание по приему
    USARTx->CR1 = USART_CR1_TE 			// Transmitter enable
				| USART_CR1_RE			// Receiver enable
				| USART_CR1_RXNEIE;		// Разрешение прерываний по приемнику (т.е. если приемник не пустой, вызывается обработчик прерывания соответствующего модуля USART)

    USARTx->CR1 |= USART_CR1_UE;		// USART enable
}

// Разрешение обработки прерываний USART и установка высокого приоритета соответствующего прерывания
static void USART_EnableIRQ(USART_TypeDef* USARTx)
{
    switch((uint32_t)USARTx) {
        case ((uint32_t)USART1):
            NVIC_EnableIRQ(USART1_IRQn);
			NVIC_SetPriority(USART1_IRQn, 0);
            break;

        case ((uint32_t)USART2):
            NVIC_EnableIRQ(USART2_IRQn);
    		NVIC_SetPriority(USART2_IRQn, 0);
            break;

        case ((uint32_t)USART3):
            NVIC_EnableIRQ(USART3_IRQn);
			NVIC_SetPriority(USART3_IRQn, 0);
            break;

        case ((uint32_t)UART4):
            NVIC_EnableIRQ(UART4_IRQn);
			NVIC_SetPriority(UART4_IRQn, 0);
            break;

        case ((uint32_t)UART5):
            NVIC_EnableIRQ(UART5_IRQn);
			NVIC_SetPriority(UART5_IRQn, 0);
            break;

        case ((uint32_t)USART6):
			NVIC_EnableIRQ(USART6_IRQn);
			NVIC_SetPriority(USART6_IRQn, 0);
            break;
    }
}



// Обработка принятого символа в прерывании
static void USART_Process_Rx_Char(USART_TypeDef* USARTx, char received_char)
{
    USART_Data_t* data = USART_Get_Data(USARTx);
    if (!data) return;

    // Обрабатываем конец команды (Enter)
    if (received_char == '\r' || received_char == '\n')
    {
        if (data->rx_index > 0)
        {
            data->rx_buffer[data->rx_index] = '\0';
            data->command_ready = 1;
            data->rx_index = 0;
        }
    }
    // Обрабатываем обычный символ
    else if (data->rx_index < (USART_BUFFER_SIZE - 1))
    {
        data->rx_buffer[data->rx_index++] = received_char;
    }
    // Переполнение буфера
    else
    {
        data->rx_index = 0;
        data->rx_buffer[0] = '\0';
    }
}



/*************************************** Публичные функции ************************************************************/

// Включение и настройка модуля USARTx ( USART1 // USART2 // USART3 // UART4 // UART5 // USART6)
void USART_Enable(USART_TypeDef* USARTx, GPIO_TypeDef* GPIO_port_Tx, int GPIO_pin_Tx, GPIO_TypeDef* GPIO_port_Rx, int GPIO_pin_Rx , uint32_t baudrate)
{
	// Включение тактирования USARTx
	USART_RCC_Enable(USARTx);

	// Включение тактирования GPIO, настройка заданных пинов приемника и передатчика в режиме AF (регистры MODER и AFR)
	GPIO_Enable_USART(USARTx, GPIO_port_Tx, GPIO_port_Rx, GPIO_pin_Tx, GPIO_pin_Rx);

	// Настройка регистра BRR и включение модуля USARTx
	USART_Init(USARTx, baudrate);

	// Разрешение обработки прерываний USART и установка высокого приоритета соответствующего прерывания
    USART_EnableIRQ(USARTx);
}

void USART_Send_Char(USART_TypeDef* USARTx, char c)
{
    // Ждем пока буфер передатчика освободится
    while (!(USARTx->SR & USART_SR_TXE));
    // Отправляем символ
    USARTx->DR = c;
}

void USART_Send_String(USART_TypeDef* USARTx, const char* str)
{
    while (*str)
    {
        USART_Send_Char(USARTx, *str++);
    }
}

void USART_Send_Number(USART_TypeDef* USARTx, uint32_t num)
{
    char buffer[12];
    char *p = buffer + 11;
    *p = '\0';

    if (num == 0)
    {
        USART_Send_Char(USARTx, '0');
        return;
    }

    while (num > 0)
    {
        *--p = '0' + (num % 10);
        num /= 10;
    }

    USART_Send_String(USARTx, p);
}

uint8_t USART_Receive_Char(USART_TypeDef* USARTx, char* c)
{
    if (USARTx->SR & USART_SR_RXNE)
    {
        *c = (char)(USARTx->DR & 0xFF);
        return 1;
    }
    return 0;
}

uint8_t USART_Is_Data_Received(USART_TypeDef* USARTx)
{
    USART_Data_t* data = USART_Get_Data(USARTx);
    return data ? data->command_ready : 0;
}

char* USART_Get_Rx_Buffer(USART_TypeDef* USARTx)
{
    USART_Data_t* data = USART_Get_Data(USARTx);
    return data ? (char*)data->rx_buffer : NULL;
}

void USART_Clear_Buffer(USART_TypeDef* USARTx)
{
    USART_Data_t* data = USART_Get_Data(USARTx);
    if (data)
    {
        data->rx_index = 0;
        data->command_ready = 0;
        data->rx_buffer[0] = '\0';
    }
}

/***************************************** Обработчики прерываний *****************************************************/

void USART1_IRQHandler(void)
{
    if (USART1->SR & USART_SR_RXNE)
    {
        char received_char = (char)(USART1->DR & 0xFF);
        USART_Process_Rx_Char(USART1, received_char);
    }
}

void USART2_IRQHandler(void)
{
    if (USART2->SR & USART_SR_RXNE)
    {
        char received_char = (char)(USART2->DR & 0xFF);
        USART_Process_Rx_Char(USART2, received_char);
    }
}

void USART3_IRQHandler(void)
{
    if (USART3->SR & USART_SR_RXNE)
    {
        char received_char = (char)(USART3->DR & 0xFF);
        USART_Process_Rx_Char(USART3, received_char);

        // Автоматическая пересылка из USART3 в USART1
        USART_Send_Char(USART1, received_char);

        // Индикация приема (опционально)
        GPIO_set_HIGH(GPIOD, 15);
        delay_ms(10);  // Короткая индикация
        GPIO_set_LOW(GPIOD, 15);
    }
}

void UART4_IRQHandler(void)
{
    if (UART4->SR & USART_SR_RXNE)
    {
        char received_char = (char)(UART4->DR & 0xFF);
        USART_Process_Rx_Char(UART4, received_char);
    }
}

void UART5_IRQHandler(void)
{
    if (UART5->SR & USART_SR_RXNE)
    {
        char received_char = (char)(UART5->DR & 0xFF);
        USART_Process_Rx_Char(UART5, received_char);
    }
}

void USART6_IRQHandler(void)
{
    if (USART6->SR & USART_SR_RXNE)
    {
        char received_char = (char)(USART6->DR & 0xFF);
        USART_Process_Rx_Char(USART6, received_char);
    }
}

