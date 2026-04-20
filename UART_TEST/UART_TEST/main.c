/*
 * NombreProgra.c
 *
 * Created: 
 * Author: 
 * Description: 
 */
/****************************************/
// Encabezado (Libraries)
#include <avr/io.h>
#include <avr/interrupt.h>

/****************************************/
// Function prototypes
void initUART();
void writeChar(char caracter);
void writeString( char* string);

/****************************************/
// Main Function
int main(void)
{
	cli();
	initUART();
	sei();
	writeChar('H');
	wrtieString("seccion20");
	while (1)
	{
	}
}

/****************************************/
// NON-Interrupt subroutines
void initUART()
{
	//configurar pines rx (PD0) y tx (PD1)
	DDRD  &= ~(1 << DDD0);
	DDRD  |= (1 << DDD1);
	
	UCSR0A = 0 ;
	// Habilitando interrupciones, habilitando Rx y Tx del UART0
	UCSR0B = (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0);
	
	//modo asincrono / PARIDAD DESHABILITADA / 1 STOP BIT / 8 DATA BITS 
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ02) ;

	//Setear UBRR0=
	UBRR0 = 103; //por calculos
	
	
	
	
}

void writeChar(char caracter)
{
	while(!(UCSR0A & ( 1 << UDRE0)));
	
	UDR0 = caracter;
	
}

void writeString(char* string)
{
	// Recorre la cadena carácter por carácter hasta encontrar el terminador nulo '\0'
	for(uint8_t i=0); string[i] != '\0'; i++)
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
	if ( bufferRX == ' a')
	{
		PORTD ^= 
	}
}