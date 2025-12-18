/**
  * @file    exti.h
  * @brief   Файл содержит прототипы функций EXTI
  */

/** Define to prevent recursive inclusion *****************************************************************************/
#ifndef __EXTI_H__
#define __EXTI_H__

/** Includes **********************************************************************************************************/
#include <stdint.h>

/** Маски триггеров ***************************************************************************************************/
#define EXTI_TRIGGER_RISING         (0x01)
#define EXTI_TRIGGER_FALLING        (0x02)
#define EXTI_TRIGGER_RISING_FALLING (0x03)

/** Перечисление портов GPIO, на которых возможно использование внешних прерываний, в RM п. 9.2.3 SYSCFG_EXTICR[x] ****/
typedef enum
{
    EXTI_PortA = 0x00,
    EXTI_PortB = 0x01,
    EXTI_PortC = 0x02,
    EXTI_PortD = 0x03,
    EXTI_PortE = 0x04,
    EXTI_PortF = 0x05,
    EXTI_PortG = 0x06,
    EXTI_PortH = 0x07,
    EXTI_PortI = 0x08
}EXTI_Port;

/** Functions *********************************************************************************************************/

	/**
	! Функция EXTI_Enable_Pin включает обработку внешних прерываний на выбранном
		пине и задает триггер, который будет вызывать прерывание.
	- EXTI_port - порт GPIO
	- EXTI_pin - пин GPIO
	- EXTI_trigger - триггер, вызывающий прерывание
	*/
void EXTI_Enable_Pin(
	EXTI_Port	EXTI_port,
	uint32_t	EXTI_pin,
	uint32_t	EXTI_trigger
);

	/**
	! Функция EXTI_Disable_Pin выключает обработку внешних прерываний на
		выбранном пине.
	- EXTI_port - порт GPIO
	- EXTI_pin - пин GPIO
	*/
void EXTI_Disable_Pin(
	EXTI_Port	EXTI_port,
	uint32_t	EXTI_pin
);

/**
	! Функция EXTI_Clear_Flag сбрасывает флаг ожидания на выбранном пине.
		Функция вызывается в обработчиках внешних прерываний.
	- EXTI_pin - пин GPIO
*/
void EXTI_Clear_Flag(uint32_t EXTI_pin);

/** Обработчики прерываний EXTI (для обработки внешних прерываний на выводах 0-15 нужны обработчики EXTIx_IRQHandler) */

// Обработчик внешнего прерывания на выводах Px0, где x => GPIOx
void EXTI0_IRQHandler(void);

#endif /*__EXTI_H__ */