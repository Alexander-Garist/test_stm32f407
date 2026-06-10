#include "dcmi_camera.h"

static volatile uint8_t g_frame_ready = 0;

/**
  * @brief  Инициализация периферии DCMI и DMA2
  */
void DCMI_Init(void)
{
    // 1. Включаем тактирование модулей DCMI и DMA2
    RCC->AHB1ENR |= RCC_AHB2ENR_DCMIEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

    // 2. Сброс и базовая конфигурация DCMI
    DCMI->CR = 0; // Очистка регистра управления

    // Настройки интерфейса под Arducam OV2640:
    // - Аппаратная синхронизация (SynchroMode = Hardware, бит ESS = 0)
    // - Захват каждого кадра (Capture Rate = All frames, биты PCKFC = 00)
    // - Шина данных 8 бит (Extended Data Mode = 8-bit, биты EDM = 00)
    // - Полярность: VSYNC активен в LOW, HREF активен в HIGH, PCLK считывается по ФРОНТУ
    DCMI->CR |= (0 << DCMI_CR_VSPOL_Pos) |  // VSYNC active LOW
                (0 << DCMI_CR_HSPOL_Pos) |  // HREF active HIGH
                (0 << DCMI_CR_PCKPOL_Pos);  // PCLK rising edge

    // 3. Конфигурация DMA2 Stream 1 Channel 1 (жестко привязан к DCMI)
    DMA2_Stream1->CR &= ~DMA_SxCR_EN; // Убедимся, что поток выключен перед настройкой
    while(DMA2_Stream1->CR & DMA_SxCR_EN); // Ждем подтверждения выключения

    DMA2_Stream1->CR = 0;

    // Выбираем канал 1 (Channel 1)
    DMA2_Stream1->CR |= (1 << DMA_SxCR_CHSEL_Pos);

    // Направление: из периферии в память (Peripheral-to-Memory)
    DMA2_Stream1->CR |= (0x00 << DMA_SxCR_DIR_Pos);

    // Настройка размера данных: DCMI отдает данные 32-битными словами.
    // Периферия: 32 бита (Word), Память: 32 бита (Word)
    DMA2_Stream1->CR |= (2 << DMA_SxCR_PSIZE_Pos) | (2 << DMA_SxCR_MSIZE_Pos);

    // Инкремент адреса памяти включен, инкремент периферии выключен
    DMA2_Stream1->CR |= DMA_SxCR_MINC;

    // Режим: Одиночная транзакция (Snapshot). Очищаем биты 13 и 14.
DMA2_Stream1->CR &= ~DMA_SxCR_CIRC;

    // Приоритет потока DMA: Очень высокий (Very High), чтобы не терять пиксели
    DMA2_Stream1->CR |= (3 << DMA_SxCR_PL_Pos);

    // Включаем прерывание по завершению передачи кадра (Transfer Complete Interrupt)
    DMA2_Stream1->CR |= DMA_SxCR_TCIE;

    // Источник данных для DMA — регистр данных DCMI
    DMA2_Stream1->PAR = (uint32_t)&(DCMI->DR);

    // 4. Настройка NVIC для обработки прерывания окончания кадра от DMA
    NVIC_SetPriority(DMA2_Stream1_IRQn, 5);
    NVIC_EnableIRQ(DMA2_Stream1_IRQn);
}

/**
  * @brief  Запуск захвата одного кадра в указанный буфер
  * @param  buffer: Указатель на массив uint32_t размером не менее CAM_FRAME_BYTES/4
  */
void DCMI_StartCapture(uint32_t *buffer)
{
    g_frame_ready = 0;

    // 1. Настраиваем адрес назначения и объем данных в DMA
    DMA2_Stream1->M0AR = (uint32_t)buffer;
    // Количество транзакций DMA (в 32-битных словах, поэтому делим байты кадра на 4)
    DMA2_Stream1->NDTR = CAM_FRAME_BYTES / 4;

    // 2. Очищаем возможные старые флаги прерываний DMA2 для Stream 1
    DMA2->LIFCR |= DMA_LIFCR_CTCIF1 | DMA_LIFCR_CHTIF1 | DMA_LIFCR_CTEIF1 | DMA_LIFCR_CDMEIF1 | DMA_LIFCR_CFEIF1;

    // 3. Включаем DMA2 Stream 1
    DMA2_Stream1->CR |= DMA_SxCR_EN;

    // 4. Включаем модуль DCMI и активируем захват (Snapshot Mode + Capture)
    DCMI->CR |= DCMI_CR_ENABLE;
    DCMI->CR |= DCMI_CR_CM;      // 1 = Snapshot mode (один кадр)
    DCMI->CR |= DCMI_CR_CAPTURE; // Старт захвата
}

uint8_t DCMI_IsFrameReady(void)
{
    return g_frame_ready;
}

void DCMI_ClearFrameReady(void)
{
    g_frame_ready = 0;
}

/**
  * @brief  Обработчик прерывания DMA2 Stream 1 (вызывается автоматически при окончании кадра)
  */
void DMA2_Stream1_IRQHandler(void)
{
    // Проверяем флаг завершения передачи (Transfer Complete) для Stream 1
    if (DMA2->LISR & DMA_LISR_TCIF1)
    {
        DMA2->LIFCR |= DMA_LIFCR_CTCIF1; // Сбрасываем флаг в регистре

        // Выключаем захват DCMI, так как кадр уже полностью в ОЗУ
        DCMI->CR &= ~DCMI_CR_CAPTURE;

        g_frame_ready = 1; // Устанавливаем программный флаг готовности кадра
    }
}
