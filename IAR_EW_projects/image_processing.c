#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "image_processing.h"

// Поиск минимального и максимального значения в массиве
static void Find_MIN_MAX_value(uint8_t *buffer, uint32_t size, uint8_t* min_val, uint8_t* max_val)
{
    *min_val = 255;
    *max_val = 0;

    // Поиск реального минимума и максимума яркости в кадре
    for (uint32_t i = 0; i < size; i++)
    {
        if (buffer[i] < *min_val) *min_val = buffer[i];
        if (buffer[i] > *max_val) *max_val = buffer[i];
    }
}

/** Увеличить контрастность темного изображения */
void ImageProcessing_increase_image_contrast(uint8_t *buffer, uint32_t size)
{
    if (size == 0 || buffer == NULL) return;

    uint8_t min_val, max_val;
    Find_MIN_MAX_value(buffer, size, &min_val, &max_val);

    // Защита от деления на ноль, если кадр абсолютно однотонный
    if (max_val == min_val) return;

    // Вычисление коэффициента растяжения значений яркости
    uint32_t scale = (255 * 256) / (max_val - min_val);

    // Пересчет яркости каждого пикселя по формуле: Новая_Яркость = (Старая_Яркость - Минимум) * Коэффициент_Растяжения
    for (uint32_t i = 0; i < size; i++)
    {
        uint32_t corrected = ((uint32_t)(buffer[i] - min_val) * scale) / 256;

        // Больше 255 яркость получиться не может, но на случай какой-либо ошибки ограничение 255
        if (corrected > 255) corrected = 255;

        // Новое значение яркости сразу записывается в исходный буфер, новая память не выделяется
        buffer[i] = (uint8_t)corrected;
    }
}

/** Бинаризация изображения */
void ImageProcessing_binarize_image(uint8_t *buffer, uint32_t size)
{
    if (size == 0 || buffer == NULL) return;

    uint8_t min_val, max_val;
    Find_MIN_MAX_value(buffer, size, &min_val, &max_val);

    // Порог яркости для бинаризации как середина диапазона яркости
    uint8_t threshold = (uint8_t)((min_val + max_val) / 2);

    // Бинаризация без создания нового массива
    for (uint32_t i = 0; i < size; i++)
    {
        if (buffer[i] > threshold)
        {
            buffer[i] = 255;
        }
        else
        {
            buffer[i] = 0;
        }
    }
}

void ImageProcessing_binarize_adaptive_local(uint8_t *buffer, int width, int height)
{
    if (buffer == NULL || width <= 0 || height <= 0) return;

    // Коэффициент инерции (масштаб сглаживания).
    // Значение 4 означает, что среднее адаптируется примерно за 16 пикселей.
    const uint32_t s = 4;

    // Порог чувствительности в процентах.
    // Значение 15 означает: пиксель станет черным, если он темнее фона вокруг минимум на 15%.
    // Идеально для бледных цифр наклейки на свету.
    const uint32_t t = 15;

    // Стартовое значение среднего (пусть будет середина диапазона 127)
    uint32_t running_average = 127 << s;

    for (int y = 0; y < height; y++)
    {
        uint32_t row_offset = y * width;

        for (int x = 0; x < width; x++)
        {
            uint32_t current_idx = row_offset + x;

            // Запоминаем ОРИГИНАЛЬНУЙ байт яркости камеры
            uint32_t current_pixel = buffer[current_idx];

            // Обновляем скользящее среднее значение фона
            running_average = running_average - (running_average >> s) + current_pixel;

            // Вычисляем порог для данного пикселя на основе текущей истории фона
            uint32_t local_threshold = (running_average >> s) * (100 - t) / 100;

            // Бинаризуем «на месте» в оригинальный массив
            if (current_pixel > local_threshold)
            {
                buffer[current_idx] = 255; // Белый фон
            }
            else
            {
                buffer[current_idx] = 0;   // Черный маркер цифры
            }
        }
    }
}

/**********************************************************************************************************************/
// Поиск среднего значения массива
static float get_average_value(uint8_t* buffer, uint32_t size)
{
    float average_value = 0.0f; // Среднее значение массива данных
    float sum = 0.0f;           // Сумма элементов массива

    for (size_t index = 0; index < size; index++)
    {
        sum += *(buffer + index);
    }

    average_value = (float)sum / (float)size; // Целая часть
    return average_value;
}

// Поиск отклонения от среднего
static float deviation_from_average(uint8_t value, float average)
{
    return (float)value - average;
}

// сумма квадратов отклонений
static float sum_squares_deviations(float* array_deviations)
{
    float sum = 0.0f;

    // массив отклонений заменяется массивом квадратов отклонений
    for (uint32_t index = 0; index < 14; index++)
    {
        sum += array_deviations[index] * array_deviations[index];
    }

    return sum;
}
/**********************************************************************************************************************/

/** Сравнение 2 изображений методом свертки */
float ImageProcessing_Compare_by_Convolution(uint8_t* example_array, uint8_t* array, uint32_t size)         // size пока что 100000, но придется уменьшить
{
    float correlation_coefficient;

    float average_X = get_average_value(example_array, size);   // Среднее значение исходного массива X (образец сравнения)
    float average_Y = get_average_value(array, size);           // Среднее значение исходного массива X (сравниваемый кадр)

    float deviation_X[19200], deviation_Y[19200]; // Массивы отклонений от средних значений

    for (uint32_t index = 0; index < size; index++)
    {
        deviation_X[index] = deviation_from_average(example_array[index], average_X);
    }

    for (uint32_t index = 0; index < size; index++)
    {
        deviation_Y[index] = deviation_from_average(array[index], average_Y);
    }

    //Произведение отклонений
    float numerator = 0.0f; // числитель коэффициента корреляции

    for (uint32_t index = 0; index < 14; index++)
    {
        numerator += deviation_X[index] * deviation_Y[index];
    }

    // подсчет сумм квадратов отклонений
    float sum_squares_X, sum_squares_Y;

    sum_squares_X = sum_squares_deviations(deviation_X);
    sum_squares_Y = sum_squares_deviations(deviation_Y);

    // знаменатель формулы
    float denominator = sqrt(sum_squares_X * sum_squares_Y);

    correlation_coefficient = numerator / denominator;

    return correlation_coefficient;
}

