/**
  * @file    systick.c
  * @brief   Файл содержит реализации функций системного таймера SysTick
  */

/** Includes **********************************************************************************************************/
#include "systick.h"
#include "CMSIS/stm32f4xx.h"

/** Defines ***********************************************************************************************************/
#define SysTick_FREQUENCY	SystemCoreClock                     // Тактовая частота (168 МГц) => 1с == 168.000.000 тактов
#define ms_per_interrupt	50									// Период прерываний SysTick в мс
#define ticks_per_ms		(SysTick_FREQUENCY / 1000)			// Количество тактов за 1 мс (168.000)
#define LOAD_max_val		(ticks_per_ms * ms_per_interrupt)	// Значение LOAD (8.400.000)

/** Variables *********************************************************************************************************/
volatile uint32_t systick_counter = 0;      // Счетчик вызовов SysTick_Handler()
static uint32_t ms_counter = 0;				// Счетчик миллисекунд

/** Functions *********************************************************************************************************/

// Инициализая системного таймера
void SysTick_Init()
{
    SysTick->CTRL = 0;
    /** SysTick->LOAD = ticks_per_ms - 1;   // В этом случае прерывание вызывается каждую миллисекунду */
    SysTick->LOAD = LOAD_max_val - 1;       // Прерывание вызывается каждые 50 мс
    SysTick->VAL = 0;

	// Установка регистра управления и статуса
    SysTick->CTRL |= (0x1 << 2)     // Выбор clock source
                    |(0x1 << 1)     // SysTick_Handler() вызывается при достижении 0 в счетчике (значит прошло 100 мс)
                    |(0x1 << 0);    // Включение
}

// Обновление счетчика миллисекунд
static void SysTick_Update_ms(void)
{
    static uint32_t accumulated_ticks = 0;
    uint32_t current_val = SysTick->VAL;

    // Вместо магического числа используем статический флаг для первой инициализации
    static uint32_t last_VAL = 0;
    static uint8_t is_first_run = 1;

    // В этот блок программа зайдет только 1 раз, чтобы установить last_VAL
    if (is_first_run)
    {
        last_VAL = current_val;
        is_first_run = 0;
        return;
    }

    uint32_t elapsed_ticks;

    if (current_val <= last_VAL)
    {
        elapsed_ticks = last_VAL - current_val;
    }
    else
    {
        elapsed_ticks = (LOAD_max_val - current_val) + last_VAL;
    }

    accumulated_ticks += elapsed_ticks;
    last_VAL = current_val;

    // Обновление счетчика мс
    if (accumulated_ticks >= ticks_per_ms)
    {
        uint32_t ms_to_add = accumulated_ticks / ticks_per_ms;
        ms_counter += ms_to_add;
        accumulated_ticks %= ticks_per_ms;
    }
}

// Получение текущего системного времени в мс
uint32_t get_current_ms(void)
{
	SysTick_Update_ms();
	return ms_counter;
}

// Неблокирующая задержка в мс
uint32_t is_time_passed_ms(uint32_t start_time_ms, uint32_t delay_time_ms)
{
	SysTick_Update_ms();
	return (ms_counter - start_time_ms) >= delay_time_ms;
}

// Блокирующая задержка в мс
void delay_ms(uint32_t ms)
{
    uint32_t startTime = ms_counter;
    while ((ms_counter - startTime) < ms)
	{
		SysTick_Update_ms();
	}
}

// Обработчик прерываний системного таймера
void SysTick_Handler(void)
{
    systick_counter++;
}

void Clock_Config_168MHz_HSI(void)
{
    // 1. Включаем HSI и ждем стабилизации
    RCC->CR |= RCC_CR_HSION;
    while(!(RCC->CR & RCC_CR_HSIRDY));

    // 2. Настраиваем Power Control для высокой частоты (VOS = 1)
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR |= PWR_CR_VOS;

    // 3. Настраиваем Flash Latency (5 тактов ожидания для 168 МГц)
    // А также включаем Prefetch и кэши инструкций/данных для скорости
    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_5WS;

    // 4. Настройка делителей шин (AHB, APB1, APB2)
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;  // AHB = 168 МГц
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV2; // APB2 = 84 МГц (макс)
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV4; // APB1 = 42 МГц (макс)

    // 5. Конфигурация PLL: M=16, N=336, P=2 (00), Q=7
    // Сбрасываем и записываем заново
    RCC->PLLCFGR = (16 << RCC_PLLCFGR_PLLM_Pos) |
                   (336 << RCC_PLLCFGR_PLLN_Pos) |
                   (0 << RCC_PLLCFGR_PLLP_Pos) |   // 00 означает делитель 2
                   (7 << RCC_PLLCFGR_PLLQ_Pos) |   // Для USB/SDIO
                   RCC_PLLCFGR_PLLSRC_HSI;         // Источник HSI

    // 6. Включаем PLL и ждем фиксации (Lock)
    RCC->CR |= RCC_CR_PLLON;
    while(!(RCC->CR & RCC_CR_PLLRDY));

    // 7. Переключаем System Clock на PLL
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    // 8. Обновляем глобальную переменную (для функций задержки)
    SystemCoreClockUpdate();
}
