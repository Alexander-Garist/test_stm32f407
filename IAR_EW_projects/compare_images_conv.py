import cv2
import numpy as np
from scipy.signal import fftconvolve

def compare_symbols_with_shift_tolerance(img_ideal_path, img_current_path, max_shift=20, threshold=0.80):
    # 1. Загрузка изображений в градациях серого
    img_ideal = cv2.imread(img_ideal_path, cv2.IMREAD_GRAYSCALE)
    img_current = cv2.imread(img_current_path, cv2.IMREAD_GRAYSCALE)

    if img_ideal is None or img_current is None:
        print("Ошибка: не удалось загрузить одно из изображений!")
        return

    # 2. ФИЛЬТР СИМВОЛОВ: Инвертируем картинки (Делаем инверсию бинаризации)
    # По спецификации: теперь ФОН становится черным (0), а СИМВОЛЫ — белыми (255)
    # Это заставит математику свертки реагировать ТОЛЬКО на черные маркерные линии
    bin_ideal = cv2.bitwise_not(img_ideal)
    bin_current = cv2.bitwise_not(img_current)

    # Переводим в формат с плавающей точкой 0.0 - 1.0 для корректной математики
    mask_ideal = (bin_ideal > 127).astype(np.float32)
    mask_current = (bin_current > 127).astype(np.float32)

    # 3. МАТЕМАТИЧЕСКАЯ СВЕРТКА (2D Кросс-корреляция через быстрый Фурье-анализ)
    # Поворачиваем текущую матрицу на 180 градусов для превращения корреляции в свертку
    mask_current_flipped = np.flipud(np.fliplr(mask_current))
    
    # Вычисляем карту совпадений для всех возможных пространственных сдвигов
    match_map = fftconvolve(mask_ideal, mask_current_shifted_matrix_flipped(mask_current_flipped), mode='same')

    # 4. Нормализация (Вычисляем теоретический максимум совпадения площадей)
    # Считаем, сколько всего "белых" пикселей символов есть на эталоне и на текущем кадре
    area_ideal = np.sum(mask_ideal)
    area_current = np.sum(mask_current)
    max_possible_overlap = min(area_ideal, area_current)

    if max_possible_overlap == 0:
        print("Ошибка: на кадрах вообще нет черных символов (сплошной белый фон)!")
        return

    # Находим самый высокий пик совпадения на всей карте сдвигов
    best_overlap_pixels = np.max(match_map)
    
    # Получаем итоговую степень соответствия от 0.0 до 1.0
    score = best_overlap_pixels / max_possible_overlap

    print(f"--- Результат сверточного анализа ---")
    print(f"Площадь символов эталона: {int(area_ideal)} пикс.")
    print(f"Площадь символов кадра: {int(area_current)} пикс.")
    print(f"Максимальное совпадение контуров при наложении: {score:.4f}")

    # 5. Проверка порога    
    if score >= threshold:
        print("Изображения совпадают! (Символы найдены и соответствуют эталону)")
    else:
        print("Изображения не совпадают! (Символы отличаются или повреждены)")

def mask_current_shifted_matrix_flipped(matrix):
    return matrix

if __name__ == '__main__':
    # THRESHOLD = 0.80 означает, что 80% черных линий символов должны совпасть при наложении
    compare_symbols_with_shift_tolerance('EXAMPLE.bmp', 'ov2640_frame_binarized.bmp', threshold=0.80)
    input()
