/*
 * UART.c
 *
 * Created:
 * Author:
 * Description:
 * Libreria para comunicacion USART/UART en ATmega328P.
 */

/****************************************/
// Librerias

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include "UART.h"

/****************************************/
// Funciones

void USART_Init(void)
{
	// Configurar TX
	DDRD |= (1 << DDD1);

	// Configurar RX
	DDRD &= ~(1 << DDD0);

	// Configurar baudrate a 9600
	UBRR0H = 0;
	UBRR0L = 103;

	// Usar modo asincrono
	UCSR0A = 0x00;

	// Activar recepcion y transmision
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);

	// Usar formato 8N1
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void USART_Transmit(char data)
{
	while (!(UCSR0A & (1 << UDRE0)))
	{
	}

	UDR0 = data;
}

void USART_SendString(const char *text)
{
	while (*text != '\0')
	{
		USART_Transmit(*text);
		text++;
	}
}

char USART_Receive(void)
{
	while (!(UCSR0A & (1 << RXC0)))
	{
	}

	return UDR0;
}

uint8_t USART_Available(void)
{
	if (UCSR0A & (1 << RXC0))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

uint8_t USART_ReadIfAvailable(char *data)
{
	if (data == 0)
	{
		return 0;
	}
	
	if (USART_Available())
	{
		*data = UDR0;
		return 1;
	}
	
	return 0;
}

void USART_NewLine(void)
{
	USART_SendString("\r\n");
}

void USART_SendNumber(uint16_t numero)
{
	char buffer[6];
	uint8_t i = 0;
	uint8_t j = 0;
	char temp = 0;

	if (numero == 0)
	{
		USART_Transmit('0');
		return;
	}

	while (numero > 0)
	{
		buffer[i] = (numero % 10) + '0';
		numero = numero / 10;
		i++;
	}

	for (j = 0; j < (i / 2); j++)
	{
		temp = buffer[j];
		buffer[j] = buffer[i - 1 - j];
		buffer[i - 1 - j] = temp;
	}

	buffer[i] = '\0';

	USART_SendString(buffer);
}
