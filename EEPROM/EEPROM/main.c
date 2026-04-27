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
#include "UARTLIB/UART.h"
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
// NON-Interrupt subroutines
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