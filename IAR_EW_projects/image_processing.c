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

//void ImageProcessing_binarize_adaptive_local(uint8_t *buffer, int width, int height)
//{
//    if (buffer == NULL || width <= 0 || height <= 0) return;
//
//    // s = 4 (делитель 16) обеспечивает мягкое, плавное отслеживание градиента фона
//    const uint32_t s = 4;
//
//    // Чувствительность в процентах.
//    // Значение 12-15 уберет мелкие волокна фона, но сохранит бледные правые цифры.
//    const uint32_t t = 14;
//
//    for (int y = 0; y < height; y++)
//    {
//        uint32_t row_offset = y * width;
//
//        // --- ПРОХОД 1: СЛЕВА НАПРАВО ---
//        uint32_t running_average_lr = 127 << s;
//        for (int x = 0; x < width; x++)
//        {
//            uint32_t idx = row_offset + x;
//            uint32_t pixel = buffer[idx];
//            running_average_lr = running_average_lr - (running_average_lr >> s) + pixel;
//
//            // Вычисляем порог левого прохода и временно кодируем результат:
//            // Если пиксель светлее порога — помечаем его битом 7 (0x80) в этом же массиве
//            uint32_t threshold = (running_average_lr >> s) * (100 - t) / 100;
//            if (pixel > threshold)
//            {
//                buffer[idx] |= 0x80; // Временно пометили как "белый" для 1 прохода
//            }
//        }
//
//        // --- ПРОХОД 2: СПРАВА НАЛЕВО ---
//        uint32_t running_average_rl = 127 << s;
//        for (int x = width - 1; x >= 0; x--)
//        {
//            uint32_t idx = row_offset + x;
//
//            // Очищаем старший бит, чтобы восстановить оригинальное значение яркости
//            uint32_t original_pixel = buffer[idx] & 0x7F;
//
//            running_average_rl = running_average_rl - (running_average_rl >> s) + original_pixel;
//            uint32_t threshold = (running_average_rl >> s) * (100 - t) / 100;
//
//            // ФИНАЛЬНОЕ РЕШЕНИЕ: пиксель признается БЕЛЫМ только если ОБА прохода
//            // (и слева, и справа) посчитали его белым фоном.
//            if ((buffer[idx] & 0x80) && (original_pixel > threshold))
//            {
//                buffer[idx] = 255; // Идеально чистый белый фон
//            }
//            else
//            {
//                buffer[idx] = 0;   // Монолитный черный контур символа
//            }
//        }
//    }
//}
