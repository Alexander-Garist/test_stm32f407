#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "image_processing.h"

// Алгоритм Брэдли с использованием интегральной суммы
void bradley_threshold_integral(const unsigned char* src, unsigned char* res, int width, int height)
{
    const int S = width / 8;
    int s2 = S / 2;
    const float t = 0.15f;
    int index;
    int x1, y1, x2, y2;

    // Выделяется кусок памяти под пиксели (160 * 120 * 4 байта = 76.8 КБ)
    uint32_t* integral_image = (uint32_t*)malloc(width * height * sizeof(uint32_t));

    if (!integral_image) return; // Защита от нехватки памяти на МК


    // Рассчитываем интегральное изображение
    for (int i = 0; i < width; i++)
    {
        uint32_t sum = 0;
        for (int j = 0; j < height; j++)
        {
            index = j * width + i;
            sum += src[index];
            if (i == 0) integral_image[index] = sum;
            else integral_image[index] = integral_image[index - 1] + sum;
        }
    }

    // Пороговая обработка
    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            index = j * width + i;
            x1 = i - s2;
            x2 = i + s2;
            y1 = j - s2;
            y2 = j + s2;

            if (x1 < 0) x1 = 0;
            if (x2 >= width) x2 = width - 1;
            if (y1 < 0) y1 = 0;
            if (y2 >= height) y2 = height - 1;

            int count = (x2 - x1) * (y2 - y1);
            if (count <= 0) count = 1; // Защита от деления на 0

            // Оригинальная формула расчета суммы из вашего кода
            long sum_rect = integral_image[y2 * width + x2]
                          - integral_image[y1 * width + x2]
                          - integral_image[y2 * width + x1]
                          + integral_image[y1 * width + x1];

            if ((long)(src[index] * count) < (long)(sum_rect * (1.0f - t)))
            {
                res[index] = 0;
            }
            else
            {
                res[index] = 255;
            }
        }
    }

    free(integral_image); // Освобождаем кучу
}

// Алгоритм Брэдли без использования интегральной суммы
void bradley_threshold_mcu_fast(const unsigned char* src, unsigned char* res, int width, int height)
{
    const int S = width / 8;
    int s2 = S / 2;
    const float t = 0.15f;

    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            int x1 = i - s2;
            int x2 = i + s2;
            int y1 = j - s2;
            int y2 = j + s2;

            if (x1 < 0) x1 = 0;
            if (x2 >= width) x2 = width - 1;
            if (y1 < 0) y1 = 0;
            if (y2 >= height) y2 = height - 1;

            long sum = 0;
            int count = 0;

            // Прямой обход окна локальной области
            for (int wy = y1; wy <= y2; wy++)
            {
                int row_offset = wy * width;
                for (int wx = x1; wx <= x2; wx++)
                {
                    sum += src[row_offset + wx];
                    count++;
                }
            }

            int index = j * width + i;
            // Стандартная математически корректная формула Брэдли
            if ((long)(src[index] * count) < (long)(sum * (1.0f - t)))
            {
                res[index] = 0;
            } else
            {
                res[index] = 255;
            }
        }
    }
}


#define WIDTH  160
#define HEIGHT 120

