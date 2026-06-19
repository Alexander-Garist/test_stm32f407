// библиотека для функций обработки изображения как набора значений яркости каждого отдельного пикселя
#ifndef __IMAGE_PROCESSING_H__
#define __IMAGE_PROCESSING_H__


// Алгоритм бинаризации Брэдли с использованием интегральной суммы
void bradley_threshold_integral(const unsigned char* src, unsigned char* res, int width, int height);

// Алгоритм бинаризации Брэдли без использования интегральной суммы
void bradley_threshold_mcu_fast(const unsigned char* src, unsigned char* res, int width, int height);


// Функция бинаризации Вульфа (для получения большей контрастности результата) без использования динамической памяти
//void wolf_binarization_mcu(const unsigned char *src, unsigned char *dst, int window_size, float k_param);
void wolf_binarization_mcu(const uint8_t *src, uint8_t *dst, int width, int height, int window_size, float k_param);

#endif /* __IMAGE_PROCESSING_H__ */