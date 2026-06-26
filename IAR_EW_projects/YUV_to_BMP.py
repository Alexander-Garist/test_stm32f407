from PIL import Image
import os

# ==================== НАСТРОЙКИ СНИМКА ====================
BIN_FILE_NAME = 'snapshot.bin'      # Имя вашего бинарного файла из МК
OUTPUT_BMP_NAME = 'result.bmp'       # Имя готовой картинки
WIDTH = 400
HEIGHT = 250

def convert_bin_to_bmp():
    if not os.path.exists(BIN_FILE_NAME):
        print(f"Ошибка: Файл '{BIN_FILE_NAME}' не найден!")
        return

    # 1. Читаем бинарный файл напрямую в память
    with open(BIN_FILE_NAME, 'rb') as f:
        raw_bytes = f.read()

    expected_size = WIDTH * HEIGHT
    print(f"Прочитано из файла: {len(raw_bytes)} байт.")

    # 2. Проверка размера (защита от неполных кадров)
    if len(raw_bytes) < expected_size:
        print("Внимание: Файл меньше кадра. Дописываем нули.")
        raw_bytes += b'\x00' * (expected_size - len(raw_bytes))
    elif len(raw_bytes) > expected_size:
        print("Внимание: Файл больше кадра. Обрезаем лишнее.")
        raw_bytes = raw_bytes[:expected_size]

    # 3. Создаем изображение в градациях серого ('L' - Luminance) из байт
    img = Image.frombytes('L', (WIDTH, HEIGHT), raw_bytes)

    # 4. Сохраняем как стандартный BMP (или .png / .jpg)
    img.save(OUTPUT_BMP_NAME)
    print(f"Успешно сохранено в {OUTPUT_BMP_NAME}")
    
    # Бонус: автоматически открыть готовую картинку на экране ПК
    img.show()

if __name__ == '__main__':
    convert_bin_to_bmp()