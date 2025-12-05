/**
  * @file    AD9833.c
  * @brief   Файл содержит реализации функций AD9833
  */

/** Includes **********************************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "AD9833.h"
#include "systick.h"
#include "gpio.h"
#include "usart.h"

/** Defines ***********************************************************************************************************/

#define MAX_COMMANDS		10
#define MAX_COMMAND_SIZE	15

/** Variables *********************************************************************************************************/

char USART_Commands[MAX_COMMANDS][MAX_COMMAND_SIZE];	// Список команд для выполнения, до 10 команд в очереди
uint32_t Amount_of_Commands = 0;						// Текущее количество команд в списке для выполнения

/*********************************************** Функции **************************************************************/

/***************************************** Статические функции ********************************************************/

	/**
	! Запись 16-битного слова в AD9833.
	- SPIx - SPI модуль (SPI1, SPI2, SPI3).
	- data - 16-битные данные для записи.
	*/
static void AD9833_Write(SPI_TypeDef* SPIx, uint16_t data)
{
    uint8_t spi_data[2];

    // Преобразование 16-битного слова в два байта (старший первый)
    spi_data[0] = (data >> 8) & 0xFF;
    spi_data[1] = data & 0xFF;

    // Включение FSY (низкий уровень)
    GPIO_set_LOW(AD9833_FSY_PORT, AD9833_FSY_PIN);

    // Отправка данных через SPI
    SPI_Transmit(SPIx, spi_data, 2);

    // Выключение FSY (высокий уровень)
    GPIO_set_HIGH(AD9833_FSY_PORT, AD9833_FSY_PIN);
}

	/**
	! Запись значения в цифровой потенциометр MCP41010.
	- SPIx - SPI модуль (SPI1, SPI2, SPI3)
	- value - значение амплитуды (0-255)
	*/
static void MCP41010_Write(SPI_TypeDef* SPIx, uint8_t value)
{
    uint8_t spi_data[2];
    uint16_t command = MCP41010_CMD_WRITE | value;	// Значение амплитуды записывается во второй байт данных, в первом остается команда 0x11

    // Преобразование команды в два байта
    spi_data[0] = (command >> 8) & 0xFF;
    spi_data[1] = command & 0xFF;

    // Включение CS (низкий уровень)
    GPIO_set_LOW(MCP41010_CS_PORT, MCP41010_CS_PIN);

    // Отправка данных через SPI
    SPI_Transmit(SPIx, spi_data, 2);

    // Выключение CS (высокий уровень)
    GPIO_set_HIGH(MCP41010_CS_PORT, MCP41010_CS_PIN);
}

/***************************************** Публичные функции **********************************************************/

// Включение пинов GPIO, которые управляют модулем генератора сигналов
void AD9833_GPIO_Init(void)
{
	GPIO_set_HIGH(AD9833_FSY_PORT, AD9833_FSY_PIN);		// PA3 -> FSY
	GPIO_set_HIGH(MCP41010_CS_PORT, MCP41010_CS_PIN);	// PA4 -> CS
}

// Инициализация модуля генератора сигналов, установка параметров сигнала, заданных в структуре параметров
void AD9833_Module_Init(SPI_TypeDef* SPIx, Signal_Parameters* signal_struct)
{
	AD9833_GPIO_Init();
	AD9833_Reset(SPIx);

	// Настройка частоты выходного сигнала
	//AD9833_Write(SPIx, 0x2000); // Управляющий код 0010 0000 0000 0000 => далее 2 14-битных слова частоты
	AD9833_SetFrequency(SPIx, signal_struct->frequency);

	// Настройка амплитуды выходного сигнала
	AD9833_SetAmplitude(SPIx, signal_struct->amplitude);

	// Настройка формы выходного сигнала
	AD9833_SetOutputMode(SPIx, signal_struct->wave_form);

	// Включение выходного сигнала
	AD9833_EnableOutput(SPIx, 1);
}

// Установка частоты выходного сигнала
void AD9833_SetFrequency(SPI_TypeDef* SPIx, uint32_t frequency)
{
    uint32_t freq_word;
    // Расчет значения частоты: FREQ = (f * 2^28) / MCLK
    freq_word = (uint32_t)((frequency * 268435456.0) / MCLK_FREQUENCY);

	AD9833_Write(SPIx, 0x2000); // Управляющий код 0010 0000 0000 0000 => далее 2 14-битных слова частоты
    // Запись в регистр частоты FREQ0 (14-битные фрагменты)
    AD9833_Write(SPIx, AD9833_FREQ0_REG | (freq_word & 0x3FFF));         // LSB
	AD9833_Write(SPIx, AD9833_FREQ0_REG | ((freq_word >> 14) & 0x3FFF)); // MSB
}

