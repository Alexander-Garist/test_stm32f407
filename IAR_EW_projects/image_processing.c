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
float ImageProcessing_Compare_by_Convolution(uint8_t* example_array, uint8_t* array, float* deviation_X, float* deviation_Y, uint32_t size)
{
    float correlation_coefficient;

    float average_X = get_average_value(example_array, size);   // Среднее значение исходного массива X (образец сравнения)
    float average_Y = get_average_value(array, size);           // Среднее значение исходного массива X (сравниваемый кадр)

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





/**************************************** доработать запись образца в Flash *******************************************/
float ImageProcessing_compare_packed_with_tolerance(uint8_t *current_packed, int width, int height)
{
    // Указываем, что наш идеальный упакованный образец лежит строго по адресу Сектора 11 во Flash
    uint8_t *ideal_packed = (uint8_t*)FLASH_SAMPLE_ADDR;

    uint32_t total_ideal_black_pixels = 0;
    uint32_t matched_black_pixels = 0;
    const int R = 2; // Радиус допуска на сдвиг

    // Если во Flash лежит стертый сектор (все байты равны 0xFF), значит образец еще не записан
    if (ideal_packed[0] == 0xFF && ideal_packed[1] == 0xFF)
    {
        return -1.0f; // Возвращаем ошибку: "Запишите образец!"
    }

    // ... (весь остальной ваш двойной цикл for по X и Y с функцией get_bit_pixel остается абсолютно без изменений) ...

    float match_score = (float)matched_black_pixels / (float)total_ideal_black_pixels;
    return match_score;
}









// Функция проверяет наличие битовых пикселей в упакованном массиве в точке (x, y)
// Возвращает 1, если пиксель БЕЛЫЙ, и 0, если пиксель ЧЕРНЫЙ
static inline uint8_t get_bit_pixel(uint8_t *packed_buffer, int x, int y, int width)
{
    // Находим абсолютный индекс пикселя в плоском потоке
    uint32_t pixel_idx = y * width + x;

    // Находим номер байта, в котором лежит этот пиксель
    uint32_t byte_idx = pixel_idx / 8;

    // Находим позицию бита внутри байта (от 7 до 0)
    uint8_t bit_pos = 7 - (pixel_idx % 8);

    // Извлекаем бит
    return (packed_buffer[byte_idx] >> bit_pos) & 1;
}

// Финальная функция сравнения упакованного текущего кадра с эталоном во Flash
// Возвращает процент совпадения черных сегментов (от 0.0 до 1.0)
float ImageProcessing_compare_packed_with_tolerance(uint8_t *current_packed, int width, int height)
{
    // Массив IDEAL_PACKED_FRAME берется напрямую из подключенного h-файла во Flash!
    uint8_t *ideal_packed = (uint8_t*)IDEAL_PACKED_FRAME;

    uint32_t total_ideal_black_pixels = 0;
    uint32_t matched_black_pixels = 0;

    // Радиус окна допуска на сдвиг в пикселях (±2 пикселя по горизонтали)
    const int R = 2;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            // 1. Проверяем пиксель в эталоне (лежит во Flash)
            // Если он равен 0 — значит, здесь по эталону ДОЛЖЕН быть черный сегмент ЖКИ
            if (get_bit_pixel(ideal_packed, x, y, width) == 0)
            {
                total_ideal_black_pixels++; // Считаем общую массу черных линий образца

                // 2. Ищем, есть ли этот черный сегмент в текущем кадре с учетом микросдвига
                uint8_t found_in_current = 0;

                // Сканируем локальное окно соседей по горизонтали [-2...0...+2]
                int x_start = (x - R < 0) ? 0 : x - R;
                int x_end   = (x + R >= width) ? width - 1 : x + R;

                for (int wx = x_start; wx <= x_end; wx++)
                {
                    // Если у со текущего кадра в этом окне нашелся черный бит (0)
                    if (get_bit_pixel(current_packed, wx, y, width) == 0)
                    {
                        found_in_current = 1; // Сегмент успешно обнаружен (даже со сдвигом!)
                        break;
                    }
                }

                if (found_in_current)
                {
                    matched_black_pixels++; // Засчитываем успешное совпадение контура
                }
            }
        }
    }

    if (total_ideal_black_pixels == 0) return 0.0f;

    // Считаем итоговый коэффициент совпадения полезных черных элементов
    float match_score = (float)matched_black_pixels / (float)total_ideal_black_pixels;
    return match_score;
}










