def convert_raw_to_bmp(raw_filename, bmp_filename, width=160, height=120):
    with open(raw_filename, "rb") as f:
        raw_bytes = f.read()
    
    if len(raw_bytes) < width * height:
        print("Ошибка: Размер файла меньше 19200 байт!")
        return

    # Генерация палитры Grayscale (256 цветов * 4 байта)
    palette = bytearray()
    for i in range(256):
        palette.extend([i, i, i, 0])
        
    pixel_data_offset = 14 + 40 + len(palette)
    file_size = pixel_data_offset + (width * height)
    
    bmp_data = bytearray()
    bmp_data.extend(b'BM') 
    bmp_data.extend(file_size.to_bytes(4, 'little'))
    bmp_data.extend((0).to_bytes(2, 'little')) 
    bmp_data.extend((0).to_bytes(2, 'little')) 
    bmp_data.extend(pixel_data_offset.to_bytes(4, 'little'))
    
    bmp_data.extend((40).to_bytes(4, 'little'))
    bmp_data.extend(width.to_bytes(4, 'little'))
    bmp_data.extend(height.to_bytes(4, 'little')) 
    bmp_data.extend((1).to_bytes(2, 'little')) 
    bmp_data.extend((8).to_bytes(2, 'little')) # 8-битный Grayscale
    bmp_data.extend((0).to_bytes(4, 'little')) 
    bmp_data.extend((width * height).to_bytes(4, 'little'))
    bmp_data.extend((2835).to_bytes(4, 'little')) 
    bmp_data.extend((2835).to_bytes(4, 'little')) 
    bmp_data.extend((256).to_bytes(4, 'little')) 
    bmp_data.extend((256).to_bytes(4, 'little')) 
    
    bmp_data.extend(palette)
    
    # BMP хранит строки снизу вверх — переворачиваем кадр
    for y in range(height - 1, -1, -1):
        row = raw_bytes[y * width : (y + 1) * width]
        bmp_data.extend(row)
        
    with open(bmp_filename, "wb") as f:
        f.write(bmp_data)
    print(f"Успешно сконвертировано в {bmp_filename}")

convert_raw_to_bmp("memory_binary.raw", "photo_binary.bmp")