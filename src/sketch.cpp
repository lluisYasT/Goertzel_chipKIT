#include <p32xxxx.h>    
#include <plib.h>       

#include <WProgram.h>
#include <IOShieldOled.h>

#define Fs			116000			// Se supone que la calculada deberia ser 125000 (esta es 3.75 superior)
#define N_MUESTRAS	2000
#define PI			3.141592653589793

int16_t muestras[N_MUESTRAS];
float muestras_norm[N_MUESTRAS];

volatile bool muestras_listas = false;
volatile uint16_t contador_muestras = 0;
float m;
float omega, coseno, seno, coef, frecuencia;
char frec[12];

void config_analog(void);

void setup()
{
	IOShieldOled.begin();
	Serial.begin(115200);
	config_analog();

	m = 20;
	omega = (2.0 * PI * m) / N_MUESTRAS;
	coseno = cos(omega);
	seno = sin(omega);
	coef = 2 * coseno;

	frecuencia = m * (float)Fs / N_MUESTRAS;
	sprintf(frec, "%.1f Hz",frecuencia);
}

void loop()
{
	while(!muestras_listas);
	muestras_listas = false;
	for (int i = 0; i < N_MUESTRAS; ++i)
	{
		muestras_norm[i] = ((float)muestras[i] - 512.0) / 512.0;

		// Enviamos datos por puerto serie por si queremos analizarlo usando gnuplot, Octave o Matlab
		//Serial.print(i);
		//Serial.print('\t');
		//Serial.println(muestras[i]);
	}

	float w, w_1 = 0, w_2 = 0;

	for (int i = 0; i < N_MUESTRAS; ++i)
	{
		w = muestras_norm[i] + coef * w_1 - w_2;
		w_2 = w_1;
		w_1 = w;
	}

	w = coef * w_1 - w_2;
	w_2 = w_1;
	w_1 = w;

	float real = (w_1 - w_2 * coseno);
	float imag = (w_2 * seno);
	float potencia = sqrt(real * real + imag * imag);

	IOShieldOled.clear();
	IOShieldOled.setCursor(0,0);
	
	IOShieldOled.putString(frec);
	char pot[8];
	IOShieldOled.setCursor(0,1);
	sprintf(pot, "%.2f", potencia);
	IOShieldOled.putString(pot);

	//delay(50);

	AD1CON1bits.ASAM = 1; // Comienza auto-muestreo
}

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
	AD1CON2bits.SMPI = 0; // Cantidad de conversiones anates de generar la interrupcion
	AD1CON1bits.SSRC  = 7; // El contador interno termina el muestreo y comienza la conversion.
	AD1CON1bits.ASAM = 1; // El muestreo comienza cuando inmediatamente despues de la conversion.

	// Seleccion el reloj del ADC
	// TPB = (1 / 80MHz) = 12.5ns <- Periodo del reloj del bus de perifericos
	// TAD = TPB * (ADCS + 1) * 2 = 12.5ns * 20  * 2 = 500ns
	// TAD = 25ns * 160 * 2 = 8000ns
	AD1CON3bits.ADCS = 40;

	// Tiempo de Auto muestreo
	// SAMC * TAD = 30 * 1000ns = 30uS
	// SAMC * TAD = 31 * 8000ns = 248uS
	// SAMC * TAD = 25 * 1000ns = 25us
	AD1CON3bits.SAMC = 4;

	// Tiempo total = Tmuestreo + Tconversion = (TAD * SAMC) + (TAD * 12)
	// Frecuencia de muestreo = 1 / Tiempo Total

	IPC6bits.AD1IP = 6;		// ADC 1 Prioridad de interrupcion
	IPC6bits.AD1IS = 3;		// ADC 1 Subprioridad
	IFS1CLR = 2;
	IEC1bits.AD1IE = 1;		// Habilitamos la interrupcion del ADC
	AD1CON1bits.ON = 1;		// Habilitamos el ADC
}
