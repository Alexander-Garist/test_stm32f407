// LED.h
// библиотека для пользования встроенными светодиодами
// прототипы функций включения/выключения, смена режима моргания

#ifndef __LED_H__
#define __LED_H__

/** Выводы GPIO к которым подключены светодиоды на плате **************************************************************/

// Зеленый LED
#define GPIO_PORT_LED_0	GPIOD
#define GPIO_PIN_LED_0	12

// Оранжевый LED
#define GPIO_PORT_LED_1	GPIOD
#define GPIO_PIN_LED_1	13

// Красный LED
#define GPIO_PORT_LED_2	GPIOD
#define GPIO_PIN_LED_2	14

// Синий LED
#define GPIO_PORT_LED_3	GPIOD
#define GPIO_PIN_LED_3	15

/** Периоды моргания светодиодов в 3 режимах **************************************************************************/

#define TIME_SHORT      100
#define TIME_MEDIUM     200
#define TIME_LONG       500

// Период моргания должен быть доступен в main.c
extern uint32_t Blink_Period;

// Счетчики морганий должны быть доступны в main.c
extern uint32_t Green_Blinks_Counter;
extern uint32_t Orange_Blinks_Counter;
extern uint32_t Red_Blinks_Counter;
extern uint32_t Blue_Blinks_Counter;

// Текущий режим моргания должен быть доступен в main.c
extern uint32_t Blink_Mode;

// Установить новый период моргания
void LED_Set_Blink_Period(uint32_t time);

// Смена режима моргания
void LED_change_blink_mode(void (*callback)(uint32_t));

// Включить все LED
void LED_turnON_4_LED(void);

// Выключить все LED
void LED_turnOFF_4_LED(void);

#endif /* __LED_H__ */