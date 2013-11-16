Goertzel ChipKIT
================
Intento de implementación del algoritmo de Goertzel en la plataforma chipKIT.

Detecta tonos en el puerto AN1 al que se le conecta el módulo de micrófono de Sparkfun. Debería poder hacerlo con cualquier señal con media de 1.65V y Vpp < 1.65V (para no saturar el ADC).
- La frecuencia de muestreo F_s es 116KHz por lo que la máxima frecuencia detectable (por el teorema de Nyquist) será 58KHz.