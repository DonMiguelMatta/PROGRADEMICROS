/*
 * ContadorBinario
 *
 * Created: 
 * Author:      Miguel Donis
 * Description: contador de 8 bits
 *              
 *              
 */

/****************************************/
// Encabezado (Libraries)
#define F_CPU 16000000
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

/****************************************/
// Function prototypes
void setup(void);

/****************************************/
// Main Function
int main(void)
{
	setup();
	
	uint8_t contador = 0;
	PORTD = contador;
	
	while(1)
	{
		// Boton incrementar — PC0
		if (!(PINC & (1 << PC0)))
		{
			_delay_ms(20);
			if (!(PINC & (1 << PC0)))
			{
				contador++;
				PORTD = contador;
				
				while (!(PINC & (1 << PC0)))
				{
				}
			}
		}
		
		// Boton decrementar — PC1
		if (!(PINC & (1 << PC1)))
		{
			_delay_ms(20);
			if (!(PINC & (1 << PC1)))
			{
				contador--;
				PORTD = contador;
				
				while (!(PINC & (1 << PC1)))
				{
				}
			}
		}
	}
}

/****************************************/
// NON-Interrupt subroutines
void setup(void)
{
	// CONFIGURAR SALIDAS Y ENTRADAS
	
	// PD0-PD7 como salidas -> LEDs
	DDRD = 0xFF;
	PORTD = 0x00;
	
	// PC0 y PC1 como entradas con pull-up interno -> botones
	DDRC &= ~((1 << PC0) | (1 << PC1));
	PORTC |=  ((1 << PC0) | (1 << PC1));
	
	// PB0 y PB1 como salidas
	DDRB |= (1 << PB0) | (1 << PB1);
	PORTB = 0x00;
}

/****************************************/
// Interrupt routines
// No se utilizan interrupciones en este programa