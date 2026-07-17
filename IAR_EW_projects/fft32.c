*******************************************************************************/
#include <stdio.h>
#include <math.h>
#include <complex.h>
 
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
 
// Правильная и полная таблица поворотных коэффициентов W_32^r для r от 0 до 15
const double complex W32[16] = {
    1.000000 + 0.000000 * I,  // W^0
    0.980785 - 0.195090 * I,  // W^1
    0.923880 - 0.382683 * I,  // W^2
    0.831470 - 0.555570 * I,  // W^3
    0.707107 - 0.707107 * I,  // W^4
    0.555570 - 0.831470 * I,  // W^5
    0.382683 - 0.923880 * I,  // W^6
    0.195090 - 0.980785 * I,  // W^7
    0.000000 - 1.000000 * I,  // W^8
   -0.195090 - 0.980785 * I,  // W^9
   -0.382683 - 0.923880 * I,  // W^10
   -0.555570 - 0.831470 * I,  // W^11
   -0.707107 - 0.707107 * I,  // W^12
   -0.831470 - 0.555570 * I,  // W^13
   -0.923880 - 0.382683 * I,  // W^14
   -0.980785 - 0.195090 * I   // W^15
};
 
// Функция инверсии 5 бит для N=32
unsigned int bit_reverse_5bit(unsigned int x) 
{
    unsigned int rev = 0;
    for (int i = 0; i < 5; i++) 
	{
        rev = (rev << 1) | (x & 1);
        x >>= 1;
    }
    return rev;
}
 
// Корректный алгоритм БПФ
void fft32(const double complex input[32], double complex output[32]) 
{
    // 1. Сначала делаем правильную перестановку во временный массив
    double complex temp[32];
    for (int i = 0; i < 32; ++i) 
	{
        temp[bit_reverse_5bit(i)] = input[i];
    }
 
    // 2. Поэтапный расчет бабочек
    for (int stage = 1; stage <= 5; ++stage) 
	{
        int len = 1 << stage;          // Длина текущего подблока (2, 4, 8, 16, 32)
        int half = len >> 1;           // Шаг бабочки (1, 2, 4, 8, 16)
        int twiddle_step = 32 / len;   // Шаг по таблице коэффициентов
 
        // Пробегаем по всему массиву с шагом размера блока
        for (int i = 0; i < 32; i += len) 
		{
            for (int j = 0; j < half; ++j) 
			{
                int top_idx = i + j;
                int bot_idx = i + j + half;
 
                // Вычисление индекса коэффициента r
                int r = j * twiddle_step;
                double complex W = W32[r];
 
                // Важно: берем значения из temp, но записываем результаты в ОДИНАКОВЫЙ буфер аккуратно
                double complex A = temp[top_idx];
                double complex B_turned = temp[bot_idx] * W;
 
                output[top_idx] = A + B_turned;
                output[bot_idx] = A - B_turned;
            }
        }
 
        // Копируем результаты текущего этапа назад в temp для следующего этапа
        for (int i = 0; i < 32; ++i) 
		{
            temp[i] = output[i];
        }
    }
}
 
int main() 
{
    double complex signal[32];
    double complex spectrum[32];
 
    // Генерируем тестовый сигнал: гармоника на частоте k=3
    // Формула: x(n) = cos(2*pi*3*n/32)
    for (int n = 0; n < 32; ++n) 
	{
        double angle = 2.0 * M_PI * 2.0 * n / 32.0;
        signal[n] = 1+sin(angle) + 0.0 * I; 
    }
 
    fft32(signal, spectrum);
 
    printf("--- Точный спектр 32-точечного БПФ ---\n");
    for (int k = 0; k < 32; ++k) 
	{
        double re = creal(spectrum[k]);
        double im = cimag(spectrum[k]);
 
        // Убираем шумы окружения float чисел (все что меньше 1e-5 приравниваем к 0)
        if (fabs(re) < 1e-5) re = 0.0;
        if (fabs(im) < 1e-5) im = 0.0;
 
        printf("X[%2d] = %8.4f %s %8.4fj\n", k, re, (im >= 0 ? "+" : "-"), fabs(im));
    }
 
    return 0;
}