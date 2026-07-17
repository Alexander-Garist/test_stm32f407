#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "image_processing.h"

/*********** Использовал раньше при обработке исходного кадра отдельно, не на лету, теперь это не используется ********/
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
void ImageProcessing_binarize_adaptive_local(uint8_t *buffer, int width, int height)
{
    if (buffer == NULL || width <= 0 || height <= 0) return;

    // Коэффициент инерции (масштаб сглаживания).
    // Значение 4 означает, что среднее адаптируется примерно за 16 пикселей.
    const uint32_t s = 4;

    // Порог чувствительности в процентах.
    // Значение 15 означает: пиксель станет черным, если он темнее фона вокруг минимум на 15%.
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


/**************************** Функции для работы с образцом, записанным в Flash сектор 11 0x080E0000 ******************/
#include "flash.h"

/** Функция поиска значения отдельного пикселя по адресу (x,y) в массиве, где 8 пикселей упакованы в 1 байт */
static inline uint8_t get_bit_pixel(uint8_t *packed_buffer, uint32_t x, uint32_t y, uint32_t frame_width)
{
    uint32_t pixel_index = y * frame_width + x;       // Индекс искомого пикселя, если представить матрицу как одномерный массив
    uint32_t byte_index = pixel_index >> 3;              // Номер байта, в котором лежит этот пиксель
    uint8_t bit_position = 7 - (pixel_index % 8);          // Позиция бита внутри байта (от 7 до 0)

    // Значения этого бита 1 или 0 - пиксель белый или черный
    return (packed_buffer[byte_index] >> bit_position) & 1;
}

/** Функция сравнения упакованного текущего кадра с эталоном во Flash
*        Возвращает процент совпадения черных сегментов (от 0.0 до 1.0) */
float ImageProcessing_compare_packed_with_tolerance(uint8_t *current_packed, uint32_t example_address, uint32_t width, uint32_t height)
{
    uint8_t *example_frame = (uint8_t*)example_address;

    uint32_t total_ideal_black_pixels = 0;
    uint32_t matched_black_pixels = 0;

    // Радиус окна допуска на сдвиг в пикселях (±2 пикселя по горизонтали)
    const int R = 2;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if (get_bit_pixel(example_frame, x, y, width) == 0)
            {
                total_ideal_black_pixels++;

                uint8_t found_in_current = 0;

                int x_start = (x - R < 0) ? 0 : x - R;
                int x_end   = (x + R >= width) ? width - 1 : x + R;

                for (int wx = x_start; wx <= x_end; wx++)
                {
                    if (get_bit_pixel(current_packed, wx, y, width) == 0)
                    {
                        found_in_current = 1;
                        break;
                    }
                }
                if (found_in_current)
                {
                    matched_black_pixels++;
                }
            }
        }
    }

    if (total_ideal_black_pixels == 0) return 0.0f;

    // Итоговый коэффициент совпадения черных элементов
    float match_score = (float)matched_black_pixels / (float)total_ideal_black_pixels;
    return match_score;
}
/**********************************************************************************************************************/


/****************************** БПФ ***********************************************************************************/

// Таблица поворотных коэффициентов W_32^r для r от 0 до 15
const double complex W32[16] =
{
    1.000000 + 0.000000 * I,  // W^0
    0.980785 - 0.195090 * I,  // W^1
    0.923880 - 0.382683 * I,  // W^2
    0.831470 - 0.555570 * I,  // W^3
    0.707107 - 0.707107 * I,  // W^4
    0.555570 - 0.831470 * I,  // W^5
    0.382683 - 0.923880 * I,  // W^6
    0.195090 - 0.980785 * I,  // W^7
    0.000000 - 1.000000 * I,  // W^8
   -0.195090 - 0.980785 * I,  // W^9
   -0.382683 - 0.923880 * I,  // W^10
   -0.555570 - 0.831470 * I,  // W^11
   -0.707107 - 0.707107 * I,  // W^12
   -0.831470 - 0.555570 * I,  // W^13
   -0.923880 - 0.382683 * I,  // W^14
   -0.980785 - 0.195090 * I   // W^15
};

