import cv2
from skimage.metrics import structural_similarity as ssim

# 1. Загрузка изображений (как 8-битные серая шкала)
img_ideal = cv2.imread('EXAMPLE.bmp', cv2.IMREAD_GRAYSCALE)
img_current = cv2.imread('ov2640_frame_binarized.bmp', cv2.IMREAD_GRAYSCALE)

# 2. Свертка: размытие по Гауссу для устранения шума и микро-сдвигов
blur_ideal = cv2.GaussianBlur(img_ideal, (5, 5), 0)
blur_current = cv2.GaussianBlur(img_current, (5, 5), 0)

# 3. Вычисление индекса структурного сходства (SSIM)
score, diff = ssim(blur_ideal, blur_current, full=True)

print(f"Степень соответствия (SSIM): {score:.4f}")

# 4. Проверка порога
THRESHOLD = 0.85  # Настраивается экспериментально
if score >= THRESHOLD:
    print("Изображения совпадают!")
else:
    print("Изображения не совпадают!")

input()