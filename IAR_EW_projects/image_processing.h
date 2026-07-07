// библиотека для функций обработки изображения как набора значений яркости каждого отдельного пикселя
#ifndef __IMAGE_PROCESSING_H__
#define __IMAGE_PROCESSING_H__

/** Увеличить контрастность темного изображения */
void ImageProcessing_increase_image_contrast(uint8_t *buffer, uint32_t size);

/** Бинаризация изображения */
void ImageProcessing_binarize_image(uint8_t *buffer, uint32_t size);
void ImageProcessing_binarize_adaptive_local(uint8_t *buffer, int width, int height);

/** Сравнение 2 изображений методом свертки */
float ImageProcessing_Compare_by_Convolution(
                                             uint8_t* example_array,    // образец кадра
                                             uint8_t* array,            // текущий кадр
                                             float* deviation_X,        // массив отклонений пикселей образца от среднего значения
                                             float* deviation_Y,        // массив отклонений пикселей текущего кадра от среднего значения
                                             uint32_t size);            // размер кадра

#endif /* __IMAGE_PROCESSING_H__ */