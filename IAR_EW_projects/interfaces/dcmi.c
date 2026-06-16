#include "dcmi.h"

/**
  * @brief  Инициализация периферии DCMI и DMA2 на уровне регистров (CMSIS)
  */
void DCMI_Init(void) {
    // 1. Включаем тактирование (DCMI на шине AHB2, DMA2 на шине AHB1)
    RCC->AHB2ENR |= RCC_AHB2ENR_DCMIEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

    // 2. Аппаратная полярность сигналов для автономных модулей DIYMORE
    // VSPOL = 1, HSPOL = 1, PCKPOL = 1 (Данные строго синхронизированы по фронтам)
    DCMI->CR = 0;
    //DCMI->CR |= DCMI_CR_PCKPOL;
    //DCMI->CR |= DCMI_CR_VSPOL;
    //DCMI->CR |= DCMI_CR_HSPOL;

    // Формат: 8-битный захват (для RGB565 будет 2 захвата на пиксель)
    DCMI->CR &= ~DCMI_CR_EDM_0;
    DCMI->CR &= ~DCMI_CR_EDM_1;

 //   DCMI->CR |= DCMI_CR_ESS;

    // Отключаем любые фоновые прерывания DCMI, чтобы ядро не уходило в бесконечный цикл
    DCMI->IER = 0;

    // 3. Конфигурация DMA2 Stream 1 Channel 1
    DMA2_Stream1->CR &= ~DMA_SxCR_EN;
    while(DMA2_Stream1->CR & DMA_SxCR_EN);

    DMA2_Stream1->CR = 0;
    DMA2_Stream1->CR |= (1 << DMA_SxCR_CHSEL_Pos);
    DMA2_Stream1->CR |= DMA_SxCR_MINC;                 // Инкремент адреса памяти включен
    DMA2_Stream1->CR |= (0x00 << DMA_SxCR_DIR_Pos);    // Направление: из периферии в память

    // Максимальный приоритет шины для DMA (Very High = 3), чтобы исключить Overrun
    DMA2_Stream1->CR |= (3 << DMA_SxCR_PL_Pos);
    DMA2_Stream1->CR |= (2 << DMA_SxCR_MSIZE_Pos) | (2 << DMA_SxCR_PSIZE_Pos); // Перенос 32-битными словами

    // Аппаратный FIFO буфер обязателен для сглаживания параллельного потока DCMI
    DMA2_Stream1->FCR = DMA_SxFCR_DMDIS | (3 << DMA_SxFCR_FTH_Pos);
    DMA2_Stream1->PAR = (uint32_t)&(DCMI->DR);

    // Глобально активируем работу DCMI один раз при старте системы
    DCMI->CR |= DCMI_CR_ENABLE;
}

/**
  * @brief  Запуск захвата одного кадра в указанный буфер
  */
void DCMI_StartCapture(uint32_t *buffer) {
    // Гасим триггер захвата для сброса конвейера
    DCMI->CR &= ~DCMI_CR_CAPTURE;

    // Выключаем поток DMA и дожидаемся полной остановки железа
    DMA2_Stream1->CR &= ~DMA_SxCR_EN;
    while(DMA2_Stream1->CR & DMA_SxCR_EN);

    // Очищаем массив нулями перед каждым снимком, чтобы гарантировать проверку перезаписи
    for (int i = 0; i < (CAM_FRAME_BYTES / 4); i++) {
        buffer[i] = 0;
    }

    // Задаем параметры нового кадра (38400 байт / 4 = 9600 слов)
    DMA2_Stream1->M0AR = (uint32_t)buffer;
    DMA2_Stream1->NDTR = CAM_FRAME_BYTES / 4;

    // Сброс флагов Stream 1 в LIFCR и флагов DCMI через прямое присваивание '='
    DMA2->LIFCR = 0x00000F40;
    DCMI->ICR = 0x0000001F;

    // Запускаем цепочку: сначала включаем приемник DMA, затем взводим затвор DCMI
    DMA2_Stream1->CR |= DMA_SxCR_EN;
    DCMI->CR |= DCMI_CR_CM;            // Режим Snapshot (Одиночный снимок)
    DCMI->CR |= DCMI_CR_CAPTURE;       // Модуль начнет качать пиксели строго по первому ровному VSYNC кадра
}

/**
  * @brief  Прямой опрос аппаратного статуса переноса кадра (Для while в main.c)
  */
uint8_t DCMI_IsFrameReady(void) {
    // Читаем все флаги статуса
    uint32_t dmc_flags = DCMI->RISR;
    uint32_t dma_flags = DMA2->LISR;

    // Проверка DMA ошибок
    if (dma_flags & DMA_LISR_TEIF1) {
        return 4;  // DMA Transfer Error
    }
    if (dma_flags & DMA_LISR_FEIF1) {
        return 5;  // DMA FIFO Error
    }
    if (dma_flags & DMA_LISR_DMEIF1) {
        return 6;  // DMA Direct Mode Error
    }

    // Проверка DCMI ошибок
    if (dmc_flags & DCMI_RISR_OVR_RIS) {
        return 2;  // Overrun
    }
    if (dmc_flags & DCMI_RISR_ERR_RIS) {
        return 3;  // Frame Error
    }

    // Проверка готовности DMA
    if (dma_flags & DMA_LISR_TCIF1) {
        return 1;  // Успех!
    }

    // Возвращаем биты статуса для отладки
    return (dmc_flags & 0xFF) | ((dma_flags >> 8) & 0xFF00);
}


void DCMI_ClearFrameStatus(void) {
    // Принудительно останавливаем захват текущего кадра
    DCMI->CR &= ~DCMI_CR_CAPTURE;
    DMA2_Stream1->CR &= ~DMA_SxCR_EN;

    // Сбрасываем аппаратные флаги для подготовки к новому циклу
    DMA2->LIFCR = 0x00000F40;
    DCMI->ICR = 0x0000001F;
}