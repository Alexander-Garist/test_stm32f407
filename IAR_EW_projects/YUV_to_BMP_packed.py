from PIL import Image
import os

# ==================== НАСТРОЙКИ СНИМКА ====================
WIDTH = 800
HEIGHT = 600
EXPECTED_SIZE = (WIDTH * HEIGHT) // 8 # 60 000 байт

def unpack_static_file(bin_filename, bmp_filename):
    if not os.path.exists(bin_filename):
        print(f"Ошибка: Файл '{bin_filename}' не найден в папке скрипта!")
        return

    with open(bin_filename, 'rb') as f:
        packed_bytes = f.read()

    print(f"Прочитано из файла: {len(packed_bytes)} байт.")

    if len(packed_bytes) != EXPECTED_SIZE:
        print(f"Ошибка: Вес файла не равен 10 000 байт (реальный размер: {len(packed_bytes)}).")
        return

    # РАСПАКОВКА БИТ С СОХРАНЕНИЕМ ПОРЯДКА (MSB-first)
    unpacked_pixels = bytearray()
    for byte in packed_bytes:
        for i in range(7, -1, -1):
            bit = (byte >> i) & 1
            if bit == 1:
                unpacked_pixels.append(255) # Белый
            else:
                unpacked_pixels.append(0)   # Черный
                
    # Создание графического файла
    img = Image.frombytes('L', (WIDTH, HEIGHT), bytes(unpacked_pixels))
    img.save(bmp_filename)
    img.show()
    print(f"Успешно распаковано в '{bmp_filename}'")

if __name__ == '__main__':
    unpack_static_file('ov2640_frame_binarized_packed.bin', 'result_frame.bmp')
    #unpack_static_file('ov2640_frame_example.bin', 'result_example.bmp')
