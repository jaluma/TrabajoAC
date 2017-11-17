// Single thread application with SIMD extensions

#include <Windows.h>
#include <intrin.h>
#include "immintrin.h"
#include <stdio.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <string>
using namespace std;
// Include required header files

#define NTIMES						1								// Number of repetitions to get suitable times
#define SIZE						1024/*(1024*1024)*/						// Number of elements in the array
#define GET_VARIABLE_NAME(Variable)	(#Variable)
#define NUMBER_FLOAT				sizeof(__m256) / sizeof(float)
#define PRINT_FUNCTIONS				true
#define PRINT_TIMER					true

// Timer
LARGE_INTEGER frequency;
LARGE_INTEGER tStart;
LARGE_INTEGER tEnd;
double dElapsedTimeS;

float* u;
float* w;
float* t;
float* v;

//atributos de return
float* r;			// vector resultante de op1
unsigned int k;		//numero de positivos op2
float* s;			// vector resultante de op3

float* createVector() {
	float* vector = (float *)_aligned_malloc(SIZE * sizeof(float), sizeof(__m256i));

	for (int i = 0; i < SIZE; i++) {
		float random = ((float)rand()) / (float)RAND_MAX;
		float diff = 1 - (-1);
		float r = random * diff;
		vector[i] = (-1) + r;	// rango de (0,2) - 1 ==> (-1, 1)
		/*vector[i] = 1;*/
	}
	return vector;
}

//eliminar vector
void removeVector(float* vector) {
	_aligned_free(vector);
}

void Dif2() {
	r = (float *)_aligned_malloc((SIZE-1) * sizeof(float), sizeof(__m256i));
	//Inicializamos una variable con el valor 2
	__m256 number2 = _mm256_set_ps(2, 2, 2, 2, 2, 2, 2, 2);
	//inicializamos una variable donde meter dif2
	__m256 value = *(__m256 *)_aligned_malloc((SIZE - 1) * sizeof(int), sizeof(__m256i));// 256 bits type, storing sixteen 32 bit integers
	__m256 valuei = *(__m256 *)_aligned_malloc((SIZE - 1) * sizeof(int), sizeof(__m256i));
	__m256 valuei_minus_1 = *(__m256 *)_aligned_malloc((SIZE - 1) * sizeof(int), sizeof(__m256i));

	for (int i = 0; i < SIZE - 1; i++) {
		__m256 valuei = *(__m256 *)&u[(i+1) * NUMBER_FLOAT];
		__m256 valuei_minus_1 = *(__m256 *)&u[i * NUMBER_FLOAT];
		value = _mm256_sub_ps(valuei, valuei_minus_1);
		value = _mm256_div_ps(value, number2);

		float *p = (float*)&value;							// Pointer p points to the first 32 integer in the packet
		for (int j = 0; j < NUMBER_FLOAT; j++) {
			r[i + j] = *p;
			p++;
			if (PRINT_FUNCTIONS)
				printf("La diferencia entre dos valores es %f\n", r[i+j]);
		}
	}
}

void countPositiveValues() {
	__m256 mask = _mm256_set_ps(0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000);
	//Calculate count
	for (int j = 0; j < SIZE / NUMBER_FLOAT; j++) {
		__m256 value = *(__m256*)&w[j * NUMBER_FLOAT];
		__m256 and = _mm256_and_ps(value, mask);	// mascara para mirar el bit mas significativo

		float *p = (float*)&and;
		for (int i = 0; i < NUMBER_FLOAT; i++) {
			if (*p != 0x80000000) {
				k++;
			}
			p++;
		}
		
	}
	if (PRINT_FUNCTIONS)
		printf("El contador de numeros positivos es %d\n", k);

}

//esta mal
void Sub() {
	//inicalizacion del vector V
	v = (float *)_aligned_malloc((SIZE - 1) * sizeof(float), sizeof(__m256i));
	__m256 kIntrisincs = _mm256_set_ps(k, k, k, k, k, k, k, k);
	for (int i = 0; i < (SIZE - 1)/ NUMBER_FLOAT; i++) {
		__m256 value = *(__m256*)&r[i * NUMBER_FLOAT];
		__m256 mult = _mm256_mul_ps(kIntrisincs, value);

		float* p = (float*)&mult;
		for (int j = 0; j < NUMBER_FLOAT; j++) {
			v[i + j] = *p;
		}

	}

	//codigo del programa
	s = (float *)_aligned_malloc((SIZE - 1) * sizeof(float), sizeof(__m256i));
	for (int i = 0; i < (SIZE - 1) / NUMBER_FLOAT; i++) {
		__m256 valueV = *(__m256*)&v[i*8];
		__m256 valueU = *(__m256*)&u[i*8];
		__m256 sub = _mm256_sub_ps(valueV, valueU);

		float* p = (float*)&sub;
		for (int j = 0; j < NUMBER_FLOAT; j++) {
			s[i + j] = *p;
		}

		if (PRINT_FUNCTIONS)
			printf("La resta es %f\n", s[i]);
	}
	removeVector(v);
	removeVector(s);
}

double timer(void(*function)(void)) {
	// Get clock frequency in Hz
	QueryPerformanceFrequency(&frequency);
	// Get initial clock count
	QueryPerformanceCounter(&tStart);
	// Code to be measured
	function();
	// Get final clock count
	QueryPerformanceCounter(&tEnd);
	// Compute the elapsed time in seconds
	dElapsedTimeS = (tEnd.QuadPart - tStart.QuadPart) / (double)frequency.QuadPart;
	// Print the elapsed time
	if (PRINT_TIMER)
		printf("Elapsed time in seconds: %f\n", dElapsedTimeS);
	// Return the elapsed time if it'll util
	return dElapsedTimeS;
}

void generateFile(double* times, double average, double std_deviation) {
	string nameFile = "times_" + to_string(average) + "_" + to_string(std_deviation) + ".csv";
	ofstream archivo(nameFile);

	for (int i = 0; i < NTIMES; i++) {
		archivo << times[i];
		archivo << "\n";
	}
	archivo.close();
}

int main() {
	double times[NTIMES];
	double average, variance, std_deviation, sum = 0, sum1 = 0;

	for (int i = 0; i < NTIMES; i++) {
		u = createVector();
		w = createVector();
		t = createVector();

		times[i] = /*timer(Dif2) +*/ timer(countPositiveValues) /*+ timer(Sub)*/;
		sum += times[i];

		removeVector(u);
		removeVector(w);
		removeVector(t);
	}

	average = sum / (double)NTIMES;

	for (int i = 0; i < NTIMES; i++) {
		sum1 = sum1 + pow((times[i] - average), 2);
	}

	variance = sum1 / (double)NTIMES;
	std_deviation = sqrt(variance);

	printf("La media de tiempos es: %f\r\n", average);
	printf("La desviacion tipica de tiempos es: %f\r\n", std_deviation);

	generateFile(times, average, std_deviation);
}