// Установка амплитуды выходного сигнала
void AD9833_SetAmplitude(SPI_TypeDef* SPIx, uint8_t amplitude)
{
    MCP41010_Write(SPIx, amplitude);
}

// Установка формы выходного сигнала
void AD9833_SetOutputMode(SPI_TypeDef* SPIx, uint16_t mode)
{
    AD9833_Write(SPIx, mode);
}

// Сброс внутренних регистров AD9833. Регистры фазы, частоты и управления НЕ СБРАСЫВАЮТСЯ
void AD9833_Reset(SPI_TypeDef* SPIx)
{
    AD9833_Write(SPIx, AD9833_RESET_CMD);
    delay_ms(10);
    AD9833_Write(SPIx, 0x0000); // Снять сброс
	delay_ms(10);
}

// Включение/выключение выходного сигнала
void AD9833_EnableOutput(SPI_TypeDef* SPIx, uint8_t enable)
{
    if(enable) {
        AD9833_Write(SPIx, AD9833_FREQ0_REG);	// Включить выход
    } else {
        AD9833_Write(SPIx, AD9833_SLEEP1_CMD);	// Выключить выход (режим сна)
    }
}

/************************ Функции для управления генератором сигнала с использованием USART ***************************/

// Вывести список команд для генератора
void AD9833_Print_List_of_Commands()
{
    printf("Actual list of commands:\n");
    for (int i = 0; i < Amount_of_Commands; i++)
    {
        printf("command_number %d: ", i);
        printf("%s\n", USART_Commands[i]);
    }
}

// Вывести текущие параметры сигнала
void AD9833_print_Signal_Parameters(Signal_Parameters* signal_struct)
{
	printf("\nCURRENT SIGNAL PARAMETERS\n");
	printf("FREQUENCY: %d\n", signal_struct->frequency);
	printf("AMPLITUDE: %d\n", signal_struct->amplitude);
	switch (signal_struct->wave_form)
	{
		case 0: printf("WAVE_FORM: SINUSOIDA\n"); break;
		case 1: printf("WAVE_FORM: SQUARE\n"); break;
		case 2: printf("WAVE_FORM: TRIANGLE\n"); break;
	}
	printf("\n");
}

// Извлечь из исходного буфера USART команды для генератора без мусора, исходный буфер очистить
void AD9833_filter_Buffer_USART(char* original_buffer, char* filtered_buffer)
{
	// Создается новый буфер, в котором уже точно не будет мусора
    uint32_t BUFFER_SIZE = 0;

	// Проверяется каждый полученный байт, является он цифрой или символом-разделителем
    for (int i = 0; original_buffer[i] != '!'; i++)
    {
        if ((original_buffer[i] >= '0') && (original_buffer[i] <= ';'))
        {
            filtered_buffer[BUFFER_SIZE] = original_buffer[i];
            BUFFER_SIZE++;
        }
    }

	// Отфильтрованный от мусора буфер заканчивается '\0' т.к. он может быть короче исходного
    filtered_buffer[BUFFER_SIZE] = '\0';

    // Исходный буфер очищается после того, как из него скопировали данные
    USART_clear_Buffer(original_buffer);
}

// Извлечь команды генератора из буфера USART и поместить их в список для выполнения
void AD9833_Parse_Commands_From_Buffer_USART(char* buffer)
{
    // Подсчет команд в буфере, которые можно распознать
	// Команда должна заканчиваться ';', если нет ';', то нельзя гарантировать, что команда получена полностью
    for (int i = 0; i < strlen(buffer); i++)
    {
        if (buffer[i] == ';') Amount_of_Commands++;
    }

    int buffer_index = 0;   // Индекс для прохода по одномерному массиву BUFFER
    int command_index = 0;  // Индекс для записи в двумерный массив USART_Commands

    // Команды из одномерного массива BUFFER переписываются в двумерный массив USART_Commands
    for (int command_number = 0; command_number < Amount_of_Commands; command_number++) // command_number - номер команды в массиве команд
    {
        command_index = 0;
        while (buffer[buffer_index] != ';')
        {
            USART_Commands[command_number][command_index] = buffer[buffer_index];
            command_index++;
            buffer_index++;
        }
        USART_Commands[command_number][command_index] = '\0';	// Каждая команда заканчивается '\0'
        buffer_index++;											// buffer_index увеличивается чтобы проскочить ';'
    }

    // Буфер очищается после извлечения команд
    USART_clear_Buffer(buffer);
}

