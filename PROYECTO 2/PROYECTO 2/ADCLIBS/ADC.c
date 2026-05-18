/*
 * ADC.c
 *
 * Created:
 * Author:
 * Description: Libreria general para el uso del ADC en ATmega328P
 */

/****************************************/
// Libraries
#include "ADC.h"

/****************************************/
// NON-Interrupt subroutines

void initADC(void)
{
	/*
		Referencia de voltaje: AVCC
		Resultado justificado a la derecha
		Canal inicial: ADC0
	*/
	ADMUX = (1 << REFS0);

	/*
		
	*/
	ADCSRA = (1 << ADEN)  |
	         (1 << ADPS2) |
	         (1 << ADPS1) |
	         (1 << ADPS0);

	/*
		Modo normal de conversion.
		No se usa auto trigger.
	*/
	ADCSRB = 0;
}

uint16_t readADC(uint8_t canal)
{
	/*
		El ATmega328P normalmente usa ADC0 a ADC5.
		Esta mascara evita seleccionar canales fuera de rango.
	*/
	canal = canal & 0x07;

	/*
		Se conserva la referencia AVCC y se cambia solamente el canal.
	*/
	ADMUX = (ADMUX & 0xF0) | canal;

	/*
		Iniciar conversion ADC.
	*/
	ADCSRA |= (1 << ADSC);

	
	while (ADCSRA & (1 << ADSC));

	/*
		Retornar resultado de 10 bits.
		Rango: 0 a 1023.
	*/
	return ADC;
}

uint8_t readADC_8bits(uint8_t canal)
{
	uint16_t valorADC;

	valorADC = readADC(canal);

	
	
	return (uint8_t)(valorADC >> 2);
}