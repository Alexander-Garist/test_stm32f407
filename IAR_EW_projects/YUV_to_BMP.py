import struct
import re
import os

def load_hex_file(file_path):
    """
    Автоматически определяет формат дампа (.hex) и чисто извлекает 
    из него упорядоченный бинарный поток байт.
    """
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()
        
    # Проверяем, является ли файл стандартным Intel HEX (записи начинаются с ':')
    is_intel_hex = any(line.strip().startswith(':') for line in lines)
    
    if is_intel_hex:
        print("[ℹ️] Обнаружен стандартный формат Intel HEX. Извлечение данных...")
        data_dict = {}
        base_address = 0
        
        for line in lines:
            line = line.strip()
            if not line.startswith(':'):
                continue
            try:
                length = int(line[1:3], 16)
                address = int(line[3:7], 16)
                record_type = int(line[7:9], 16)
                
                if record_type == 0:  # Запись данных
                    abs_address = base_address + address
                    data_hex = line[9:9 + length * 2]
                    for i in range(length):
                        byte_val = int(data_hex[i*2 : i*2 + 2], 16)
                        data_dict[abs_address + i] = byte_val
                elif record_type == 4:  # Расширенный линейный адрес
                    base_address = int(line[9:13], 16) << 16
            except ValueError:
                continue
                
        if not data_dict:
            raise ValueError("Не удалось распарсить Intel HEX. Проверьте структуру.")
            
        # Сшиваем непрерывный массив, строго сортируя байты по адресам памяти
        sorted_addresses = sorted(data_dict.keys())
        return bytes([data_dict[addr] for addr in sorted_addresses])
        
    else:
        print("[ℹ️] Обнаружен текстовый Hex-дамп (набор байт). Очистка от мусора...")
        # Если это простой лог/текст, убираем пробелы, скобки, "0x" и склеиваем
        full_text = "".join(lines)
        hex_chars = re.sub(r'[^0-9a-fA-F]', '', full_text)
        if not hex_chars:
            raise ValueError("В текстовом файле не найдено валидных Hex-символов.")
        return bytes.fromhex(hex_chars)

def yuv422_to_bmp(hex_input_path, bmp_output_path, width=160, height=120):
    # 1. Извлечение бинарного массива из текстового HEX
    try:
        yuv_data = load_hex_file(hex_input_path)
    except Exception as e:
        print(f"[❌] Критическая ошибка при чтении HEX: {e}")
        return

    expected_size = width * height * 2 # Для 160x120 это ровно 38400 байт
    print(f"[ℹ️] Успешно извлечено: {len(yuv_data)} байт из дампа.")
    
    if len(yuv_data) < expected_size:
        print(f"[⚠️] Внимание: байт меньше нормы ({len(yuv_data)} < {expected_size}). Дописываем нули...")
        yuv_data += b'\x00' * (expected_size - len(yuv_data))
    elif len(yuv_data) > expected_size:
        print(f"[ℹ️] Дамп больше кадра. Обрезаем лишнее до {expected_size} байт.")
        yuv_data = yuv_data[:expected_size]

    rgb_bytes = bytearray()
    
    # 2. Декодирование макропикселей YUV422 (Y0, U0, Y1, V0) -> 2 пикселя RGB
    for i in range(0, expected_size, 4):
        y0 = yuv_data[i]
        u  = yuv_data[i+1]
        y1 = yuv_data[i+2]
        v  = yuv_data[i+3]
        
        c = u - 128
        d = v - 128
        
        # Точка 1 (Y0)
        r0 = max(0, min(255, int(y0 + 1.402 * d)))
        g0 = max(0, min(255, int(y0 - 0.344136 * c - 0.714136 * d)))
        b0 = max(0, min(255, int(y0 + 1.772 * c)))
        
        # Точка 2 (Y1)
        r1 = max(0, min(255, int(y1 + 1.402 * d)))
        g1 = max(0, min(255, int(y1 - 0.344136 * c - 0.714136 * d)))
        b1 = max(0, min(255, int(y1 + 1.772 * c)))
        
        # Запись в структуру BMP (строгий формат BGR)
        rgb_bytes.extend([b0, g0, r0, b1, g1, r1])

    # 3. Сборка стандартного заголовка BMP (54 байта)
    file_size = 54 + len(rgb_bytes)
    header = struct.pack('<2sIHHIIIIHHIIIIII', 
                         b'BM', file_size, 0, 0, 54, 
                         40, width, height, 1, 24, 0, len(rgb_bytes), 2835, 2835, 0, 0)
    
    # 4. Сохранение файла
    with open(bmp_output_path, 'wb') as f:
        f.write(header)
        f.write(rgb_bytes)
        
    print(f"[✅] BMP картинка создана: {bmp_output_path}")

# ==================== НАСТРОЙКИ ПЕРЕД ЗАПУСКОМ ====================
HEX_FILE_NAME = 'memory.hex'   # Имя вашего файла с дампом из STM32
OUTPUT_BMP_NAME = 'decoded_cam.bmp' # Имя файла, который создаст скрипт

if __name__ == '__main__':
    if os.path.exists(HEX_FILE_NAME):
        yuv422_to_bmp(HEX_FILE_NAME, OUTPUT_BMP_NAME, width=160, height=120)
    else:
        print(f"[❌] Ошибка: Файл '{HEX_FILE_NAME}' не найден в текущей папке скрипта!")
