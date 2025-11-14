#include "systick.h"

volatile uint32_t systick_counter = 0;      //Счетчик вызовов SysTick_Handler()

//Статические функции, нужны для сокрытия их реализации в других модулях
static void SysTick_init(uint32_t freq)                                     //Инициализация SysTick
{
    SysTick->CTRL = 0;  //Начальное обнуление
    SysTick->LOAD = (SystemCoreClock / 1000) * freq - 1;  //аргумент freq - желаемый интервал генерации прерывания SysTick_Handler() в миллисекундах
                                                    //например SysTick_init(10); означает, что каждые 10 мс будет вызываться SysTick_Handler()
    SysTick->VAL = 0;   //Начальное обнуление

    SysTick->CTRL |= (1 << 2)       //Выбор clock source
        |(1 << 1)                   //Разрешение вызывать обработчик прерывания SysTick_Handler() при достижении 0 в счетчике
        |(1 << 0);                  //ENABLE
}

//Функции для использования в других модулях
void SysTick_Enable(uint32_t freq)
{
    SysTick_init(freq);
}
void SysTick_Handler(void)                                                  //Обработчик прерывания SysTick, вызывается каждый раз когда SysTick->VAL становится 0
{
    systick_counter++;
}
void delay_ms(uint32_t ms)                                                  //Блокирующая задержка
{
    uint32_t startTime = systick_counter;
    while((systick_counter - startTime) < ms){} //подождать ms миллисекунд
}






uint32_t get_current_time(void)                                             //Получить текущее системное время после RESET
{
    return systick_counter;     //Количество произошедших вызовов SysTick_Handler()
}
uint32_t is_time_passed(uint32_t start_time, uint32_t delay_time)           //Проверка прошло ли время delay_time после момента start_time
{
    return(systick_counter - start_time) >= delay_time;
}

