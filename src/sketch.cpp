#if defined(__PIC32MX__)
    #include <p32xxxx.h>    /* this gives all the CPU/hardware definitions */
    #include <plib.h>       /* this gives the i/o definitions */
#endif
#include <WProgram.h>
#include <IOShieldOled.h>

#define N_MUESTRAS	2048

int16_t muestras[N_MUESTRAS];

volatile bool muestras_listas = false;
volatile uint16_t contador_muestras = 0;

void config_analog(void);

void setup()
{
	IOShieldOled.begin();
	Serial.begin(115200);
	config_analog();
}

void loop()
{
	while(!muestras_listas);
	muestras_listas = false;
	for (int i = 0; i < N_MUESTRAS; ++i)
	{
		Serial.println(muestras[i], DEC);
	}
	Serial.println();

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
	AD1CON1bits.SSRC  = 7; // El contador interno termina el muestreo y comienza la conversion.
	AD1CON1bits.ASAM = 1; // El muestreo comienza cuando se active el bit SAMP

	// Seleccion el reloj del ADC
	// TPB = (1 / 80MHz) * 2 = 25ns
	// TAD = TPB * (ADCS + 1) * 2 = 25ns * 20  * 2 = 1000ns
	// TAD = 25ns * 160 * 2 = 8000ns
	AD1CON3bits.ADCS = 159;

	// Tiempo de Auto muestreo
	// SAMC * TAD = 30 * 1000ns = 30uS 33333KHz
	// SAMC * TAD = 31 * 8000ns = 248uS 4032.26KHz
	AD1CON3bits.SAMC = 31;

	IPC6bits.AD1IP = 6;		// ADC 1 Prioridad de interrupcion
	IPC6bits.AD1IS = 3;		// ADC 1 Subprioridad
	IFS1CLR = 2;
	IEC1bits.AD1IE = 1;		// Habilitamos la interrupcion del ADC
	AD1CON1bits.ON = 1;		// Habilitamos el ADC
}