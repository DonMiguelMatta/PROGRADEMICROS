/*
 * ADC.c
 *
 * Created:
 * Author:
 * Description: Libreria general para el uso del ADC en ATmega328P
 */

/****************************************/
// Librerias
#include "ADC.h"

/****************************************/
// Funciones

void initADC(void)
{
	// Configurar referencia AVCC y canal inicial ADC0
	ADMUX = (1 << REFS0);

	// Encender ADC con prescaler 128
	ADCSRA = (1 << ADEN)  |
	         (1 << ADPS2) |
	         (1 << ADPS1) |
	         (1 << ADPS0);

	// Usar conversion normal
	ADCSRB = 0;
}

uint16_t readADC(uint8_t canal)
{
	// Limitar canal al rango del ADC
	canal = canal & 0x07;

	// Cambiar canal sin tocar la referencia
	ADMUX = (ADMUX & 0xF0) | canal;

	// Iniciar lectura analogica
	ADCSRA |= (1 << ADSC);

	while (ADCSRA & (1 << ADSC));

	// Devolver lectura de 10 bits
	return ADC;
}

uint8_t readADC_8bits(uint8_t canal)
{
	uint16_t valorADC;

	valorADC = readADC(canal);

	return (uint8_t)(valorADC >> 2);
}
