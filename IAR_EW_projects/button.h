// button.h
#ifndef __BUTTON_H__
#define __BUTTON_H__

#include <stdint.h>


/** Триггер внешнего прерывания и задержка для фильтрации дребезга при нажатии кнопки *********************************/
#define BUTTON_TRIGGER  EXTI_TRIGGER_FALLING
#define DEBOUNCE_TIME   100

extern uint8_t button_state;						// Состояние кнопки нажата/не нажата
extern uint32_t button_last_time;					// Время последнего нажатия

#endif /* __BUTTON_H__ */