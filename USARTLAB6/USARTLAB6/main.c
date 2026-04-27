/*
 * NombreProgra.c
 *
 * Created:
 * Author:
 * Description: 
 */
/****************************************/
// Encabezado (Libraries)

#define F_CPU 16000000UL
#include <avr/io.h>
#include "USARTLIB/ascii_binario.h"

/****************************************/
// Function prototypes

void USART_Init(void);
void USART_Transmit(char data);
void USART_SendString(char *text);
char USART_Receive(void);

void ADC_Init(void);
uint16_t ADC_Read(uint8_t channel);

void Enviar_Menu(void);
void Enviar_Numero(uint16_t numero);
void Mostrar_Byte_En_LEDs(uint8_t dato);
void Delay_ms_simple(unsigned int tiempo);

/****************************************/
// Main Function

int main(void)
{
    char opcion;
    char caracterASCII;
    uint8_t valorBinario;
    uint16_t potValue;

    // D2-D7 como salida
    DDRD |= (1 << DDD2) | (1 << DDD3) | (1 << DDD4) |
            (1 << DDD5) | (1 << DDD6) | (1 << DDD7);

    // D8-D9 como salida
    DDRB |= (1 << DDB0) | (1 << DDB1);

    // Limpia leds
    PORTD &= ~((1 << PORTD2) | (1 << PORTD3) | (1 << PORTD4) |
               (1 << PORTD5) | (1 << PORTD6) | (1 << PORTD7));
    PORTB &= ~((1 << PORTB0) | (1 << PORTB1));

    USART_Init();
    ADC_Init();

    while (1)
    {
		//abrir menu 
        Enviar_Menu();
        opcion = USART_Receive();
        USART_Transmit(opcion);
        USART_SendString("\r\n");

        if (opcion == '1')
        {
			//lectura ADC pot
            potValue = ADC_Read(4);
            USART_SendString("Potenciometro = ");
            Enviar_Numero(potValue);
            USART_SendString("\r\n\r\n");
        }
        else if (opcion == '2')
        {
	        uint8_t valorBinario;
	        uint8_t k;

	        USART_SendString("Ingrese un caracter ASCII: ");
	        caracterASCII = USART_Receive();
	        USART_Transmit(caracterASCII);
	        USART_SendString("\r\n");

	        valorBinario = ASCII_a_Binario(caracterASCII);

	        USART_SendString("Codigo ASCII binario = ");
	        for (k = 0; k < 8; k++)
	        {
		        if (valorBinario & (1 << (7 - k)))
		        {
			        USART_Transmit('1');
		        }
		        else
		        {
			        USART_Transmit('0');
		        }
	        }
	        USART_SendString("\r\n");

	        ASCII_En_LEDs_D2_D9(caracterASCII);

	        USART_SendString("Mostrado en leds\r\n\r\n");
        }
        else
        {
            USART_SendString("Opcion no valida\r\n\r\n");
        }

        Delay_ms_simple(200);
    }
}

/****************************************/
// NON-Interrupt subroutines

void USART_Init(void)
{
    // Baud rate 9600 para 16 MHz
    UBRR0H = 0;
    UBRR0L = 103;

    // Modo asincrono normal
    UCSR0A = 0x00;

    // Habilita receptor y transmisor
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);

    // 8 bits, sin paridad, 1 bit de parada
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void USART_Transmit(char data)
{
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

void USART_SendString(char *text)
{
    while (*text != '\0')
    {
        USART_Transmit(*text);
        text++;
    }
}

char USART_Receive(void)
{
    while (!(UCSR0A & (1 << RXC0)));
    return UDR0;
}

void ADC_Init(void)
{
    // Referencia AVcc
    ADMUX = (1 << REFS0);

    // Habilita ADC, prescaler 128
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t ADC_Read(uint8_t channel)
{
    ADMUX &= 0xF0;
    ADMUX |= (channel & 0x0F);

    ADCSRA |= (1 << ADSC);

    while (ADCSRA & (1 << ADSC));

    return ADC;
}

void Enviar_Menu(void)
{
    USART_SendString("====== MENU ======\r\n");
    USART_SendString("1. Leer Potenciometro\r\n");
    USART_SendString("2. Enviar Ascii\r\n");
    USART_SendString("Seleccione una opcion: ");
}

//convertir numero a ascii y enviar por serial
void Enviar_Numero(uint16_t numero) //16 bits sin signo
{
    char buffer[6];
    uint8_t i = 0;
    uint8_t j;
    char temp;

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

    for (j = 0; j < i / 2; j++)
    {
        temp = buffer[j];
        buffer[j] = buffer[i - 1 - j];
        buffer[i - 1 - j] = temp;
    }

    buffer[i] = '\0';
    USART_SendString(buffer);
}

//Mostrar ascii en leds
void Mostrar_Byte_En_LEDs(uint8_t dato)
{
	// Apaga primero todos los leds
	PORTD &= ~((1 << PORTD2) | (1 << PORTD3) | (1 << PORTD4) |
	(1 << PORTD5) | (1 << PORTD6) | (1 << PORTD7));
	PORTB &= ~((1 << PORTB0) | (1 << PORTB1));

	// Espera 15 ms
	Delay_ms_simple(15);

	// Enciende con el nuevo valor
	PORTD |= ((dato & 0x3F) << 2);
	PORTB |= ((dato >> 6) & 0x03);
}

//delay
void Delay_ms_simple(unsigned int tiempo)
{
    unsigned int i;
    unsigned int j;

    for (i = 0; i < tiempo; i++)
    {
        for (j = 0; j < 1600; j++)
        {
        }
    }
}

/****************************************/
// Interrupt routines