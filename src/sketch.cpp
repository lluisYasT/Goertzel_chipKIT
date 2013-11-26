#include <plib.h>       
#include <WProgram.h>

#define GAIN_BITS		15
#define GAIN				(1<<GAIN_BITS)
#define N_MUESTRAS	800
#define COSENO			19261	// 0.5878 * 2^15 1.2kHz
#define SENO				26510 // 0.809 * 2^15
#define MITAD				16384 // 0.5 * 2^15
#define ATENUADOR		57		// Para no saturar
#define UMBRAL			2949	// 0.3^2 * 2^15

volatile bool muestras_listas = false;
volatile uint16_t contador_muestras = 0;
volatile int max = 0;
volatile int q0, q1 = 0, q2 = 0;

void setup()
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

	// Frecuencia de muestreo = 8kHz

	IPC6bits.AD1IP = 6;		// ADC 1 Prioridad de interrupcion
	IPC6bits.AD1IS = 3;		// ADC 1 Subprioridad
	IFS1CLR = 2;
	IEC1bits.AD1IE = 1;		// Habilitamos la interrupcion del ADC
	AD1CON1bits.ON = 1;		// Habilitamos el ADC
}

void loop()
{
	while(!muestras_listas);
	muestras_listas = false;

	// Parte no recursiva de Goertzel
	q0 = (COSENO * q1) >> GAIN_BITS;
	q0 -= (MITAD * q2) >> GAIN_BITS;
	q0 += (COSENO * q1) >> GAIN_BITS;
	q0 -= (MITAD * q2) >> GAIN_BITS;
	q2 = q1;
	q1 = q0;

	// Calculo la potencia
	int real = q1;
	real -= (COSENO * q1) >> GAIN_BITS;
	real = (real * real) >> GAIN_BITS;

	int imag = (SENO * q2) >> GAIN_BITS;
	imag = (imag * imag) >> GAIN_BITS;
	
	q1 = 0;
	q2 = 0;
	int potencia = real + imag;

	// Valor con el que comparamos la potencia 
	// para saber si hay tono
	max = (max * max) >> GAIN_BITS;
	max = (max * UMBRAL) >> GAIN_BITS;

	// Si hay tono encendemos el LED de la placa
	if(potencia > max)
	{
		digitalWrite(13,HIGH);
	} else {
		digitalWrite(13,LOW);
	}

	max = 0;

	AD1CON1bits.ASAM = 1; // Comienza auto-muestreo
}

// Interrupcion del ADC
extern "C"
{
	void __ISR(_ADC_VECTOR, ipl6) ADCInterruptHandler()
	{
		IFS1CLR = 2;

		int muestra = ADC1BUF0;
		muestra = muestra - 512;
		muestra = muestra << (GAIN_BITS - 9);
		muestra *= ATENUADOR;
		muestra >>= GAIN_BITS;
		
		if(muestra > max) {
			max = muestra;
		}


		q0 = (COSENO * q1) >> GAIN_BITS;
		q0 -= (MITAD * q2) >> GAIN_BITS;
		q0 += (COSENO * q1) >> GAIN_BITS;
		q0 -= (MITAD * q2) >> GAIN_BITS;
		q0 += muestra;
		q2 = q1;
		q1 = q0;

		contador_muestras++;

		if(contador_muestras >= N_MUESTRAS)
		{
			contador_muestras = 0;
			muestras_listas = true;
			AD1CON1bits.ASAM = 0; // Paramos el automuestreo
		}
	}
}
