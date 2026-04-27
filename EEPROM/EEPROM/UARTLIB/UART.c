/*
 * CFile1.c
 *
 * Created: 22/04/2026 04:13:15 p. m.
 *  Author: migue
 */ 
/****************************************/
// NON-Interrupt subroutines

#include "UARTLIB/UART.h"

#define F_CPU 16000000
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

void initUART()
{
	// CONFIG. PINES RX (PD) Y TX (PD1)
	DDRD	&= ~(1 << DDD0);
	DDRD	|= (1 << DDD1);
	
	UCSR0A	= 0;
	// HABILITANDO INTERRUPCIONES, HABILITNADO RX Y TX DE UART0
	UCSR0B	= (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0);
	
	// ASYNC, PARIDAD DESHABILITADA, 1 STOP BIT, 8 DATA BITS
	UCSR0C	= (1 << UCSZ01) | (1 << UCSZ00);
	
	// SETEAR UBRR0
	UBRR0	= 103;
}

void writeChar(char caracter)
{
	while(!(UCSR0A & (1 << UDRE0)));
	UDR0 = caracter;
}
void writeString(char* string)
{
	for (uint8_t i=0; string[i] != '\0'; i++)
	{
		writeChar(string[i]);
		
	}
}
/****************************************/
// Interrupt routines
