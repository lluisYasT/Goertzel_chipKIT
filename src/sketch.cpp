
#include <p32xxxx.h>    
#include <plib.h>       

#include <WProgram.h>
#include <IOShieldOled.h>

#define Fs			20000
#define N_MUESTRAS	256
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

	omega = (2.0 * PI * m) / N_MUESTRAS;
	coseno = cos(omega);
	seno = sin(omega);
	coef = 2 * coseno;

	m = 2;

	frecuencia = m * (float)Fs / N_MUESTRAS;
	sprintf(frec, "%.1f Hz",frecuencia);
}

void loop()
{
	
	while(contador_muestras < N_MUESTRAS) {
		while(!IFS1 & 0x0002){};					// Ha acabado la conversion?
		muestras[contador_muestras++] = ADC1BUF0;	// Guardamos el primer valor del ADC
		IFS1CLR = 0x0002;							// Repetimos
	}
	for (int i = 0; i < N_MUESTRAS; ++i)
	{
		muestras_norm[i] = ((float)muestras[i] - 512.0) / 512.0;
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

	delay(50);
}

void config_analog()
{
	//Configuracion del ADC
	AD1PCFG = 0xFFFE;			// PORTB digital, RB0 analogico
	AD1CON1 = 0x0040;			// Bit SSRC = 010 Para usar TMR3

	AD1CHSbits.CH0SA = 1; // Canal 1. AN1

	AD1CSSL = 0;
	AD1CON3 = 0x0000;			// Tiempo de muestreo es TMR3, TAD = TPB (interno) * 2
	AD1CON2 = 0x0004;			// Interrupcion cuando tenga 2 conversiones

	// Configuramos TMR3 para que venza cada 50 us
	TMR3 	= 0x0000;
	PR3 	= 0x01F4;			500 decimal
	T3CON 	= 0x8010;

	AD1CON1SET = 0x8000;
	AD1CON1SET = 0x0004;
}