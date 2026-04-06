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

#define T1Value 0xE17B
/****************************************/
// Function prototypes
void  setup ();
void initADC ();
void initTMR1();


/****************************************/
// Main Function

int main(void)
{
	cli();
	setup();
	initADC();
	// HABILITAR INTERRUPCIONES
	ADCSRA |= (1 << ADIE);
	
	//INICIAR ADC
	ADCSRA |= (1 << ADSC);
	TIMSK1 |= (1 << TOIE1);
	sei();
	while (1)
	{
	}
}


/****************************************/
// NON-Interrupt subroutines
void setup ()
{
	// F_sistema = 1Mhz
	CLKPR = (1 << CLKPCE);
	CLKPR = (1 << CLKPS2);
	initTMR1()
	
	//CONFIG. SALIDAS/ENTRADAS
	DDRD = 0xFF; // TODO EL PUERTO D COMMO SALIDA
	PORTD = 0x00; // TODO APAGADO
	UCSR0B = 0x00; // APAGAR PINES DEBIDO A UART
}

void initADC ()
{
	ADMUX = 0;
	// Aref = AVcc; Just. a la izq; selecci[on de ADC6
	ADMUX |= (1 << REFS0) | (1 << ADLAR) | ( 1 << MUX2) | ( 1 << MUX1) ;
	ADCSRA = 0 ;
	
	// HABILITAR ADC Y SELECCIONAR PRESCALER = 8 -> 1 MHz/8 = 125 kHz
	ADCSRA |=  (1 << ADEN) | ( 1 << ADPS1) | (1 << ADPS0);

	
}

void initTMR1 ()
{
	TCCR1A = 0;
	TCCR1B = 0;
	TCCR1B |= (1 << CS11) | (1 << CS10);
	TCNT1 = T1Value;
	
}

/****************************************/
// Interrupt routines
ISR(ADC_vect)
{
	PORTD = ADCH;
	
}

ISR(TIMER1_OVF_vect)
{
	ADCSRA |= (1 << ADSC);
}