//// Функция бинаризации Вульфа без использования динамической памяти (0 байт ОЗУ)
//void wolf_binarization_mcu(const unsigned char *src, unsigned char *dst, int window_size, float k_param)
//{
//    int radius = window_size / 2;
//
//    // 1. Поиск глобального минимума яркости
//    unsigned char min_val = 255;
//    for (int i = 0; i < WIDTH * HEIGHT; i++)
//    {
//        if (src[i] < min_val)
//        {
//            min_val = src[i];
//        }
//    }
//
//    // 2. Первый проход: Поиск максимального стандартного отклонения (R)
//    float max_stddev = 0.0f;
//    for (int y = 0; y < HEIGHT; y++)
//    {
//        for (int x = 0; x < WIDTH; x++)
//        {
//            int sum = 0;
//            int sq_sum = 0;
//            int count = 0;
//
//            int y_start = (y - radius < 0) ? 0 : y - radius;
//            int y_end = (y + radius >= HEIGHT) ? HEIGHT - 1 : y + radius;
//            int x_start = (x - radius < 0) ? 0 : x - radius;
//            int x_end = (x + radius >= WIDTH) ? WIDTH - 1 : x + radius;
//
//            for (int wy = y_start; wy <= y_end; wy++)
//            {
//                int row_offset = wy * WIDTH;
//                for (int wx = x_start; wx <= x_end; wx++)
//                {
//                    unsigned char val = src[row_offset + wx];
//                    sum += val;
//                    sq_sum += val * val; // Сумма квадратов не переполнит 32-битный int
//                    count++;
//                }
//            }
//
//            float mean = (float)sum / count;
//            float variance = ((float)sq_sum / count) - (mean * mean);
//            if (variance < 0) variance = 0.0f;
//            float stddev = sqrtf(variance);
//
//            if (stddev > max_stddev) {
//                max_stddev = stddev;
//            }
//        }
//    }
//
//    float R = max_stddev;
//    if (R == 0.0f) R = 1.0f; // Защита от деления на ноль
//
//    // 3. Второй проход: Локальный расчет порога и запись результата
//    for (int y = 0; y < HEIGHT; y++)
//    {
//        for (int x = 0; x < WIDTH; x++)
//        {
//            int sum = 0;
//            int sq_sum = 0;
//            int count = 0;
//
//            int y_start = (y - radius < 0) ? 0 : y - radius;
//            int y_end = (y + radius >= HEIGHT) ? HEIGHT - 1 : y + radius;
//            int x_start = (x - radius < 0) ? 0 : x - radius;
//            int x_end = (x + radius >= WIDTH) ? WIDTH - 1 : x + radius;
//
//            for (int wy = y_start; wy <= y_end; wy++)
//            {
//                int row_offset = wy * WIDTH;
//                for (int wx = x_start; wx <= x_end; wx++)
//                {
//                    unsigned char val = src[row_offset + wx];
//                    sum += val;
//                    sq_sum += val * val;
//                    count++;
//                }
//            }
//
//            float mean = (float)sum / count;
//            float variance = ((float)sq_sum / count) - (mean * mean);
//            if (variance < 0) variance = 0.0f;
//            float stddev = sqrtf(variance);
//
//            // Вычисление порога Вульфа
//            float threshold = (1.0f - k_param) * mean + k_param * (float)min_val + k_param * (stddev / R) * (mean - (float)min_val);
//
//            int idx = y * WIDTH + x;
//            dst[idx] = (src[idx] >= threshold) ? 255 : 0;
//        }
//    }
//}



void wolf_binarization_mcu(const uint8_t *src, uint8_t *dst, int width, int height, int window_size, float k_param) {
    int radius = window_size / 2;
    int total_pixels = width * height;

    // Шаг 1: Поиск глобального минимума яркости (Min)
    uint8_t min_val = 255;
    for (int i = 0; i < total_pixels; i++) {
        if (src[i] < min_val) min_val = src[i];
    }

    // Шаг 2: Поиск максимального стандартного отклонения (R) по всему кадру
    float max_stddev = 0.0f;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int sum = 0, sq_sum = 0, count = 0;

            int y_start = (y - radius < 0) ? 0 : y - radius;
            int y_end = (y + radius >= height) ? height - 1 : y + radius;
            int x_start = (x - radius < 0) ? 0 : x - radius;
            int x_end = (x + radius >= width) ? width - 1 : x + radius;

            for (int wy = y_start; wy <= y_end; wy++) {
                int row_offset = wy * width;
                for (int wx = x_start; wx <= x_end; wx++) {
                    uint8_t val = src[row_offset + wx];
                    sum += val;
                    sq_sum += val * val;
                    count++;
                }
            }

            float mean = (float)sum / count;
            float variance = ((float)sq_sum / count) - (mean * mean);
            if (variance < 0) variance = 0.0f;
            float stddev = sqrtf(variance);

            if (stddev > max_stddev) {
                max_stddev = stddev;
            }
        }
    }

    float R = (max_stddev == 0.0f) ? 1.0f : max_stddev;

    // Шаг 3: Финальный расчет локального порога Вульфа и бинаризация
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int sum = 0, sq_sum = 0, count = 0;

            int y_start = (y - radius < 0) ? 0 : y - radius;
            int y_end = (y + radius >= height) ? height - 1 : y + radius;
            int x_start = (x - radius < 0) ? 0 : x - radius;
            int x_end = (x + radius >= width) ? width - 1 : x + radius;

            for (int wy = y_start; wy <= y_end; wy++) {
                int row_offset = wy * width;
                for (int wx = x_start; wx <= x_end; wx++) {
                    uint8_t val = src[row_offset + wx];
                    sum += val;
                    sq_sum += val * val;
                    count++;
                }
            }

            float mean = (float)sum / count;
            float variance = ((float)sq_sum / count) - (mean * mean);
            if (variance < 0) variance = 0.0f;
            float stddev = sqrtf(variance);

            // Оригинальная математическая формула Кристиана Вульфа
            float threshold = (1.0f - k_param) * mean + k_param * (float)min_val + k_param * (stddev / R) * (mean - (float)min_val);

            int idx = y * width + x;

            // Если пиксель светлее порога — делаем его белым (255), если темнее — черным (0)
            dst[idx] = (src[idx] >= threshold) ? 255 : 0;
        }
    }
}