// Таблица поворотных коэффициентов W_32^r для r от 0 до 31
const double complex W64[32] =
{
    1.0000 + 0.0000 * I,  // W_64^0  // W32^0  // W16^0  // W8^0    // W4^0             (чисто +1)
    0.9952 - 0.0980 * I,  // W_64^1
    0.9808 - 0.1951 * I,  // W_64^2  // W32^1
    0.9569 - 0.2903 * I,  // W_64^3
    0.9239 - 0.3827 * I,  // W_64^4  // W32^2  // W16^1
    0.8819 - 0.4714 * I,  // W_64^5
    0.8315 - 0.5556 * I,  // W_64^6  // W32^3
    0.7730 - 0.6344 * I,  // W_64^7
    0.7071 - 0.7071 * I,  // W_64^8  // W32^4  // W16^2  // W8^1
    0.6344 - 0.7730 * I,  // W_64^9
    0.5556 - 0.8315 * I,  // W_64^10 // W32^5
    0.4714 - 0.8819 * I,  // W_64^11
    0.3827 - 0.9239 * I,  // W_64^12 // W32^6  // W16^3
    0.2903 - 0.9569 * I,  // W_64^13
    0.1951 - 0.9808 * I,  // W_64^14 // W32^7
    0.0980 - 0.9952 * I,  // W_64^15
    0.0000 - 1.0000 * I,  // W_64^16 // W32^8  // W16^4  // W8^2     // W4^1                  (чисто мнимое -j)
   -0.0980 - 0.9952 * I,  // W_64^17
   -0.1951 - 0.9808 * I,  // W_64^18 // W32^9
   -0.2903 - 0.9569 * I,  // W_64^19
   -0.3827 - 0.9239 * I,  // W_64^20 // W32^10  // W16^5
   -0.4714 - 0.8819 * I,  // W_64^21
   -0.5556 - 0.8315 * I,  // W_64^22 // W32^11
   -0.6344 - 0.7730 * I,  // W_64^23
   -0.7071 - 0.7071 * I,  // W_64^24 // W32^12  // W16^6  // W8^3
   -0.7730 - 0.6344 * I,  // W_64^25
   -0.8315 - 0.5556 * I,  // W_64^26 // W32^13
   -0.8819 - 0.4714 * I,  // W_64^27
   -0.9239 - 0.3827 * I,  // W_64^28 // W32^14  // W16^7
   -0.9569 - 0.2903 * I,  // W_64^29
   -0.9808 - 0.1951 * I,  // W_64^30 // W32^15
   -0.9952 - 0.0980 * I   // W_64^31
};


// Функция инверсии 5 бит для N=32
static unsigned int bit_reverse_5bit(unsigned int x)
{
    unsigned int rev = 0;
    for (int i = 0; i < 5; i++)
    {
        rev = (rev << 1) | (x & 1);
        x >>= 1;
    }
    return rev;
}

// Функция инверсии 6 бит для N=64
static unsigned int bit_reverse_6bit(unsigned int x)
{
    unsigned int rev = 0;
    for (int i = 0; i < 6; i++)
    {
        rev = (rev << 1) | (x & 1);
        x >>= 1;
    }
    return rev;
}

// БПФ для 32 точек
void fft32(const double complex input[32], double complex output[32])
{
    // 1. Сначала делаем правильную перестановку во временный массив
    double complex temp[32];
    for (int i = 0; i < 32; ++i)
    {
        temp[bit_reverse_5bit(i)] = input[i];
    }

    // 2. Поэтапный расчет бабочек
    for (int stage = 1; stage <= 5; ++stage)
    {
        int len = 1 << stage;          // Длина текущего подблока (2, 4, 8, 16, 32)
        int half = len >> 1;           // Шаг бабочки (1, 2, 4, 8, 16)
        int twiddle_step = 32 / len;   // Шаг по таблице коэффициентов

        // Пробегаем по всему массиву с шагом размера блока
        for (int i = 0; i < 32; i += len)
        {
            for (int j = 0; j < half; ++j)
            {
                int top_idx = i + j;
                int bot_idx = i + j + half;

                // Вычисление индекса коэффициента r
                int r = j * twiddle_step;
                double complex W = W32[r];

                // Важно: берем значения из temp, но записываем результаты в ОДИНАКОВЫЙ буфер аккуратно
                double complex A = temp[top_idx];
                double complex B_turned = temp[bot_idx] * W;

                output[top_idx] = A + B_turned;
                output[bot_idx] = A - B_turned;
            }
        }

        // Копируем результаты текущего этапа назад в temp для следующего этапа
        for (int i = 0; i < 32; ++i)
        {
            temp[i] = output[i];
        }
    }
}

// БПФ для 64 точек
void fft64(const double complex input[64], double complex output[64])
{
    // 1. Сначала делаем правильную перестановку во временный массив
    double complex temp[64];
    for (int i = 0; i < 64; ++i)
    {
        temp[bit_reverse_6bit(i)] = input[i];
    }

    // 2. Поэтапный расчет бабочек
    for (int stage = 1; stage <= 6; ++stage)    // 'этапов Log2(64)=6' (2, 4, 8, 16, 32.   64)
    {
        int len = 1 << stage;          // Длина текущего подблока (2, 4, 8, 16, 32.   64)
        int half = len >> 1;           // Шаг бабочки (1, 2, 4, 8, 16.   32)
        int twiddle_step = 64 / len;   // Шаг по таблице коэффициентов

        // Пробегаем по всему массиву с шагом размера блока
        for (int i = 0; i < 64; i += len)
        {
            for (int j = 0; j < half; ++j)
            {
                int top_idx = i + j;
                int bot_idx = i + j + half;

                // Вычисление индекса коэффициента r
                int r = j * twiddle_step;
                double complex W = W64[r];

                // Важно: берем значения из temp, но записываем результаты в ОДИНАКОВЫЙ буфер аккуратно
                double complex A = temp[top_idx];
                double complex B_turned = temp[bot_idx] * W;

                output[top_idx] = A + B_turned;
                output[bot_idx] = A - B_turned;
            }
        }

        // Копируем результаты текущего этапа назад в temp для следующего этапа
        for (int i = 0; i < 64; ++i)
        {
            temp[i] = output[i];
        }
    }
}

/**********************************************************************************************************************/