// Извлечь параметры из команды, которая первая в списке, выполнить ее, удалить ее из списка
void AD9833_Execute_Command(USART_TypeDef* USARTx, Signal_Parameters* signal_params)
{
	// Вывод актуальной очереди команд
	//AD9833_Print_List_of_Commands();

	// Массивы символов для извлечения параметров сигнала
	// Размеры массивов определяются максимальными значениями частоты, амплитуды и формы сигнала
    char frequency[9];  // Максимум 12500000 + \0
    char amplitude[4];  // Максимум 255      + \0
    char wave_form[2];  // Максимум 2        + \0

    uint32_t command_index = 0;		// Индекс прохода по строке команды
    uint32_t param_index = 0;		// Индекс в строке отдельного параметра

    // Частота
    while (USART_Commands[0][command_index] != ':')
    {
        frequency[param_index] = USART_Commands[0][command_index];
        command_index++;
        param_index++;
    }
    frequency[param_index] = '\0';
    param_index = 0;
    command_index++;

    // Амплитуда
    while (USART_Commands[0][command_index] != ':')
    {
        amplitude[param_index] = USART_Commands[0][command_index];
        command_index++;
        param_index++;
    }
    amplitude[param_index] = '\0';
    param_index = 0;
    command_index++;

    // Форма сигнала
    wave_form[0] = USART_Commands[0][command_index];
    wave_form[1] = '\0';

/**** Вывод в терминал для отладки. Закомментированные строки кода раскомментировать при необходимости отладки. *******/
/**
    // Если данные в команде корректные, выполнить команду, если нет, выдать ERROR
    printf("\nExecuting command:");
    printf("%s\n", USART_Commands[0]);
*/
	// Проверка корректности частоты (должна быть в пределах от 1 до 12500000)
    if ((atoi(frequency) > 0) && (atoi(frequency) <= 12500000))
    {
        // printf("       Frequency changed to %d\n", atoi(frequency));
        signal_params->frequency = atoi(frequency);
    }
    // else printf("ERROR: Frequency not changed\n");

	// Проверка корректности амплитуды (должна быть в пределах от 1 до 255)
    if ((atoi(amplitude) > 0) && (atoi(amplitude) < 256))
    {
        // printf("       Amplitude changed to %d\n", atoi(amplitude));
        signal_params->amplitude = atoi(amplitude);
    }
    // else printf("ERROR: Amplitude not changed\n");

	// Проверка корректности формы сигнала (может быть 0, 1 или 2)
    if ((atoi(wave_form) >= 0) && (atoi(wave_form) < 3))
    {
        //printf("       Wave_form changed to wave_form number %d\n\n", atoi(wave_form));
		if (atoi(wave_form) == 0)	signal_params->wave_form = AD9833_SIN_MODE;
		if (atoi(wave_form) == 1)	signal_params->wave_form = AD9833_SQUARE_MODE;
		if (atoi(wave_form) == 2)	signal_params->wave_form = AD9833_TRIANGLE_MODE;

    }
    //else printf("ERROR: Wave_form not changed\n\n");

	// Отправка выполненной команды обратно по USART с добавкой строки "Command executed: " и добавкой символов '\r' '\n'

	USART_Transmit(USARTx, "Command executed: ", strlen("Command executed: "));
	for (int i = 0; i < strlen(USART_Commands[0]); i++)
	{
		USART_Transmit(USARTx, &USART_Commands[0][i], 1);
	}
	USART_Transmit(USARTx, "\r\n", 2);

/**** Вывод в терминал для отладки. Закомментированные строки кода раскомментировать при необходимости отладки. *******/

	// Удаление выполненной команды из очереди
    //printf("Deleting command number 0\n");

    // Команда USART_Commands[0] выполнена, теперь команда удаляется из очереди, остальные команды сдвигаются.
    for (int i = 0; i < Amount_of_Commands - 1; i++)
    {
        int index = 0;
        while (USART_Commands[i + 1][index] != '\0')
        {
            USART_Commands[i][index] = USART_Commands[i + 1][index];
            index++;
        }
        USART_Commands[i][index] = '\0';
    }
    Amount_of_Commands--;	// Количество команд в очереди уменьшается

	if (!Amount_of_Commands)	// если заклнчились все команды в очереди, очередь нужно очистить полностью
	{
		for (int i = 0; i < MAX_COMMANDS; i++)
		{
			USART_Commands[i][0] = '\0';
		}
	}
	// Вывод актуальной очереди команд после удаления только что удаленной
    //AD9833_Print_List_of_Commands();
}