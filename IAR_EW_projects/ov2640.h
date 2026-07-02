#ifndef __OV2640_H__
#define __OV2640_H__

#include <stdint.h>

#define CAM_WIDTH        400                        // в 1 строке 800 байт яркости и цветности, нужна только яркость для ЧБ
#define CAM_HEIGHT       250                        // количество строк не изменится
#define CAM_FRAME_BYTES  (CAM_WIDTH * CAM_HEIGHT)   // размер массива для яркости пикселей 1 кадра

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


/** Получить ID камеры */
void ov2640_Read_ID(uint8_t device_address);

/** Сброс камеры перед работой */
void ov2640_Reset();

/** Инициализация камеры: запись регистров по I2C */
void ov2640_Init(uint8_t device_address);


/** Захват кадра камеры
* в кадре 600 строк по 800 байт в каждой строке
* нужно забрать байты яркости (только нечетные) из каждой строки
* окно по высоте: строки от 175 до 425, остальные игнорируются
*/
int ov2640_capture_snapshot(uint8_t *buffer, int width, int height);

/** Определить длительность кадра, длительность строки, количество строк и количество байт в строке */
void ov2640_count_pixels_in_frame();

#endif /* __OV2640_H__ */