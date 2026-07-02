from PIL import Image
import os

#BIN_FILE_NAME = 'ov2640_frame.bin'
#OUTPUT_BMP_NAME = 'ov2640_frame.bmp'
WIDTH = 400
HEIGHT = 250

def convert_bin_to_bmp(bin_filename, bmp_filename):
    if not os.path.exists(bin_filename):
        print(f"Ошибка: Файл '{bin_filename}' не найден!")
        return

    # 1. Чтение бинарного файл напрямую в память
    with open(bin_filename, 'rb') as f:
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

    # 3. Изображение в градациях серого ('L' - Luminance) из байт
    img = Image.frombytes('L', (WIDTH, HEIGHT), raw_bytes)

    # 4. Сохранить как стандартный BMP
    img.save(bmp_filename)
    print(f"Успешно сохранено в {bmp_filename}")
    
    # Автоматически открыть готовую картинку на экране ПК
    img.show()

if __name__ == '__main__':
    convert_bin_to_bmp('ov2640_frame.bin', 'ov2640_frame.bmp')
    convert_bin_to_bmp('ov2640_frame_high_contrast.bin', 'ov2640_frame_high_contrast.bmp')
    convert_bin_to_bmp('ov2640_frame_binarized.bin', 'ov2640_frame_binarized.bmp')