/*
 * UART.c
 *
 * Created:
 * Author:
 * Description: Libreria general para comunicacion serial UART en ATmega328P
 */

/****************************************/
// Libraries
#include "UART.h"

/****************************************/
// NON-Interrupt subroutines

void initUART(void)
{
	initUART_Baud(UART_BAUD_RATE);
}

void initUART_Baud(uint32_t baudrate)
{
	uint16_t ubrr;

	/*
		Calculo del valor para el registro UBRR.

		Para F_CPU = 16 MHz y baudrate = 9600:
		UBRR = 103 aproximadamente.
	*/
	ubrr = (uint16_t)((F_CPU / (16UL * baudrate)) - 1);

	/*
		Configurar baudrate.
	*/
	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)(ubrr);

	/*
		Habilitar transmision y recepcion UART.

		RXEN0: habilita recepcion
		TXEN0: habilita transmision
	*/
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);

	/*
		Formato de trama:

		8 bits de datos
		1 bit de stop
		Sin paridad
	*/
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void writeChar(char caracter)
{
	/*
		Esperar a que el buffer de transmision este libre.
		No usa interrupciones ni delays.
	*/
	while (!(UCSR0A & (1 << UDRE0)));

	/*
		Enviar caracter.
	*/
	UDR0 = caracter;
}

void writeString(char* string)
{
	uint8_t i = 0;

	/*
		Enviar cadena hasta encontrar el caracter nulo '\0'.
	*/
	while (string[i] != '\0')
	{
		writeChar(string[i]);
		i++;
	}
}

uint8_t availableUART(void)
{
	/*
		Verifica si hay un dato recibido disponible.

		Retorna:
		1 si hay dato recibido
		0 si no hay dato recibido
	*/
	if (UCSR0A & (1 << RXC0))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

char readChar(void)
{
	/*
		Lee un caracter recibido.

		Importante:
		Antes de usar esta funcion, se recomienda verificar
		availableUART().
	*/

	if (availableUART())
	{
		return UDR0;
	}
	else
	{
		return '\0';
	}
}

void writeNumber(uint16_t numero)
{
	char buffer[6];
	uint8_t i = 0;
	uint8_t j;
	char temp;

	if (numero == 0)
	{
		writeChar('0');
		return;
	}

	/*
		Convertir numero a caracteres, pero queda invertido.
	*/
	while (numero > 0)
	{
		buffer[i] = (numero % 10) + '0';
		numero = numero / 10;
		i++;
	}

	/*
		Invertir el arreglo para imprimir el numero correctamente.
	*/
	for (j = 0; j < i / 2; j++)
	{
		temp = buffer[j];
		buffer[j] = buffer[i - j - 1];
		buffer[i - j - 1] = temp;
	}

	/*
		Enviar numero.
	*/
	for (j = 0; j < i; j++)
	{
		writeChar(buffer[j]);
	}
}

void writeNewLine(void)
{
	writeChar('\r');
	writeChar('\n');
}