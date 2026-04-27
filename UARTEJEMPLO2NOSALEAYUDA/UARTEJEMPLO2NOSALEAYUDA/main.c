/*
* UART_TEST.c
*
* Created: 4/15/2026 5:19:39 PM
* Author : Donis
*/

/****************************************/
// Encabezado (Libraries)
#define F_CPU 16000000
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "UARTLIBR/UARTLIBR.h"
/****************************************/
// Function prototypes
void initUART();
void writeChar(char caracter);
void writeString(char* string);

/****************************************/
// Main Function
int main(void)
{
	cli();
	DDRD	|= (1 << DDD6);  | (1<<DDD5);
	PORTD	&= ~((1<<PORTD6) | (1 << PORTD5));
	DDRD &= ~(1<<DDD2);
	PORTD |= (1<<PORTD2);
	
	PCICR |= (1<,PCIE2); //HABILITO INTS PARA EL PUERTO D
	PCMSK2 |= (1<<PCINT18); //HABILITO INT PARA EL PD2 (HABILITAR SOLO DONDE LOS PINS SE VAN A UTILIZAR)
	
	
	initUART();
	sei();
	writeChar('H');
	writeChar('o');
	writeChar('l');
	writeChar('a');
	writeString("Seccion 20");
	while (1)
	{
	}
	

}
/****************************************/
/****************************************/
// Interrupt routines
ISR(USART_RX_vect)
{
	char bufferRX = UDR0;
	writeChar(bufferRX);
	if (bufferRX == 'a')
	{
		PORTD ^= (1 << PORTD5);
	}
}

ISR(PCINT2_vect)
{
	uint8_t estado PD2 = PIND & (1<<PIND2);
	if (estadopD2 != (1<<PIND2))
	{
		PORTD ^= (1<<PORTD5);
	}
}