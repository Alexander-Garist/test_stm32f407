/**
  * @file    usart.h
  * @brief   Файл содержит прототипы функций UART/USART
  */

/** Define to prevent recursive inclusion *****************************************************************************/
#ifndef __USART_H__
#define __USART_H__

/** Includes **********************************************************************************************************/
#include "CMSIS/stm32f4xx.h"

/** Defines ***********************************************************************************************************/
#define MAX_BUFFER_SIZE 256

// Типичные скорости приема/передачи по USART
typedef enum
{
	BRR_2400	= 2400,
	BRR_9600	= 9600,
	BRR_19200	= 19200,
	BRR_57600	= 57600,
	BRR_115200	= 115200,
	BRR_230400	= 230400,
	BRR_460800	= 460800,
	BRR_921600	= 921600,
	BRR_2250000	= 2250000,
	BRR_4500000	= 4500000
}USART_Baudrate_t;

/***************************** Структура для инициализации модуля USART ***********************************************/

	/**
	! Создать экземпляр USART_Init_Struct, передать указатель на данную структуру в функцию USART_Enable_with_struct.
	*/
typedef struct
{
	USART_TypeDef*	USARTx;			// Модуль UART/USART (USART1 / USART2 / USART3 / UART 4 / UART5 / USART6)
	GPIO_TypeDef*	GPIO_port_Tx;	// Порт передатчика (GPIOA / GPIOB / ...)
	int				GPIO_pin_Tx;	// Пин передатчика (0-15)
	GPIO_TypeDef*	GPIO_port_Rx;	// Порт приемника (GPIOA / GPIOB / ...)
	int				GPIO_pin_Rx;	// Пин приемника (0-15)
	uint32_t		baudrate;		// Скорость обмена данными бит/с
}USART_Init_Struct;

/*************************** Перечисление статусов выполнения функций USART *******************************************/
typedef enum
{
    USART_OK =					 0,  // Функция успешно отработала
    USART_ERROR_TRANSMIT =		-1,  // Ошибка передачи данных
    USART_ERROR_RECEIVE =		-2   // Ошибка приема данных
}USART_Status_t;

/** Functions *********************************************************************************************************/

/**************** Конфигурация *************************/

	/**
	! Включение и настройка модуля UART/USART с помощью созданной структуры инициализации.
	- Init_Struct - структура инициализации, содержит всю информацию о включаемом модуле USART.
	*/
void USART_Enable(USART_Init_Struct* Init_Struct);

	/**
	! Разрешение обработки прерываний USART и установка приоритета соответствующего прерывания.
	- USARTx - выбранный модуль UART/USART.
	- priority - приоритет прерывания для выбранного модуля USART.
	*/
void USART_EnableIRQ(USART_TypeDef*	USARTx,	uint32_t priority);

	/**
	! Запрет обработки прерываний USART.
	- USARTx - выбранный модуль UART/USART.
	*/
void USART_DisableIRQ(USART_TypeDef* USARTx);

/******************* Прием/передача **********************/

	/**
	! Передача данных по UART/USART
	- USARTx - выбранный модуль UART/USART.
	- data - указатель на массив символьных данных для передачи.
	- size - количество передаваемых байт.
	*/
USART_Status_t USART_Transmit(
	USART_TypeDef*	USARTx,
	char*			data,
	uint32_t		size
);

	/**
	! Прием данных по UART/USART.
	- USARTx - выбранный модуль UART/USART.
	- buffer - указатель на буфер приемника.
	- STOP_BYTE - стоповый байт, который прекращает прием данных
	*/
USART_Status_t USART_Receive(
	USART_TypeDef*	USARTx,
	char*			buffer,
	char			STOP_BYTE
);

/** эти функции добавились для управления генератором сигналов через USART */

	/**
	! Вывод в терминал текущего содержимого буфера.
	- buffer - указатель на буфер приемника.
	- buffer_size - размер буфера.
	*/
void USART_print_Buffer(char* buffer, uint32_t buffer_size);

	/**
	! Очистка буфера.
	- buffer - указатель на буфер приемника.
	*/
void USART_clear_Buffer(char* buffer);

/****************************** Обработчики прерываний UART/USART *****************************************************/

	/**
	! Обработчик прерывания USART1
	*/
void USART1_IRQHandler(void);

	/**
	! Обработчик прерывания USART2
	*/
void USART2_IRQHandler(void);

	/**
	! Обработчик прерывания USART3
	*/
void USART3_IRQHandler(void);

	/**
	! Обработчик прерывания UART4
	*/
void UART4_IRQHandler(void);

	/**
	! Обработчик прерывания UART5
	*/
void UART5_IRQHandler(void);

	/**
	! Обработчик прерывания USART6
	*/
void USART6_IRQHandler(void);

#endif /*__USART_H__ */