/*
* UART_TEST.c
*
* Created: 4/15/2026 5:19:39 PM
* Author : MiguelD
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


/****************************************/
// Main Function
int main(void)
{
	cli();
	DDRD	|= (1 << DDD5);
	PORTD	&= ~(1 << PORTD5);
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
