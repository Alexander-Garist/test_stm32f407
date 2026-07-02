// библиотека дл€ функций обработки изображени€ как набора значений €ркости каждого отдельного пиксел€
#ifndef __IMAGE_PROCESSING_H__
#define __IMAGE_PROCESSING_H__

/** ”величить контрастность темного изображени€ */
void ImageProcessing_increase_image_contrast(uint8_t *buffer, uint32_t size);

/** Ѕинаризаци€ изображени€ */
void ImageProcessing_binarize_image(uint8_t *buffer, uint32_t size);
void ImageProcessing_binarize_adaptive_local(uint8_t *buffer, int width, int height);
#endif /* __IMAGE_PROCESSING_H__ */