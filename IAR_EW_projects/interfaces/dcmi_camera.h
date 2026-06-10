#ifndef DCMI_CAMERA_H
#define DCMI_CAMERA_H

#include "stm32f4xx.h"

// Разрешение QVGA (160*120), формат YUV422 (2 байта на пиксель)
#define CAM_WIDTH        160
#define CAM_HEIGHT       120
#define CAM_FRAME_BYTES  (CAM_WIDTH * CAM_HEIGHT * 2) // 38400 байт

// Прототипы функций
void DCMI_Init(void);
void DCMI_StartCapture(uint32_t *buffer);
uint8_t DCMI_IsFrameReady(void);
void DCMI_ClearFrameReady(void);

#endif // DCMI_CAMERA_H
