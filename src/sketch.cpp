#include <p32xxxx.h>    
#include <plib.h>       

#include <WProgram.h>
#include <IOShieldOled.h>

#include "sin_1_2.h"

#define GAIN_BITS		15
#define GAIN				(1<<GAIN_BITS)
#define Fs					8000			
#define N_MUESTRAS	800
#define PI					3.141592653589793 
#define COSENO			19261	// 0.5878 * 2^15 1.2kHz
#define SENO				26510 // 0.809 * 2^15
#define MITAD				16384 // 0.5 * 2^15
#define ATENUADOR		57		// Para no saturar
#define UMBRAL			2949	// 0.3^2 * 2^15

#define PIN_UP			4
#define PIN_DOWN		78

int16_t muestras[N_MUESTRAS];
int muestras_norm[N_MUESTRAS];

volatile bool muestras_listas = false;
volatile uint16_t contador_muestras = 0;
int m;
float omega;
//int coseno, seno, coef;
char frec[12];

void config_analog(void);
//void calculo_coeficientes(void);

int goertzel(void);

void setup()
{
	IOShieldOled.begin();
	Serial.begin(115200);
	config_analog();
	
	//m = 10;		// Valor por defecto
	//calculo_coeficientes();
}

void loop()
{
	while(!muestras_listas);
	muestras_listas = false;
	int max = 0;
	for (int i = 0; i < N_MUESTRAS; ++i)
	{
		muestras_norm[i] = muestras[i] - 512;
		muestras_norm[i] = muestras_norm[i] << (GAIN_BITS - 9);
		muestras_norm[i] *= ATENUADOR;
		muestras_norm[i] >>= GAIN_BITS;
		
		if(muestras_norm[i] > max) {
			max = muestras_norm[i];
		}

		//Serial.print(muestras_norm[i]);
		//Serial.print(',');
	}

	int potencia = goertzel();

	IOShieldOled.clear();
	IOShieldOled.setCursor(0,0);
	
	//IOShieldOled.putString(frec);
	char pot[8];
	IOShieldOled.setCursor(0,1);
	sprintf(pot, "%d", potencia);
	IOShieldOled.putString(pot);


	
	/*
	if(max > 1 << GAIN_BITS)
	{
		IOShieldOled.setCursor(0,2);
		IOShieldOled.putString("Saturacion!!");
	}
	*/

	max = (max * max) >> GAIN_BITS;
	max = (max * UMBRAL) >> GAIN_BITS;

	if(potencia > max)
	{
		IOShieldOled.moveTo(0,0);
		IOShieldOled.drawRect(127,31);
		IOShieldOled.updateDisplay();
	}

	/*
	if(digitalRead(PIN_DOWN) && m > 0)
	{
		m--;
		calculo_coeficientes();
		delay(200);
	}
	// Podemos aumentar la frecuencia hasta Fs/2 ya que a partir de ahi
	// se produce aliasing
	if(digitalRead(PIN_UP) && m < N_MUESTRAS/2 - 1)
	{
		m++;
		calculo_coeficientes();
		delay(200);
	}
	*/

	AD1CON1bits.ASAM = 1; // Comienza auto-muestreo
}

// Interrupcion del ADC
extern "C"
{
	void __ISR(_ADC_VECTOR, ipl6) ADCInterruptHandler()
	{
		IFS1CLR = 2;

		muestras[contador_muestras++] = ADC1BUF0;

		if(contador_muestras >= N_MUESTRAS)
		{
			contador_muestras = 0;
			muestras_listas = true;
			AD1CON1bits.ASAM = 0; // Paramos el automuestreo
		}
	}
}

void config_analog()
{
	//Configuracion del ADC
	AD1PCFG = ~1;	// PORTB digital, RB0 analogico
	AD1CHSbits.CH0SA = 1; // Canal 1. AN1
	AD1CON2bits.SMPI = 0; // Cantidad de conversiones antes de generar la interrupcion
	AD1CON1bits.SSRC  = 7; // El contador interno termina el muestreo y comienza la conversion.
	AD1CON1bits.ASAM = 1; // El muestreo comienza inmediatamente despues de la conversion.

	// Seleccion el reloj del ADC
	// TPB = (1 / 80MHz) = 12.5ns <- Periodo del reloj del bus de perifericos
	// TAD = TPB * (ADCS + 1) * 2
	AD1CON3bits.ADCS = 255;

	// Tiempo de Auto muestreo
	// Tmuestreo = SAMC * TAD
	AD1CON3bits.SAMC = 26;

	// Tiempo total = Tmuestreo + Tconversion = (TAD * SAMC) + (TAD * 12)
	// Frecuencia de muestreo = 1 / Tiempo Total

	IPC6bits.AD1IP = 6;		// ADC 1 Prioridad de interrupcion
	IPC6bits.AD1IS = 3;		// ADC 1 Subprioridad
	IFS1CLR = 2;
	IEC1bits.AD1IE = 1;		// Habilitamos la interrupcion del ADC
	AD1CON1bits.ON = 1;		// Habilitamos el ADC
}

// Calculo de los coeficientes necesarios para Goertzel.
// Intentamos calcularlos solamente en el comienzo del programa
// y cuando cambiemos el valor de m.
/*
void calculo_coeficientes()
{

	float aux;
	omega = (2 * PI * m) / N_MUESTRAS;
	aux = cos(omega);
	coseno = (int)(aux * GAIN);
	aux = sin(omega);
	seno = (int)(aux * GAIN);

	int frecuencia = m * Fs / N_MUESTRAS;
	int precision = Fs / N_MUESTRAS / 2;
	sprintf(frec, "%d+-%d Hz",frecuencia, precision);
}
*/

int goertzel()
{

	int q0, q1 = 0, q2 = 0;

	for (int i = 0; i < N_MUESTRAS; ++i)
	{
		q0 = (COSENO * q1) >> GAIN_BITS;
		q0 -= (MITAD * q2) >> GAIN_BITS;
		q0 += (COSENO * q1) >> GAIN_BITS;
		q0 -= (MITAD * q2) >> GAIN_BITS;
		q0 += muestras_norm[i];
		q2 = q1;
		q1 = q0;

	}


	q0 = (COSENO * q1) >> GAIN_BITS;
	q0 -= (MITAD * q2) >> GAIN_BITS;
	q0 += (COSENO * q1) >> GAIN_BITS;
	q0 -= (MITAD * q2) >> GAIN_BITS;
	q2 = q1;
	q1 = q0;

	int real = q1;
	real -= (COSENO * q1) >> GAIN_BITS;
	real = (real * real) >> GAIN_BITS;

	int imag = (SENO * q2) >> GAIN_BITS;
	imag = (imag * imag) >> GAIN_BITS;

	int potencia = real + imag;
	
	return potencia;

}
