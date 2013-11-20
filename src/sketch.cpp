#include <p32xxxx.h>    
#include <plib.h>       

#include <WProgram.h>
#include <IOShieldOled.h>

#define GAIN_BITS		15
#define GAIN				(1<<GAIN_BITS)
#define Fs					116000			
#define N_MUESTRAS	200
#define PI					3141592653589793 
#define UMBRAL			550									//Potencia minima deteccion tono

#define PIN_UP			4
#define PIN_DOWN		78

int16_t muestras[N_MUESTRAS];
int muestras_norm[N_MUESTRAS];

volatile bool muestras_listas = false;
volatile uint16_t contador_muestras = 0;
int m;
int omega, coseno, seno, coef;
char frec[12];

void config_analog(void);
void calculo_coeficientes(void);

void setup()
{
	IOShieldOled.begin();
	Serial.begin(115200);
	config_analog();
	
	m = 10;		// Valor por defecto
	calculo_coeficientes();
}

void loop()
{
	while(!muestras_listas);
	muestras_listas = false;
	int max = 0;
	for (int i = 0; i < N_MUESTRAS; ++i)
	{
		muestras_norm[i] = muestras[i] - 511.0;
		muestras_norm[i] = muestras_norm[i] << (GAIN_BITS - 10);

		if(abs(muestras_norm[i]) > max) max = muestras_norm[i];
		// Enviamos datos por puerto serie por si queremos analizarlo usando gnuplot, Octave o Matlab
		//Serial.print(i);
		//Serial.print('\t');
		//Serial.println(muestras[i]);
	}

	// normalizamos usando la muestra de valor maximo
	// para intentar independizarlo de la potencia recibida
	for (int i = 0; i < N_MUESTRAS; i++) {
		muestras_norm[i] /= max;
	}

	// Comeinza Goertzel
	int w, w_1 = 0, w_2 = 0;

	for (int i = 0; i < N_MUESTRAS; ++i)
	{
		w = muestras_norm[i] + coef * w_1 - w_2;
		w_2 = w_1;
		w_1 = w;
	}

	w = coef * w_1 - w_2;
	w_2 = w_1;
	w_1 = w;

	int real = (w_1 - w_2 * coseno);
	int imag = (w_2 * seno);
	int potencia = sqrt(real * real + imag * imag);
	potencia = potencia >> GAIN_BITS;
	// Fin Goertzel

	IOShieldOled.clear();
	IOShieldOled.setCursor(0,0);
	
	IOShieldOled.putString(frec);
	char pot[8];
	IOShieldOled.setCursor(0,1);
	sprintf(pot, "%.2f", potencia);
	IOShieldOled.putString(pot);
	if(max > 510 << GAIN_BITS)
	{
		IOShieldOled.setCursor(0,2);
		IOShieldOled.putString("Saturacion!!");
	}

	if(potencia > UMBRAL)
	{
		IOShieldOled.moveTo(0,0);
		IOShieldOled.drawRect(127,31);
		IOShieldOled.updateDisplay();
	}

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
	AD1CON2bits.SMPI = 0; // Cantidad de conversiones anates de generar la interrupcion
	AD1CON1bits.SSRC  = 7; // El contador interno termina el muestreo y comienza la conversion.
	AD1CON1bits.ASAM = 1; // El muestreo comienza cuando inmediatamente despues de la conversion.

	// Seleccion el reloj del ADC
	// TPB = (1 / 80MHz) = 12.5ns <- Periodo del reloj del bus de perifericos
	// TAD = TPB * (ADCS + 1) * 2
	AD1CON3bits.ADCS = 40;

	// Tiempo de Auto muestreo
	// Tmuestreo = SAMC * TAD
	AD1CON3bits.SAMC = 4;

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
void calculo_coeficientes()
{

	float aux;
	omega = (2 * PI * m) / N_MUESTRAS;
	aux = cos(omega);
	coseno = (int)(aux * GAIN);
	aux = sin(omega);
	seno = (int)(aux * GAIN);
	coef = 2 * coseno;

	int frecuencia = m * Fs / N_MUESTRAS;
	int precision = Fs / N_MUESTRAS / 2;
	sprintf(frec, "%d+-%d Hz",frecuencia, precision);
}
