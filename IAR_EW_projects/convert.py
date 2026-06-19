with open("memory_binary.hex", "r") as f:
    data = bytearray()
    for line in f:
        line = line.strip()
        if line.startswith(":") and line[7:9] == "00": # Записи типа 00 (Data)
            count = int(line[1:3], 16)
            payload = line[9:9+count*2]
            data.extend(bytes.fromhex(payload))

with open("memory_binary.raw", "wb") as f:
    f.write(data)