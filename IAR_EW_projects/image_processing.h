// библиотека для функций обработки изображения как набора значений яркости каждого отдельного пикселя
#ifndef __IMAGE_PROCESSING_H__
#define __IMAGE_PROCESSING_H__

/** Увеличить контрастность темного изображения */
void ImageProcessing_increase_image_contrast(uint8_t *buffer, uint32_t size);

/** Бинаризация изображения */
void ImageProcessing_binarize_adaptive_local(uint8_t *buffer, int width, int height);

/** Функция сравнения упакованного текущего кадра с эталоном во Flash
*        Возвращает процент совпадения черных сегментов (от 0.0 до 1.0) */
float ImageProcessing_compare_packed_with_tolerance(uint8_t *current_packed, uint32_t example_address, uint32_t width, uint32_t height);

/**********************************************************************************************************************/
// здесь добавились функции для обработки изображения через БПФ

#include <math.h>
#include <complex.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// БПФ для 32 точек
void fft32(const double complex input[32], double complex output[32]);

// БПФ для 64 точек
void fft64(const double complex input[64], double complex output[64]);

#endif /* __IMAGE_PROCESSING_H__ */