#ifndef __OV2640_H__
#define __OV2640_H__

#include <stdint.h>

#define CAM_WIDTH        800                        // Высота кадра, который выдает камера
#define CAM_HEIGHT       600                        // Ширина кадра, который выдает камера
#define CAM_FRAME_BYTES  (CAM_WIDTH * CAM_HEIGHT)   // Размер массива для яркости пикселей 1 кадра

#define VSYNC_PORT      GPIOB
#define VSYNC_PIN       (1 << 5)
#define VSYNC_IS_HIGH   (VSYNC_PORT->IDR & VSYNC_PIN)

#define HREF_PORT       GPIOC
#define HREF_PIN        (1 << 9)
#define HREF_IS_HIGH    (HREF_PORT->IDR & HREF_PIN)

#define DCLK_PORT       GPIOC
#define DCLK_PIN        (1 << 10)
#define DCLK_IS_HIGH    (DCLK_PORT->IDR & DCLK_PIN)

#define DATA_PORT       GPIOE

typedef struct
{
    uint8_t reg;
    uint8_t val;
}
ov2640_reg_t;

/** Инициализация камеры: запись регистров по I2C */
void ov2640_Init(uint8_t device_address);

/** Определить длительность кадра, длительность строки, количество строк и количество байт в строке */
void ov2640_count_pixels_in_frame();

/** Захват кадра + бинаризация на лету + упаковка в сжатый массив  БЕЗ СОХРАНЕНИЯ ИСХОДНОГО КАДРА*/
int ov2640_capture_and_process(uint8_t *packed_buffer, int width, int height);

#endif /* __OV2640_H__ */