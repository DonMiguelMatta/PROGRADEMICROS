/*
 * CFile1.c
 *
 * Created: 08/04/2026 05:16:19 p. m.
 *  Author: migue
 */ 

#include "PWM0.h"
void initPWM0()
{
	// OCR0A = PD6 Y OCR0B = PB5
	
	DDRD |= (1<<DDD6) 
	TCCR0A = 0 ;
	TCCR0B = 0;
	// cONFIGURAR MODO INVERTIDO O NO INVERTIDO
	
	if(invert)
	{
		TCCR0A |= (1<<COM0A1) | (1<<COM0A0); //NO INVERTIDO PB6
		}else {
			TCCR0B |= (1<<COM0B1) | (1<<COM0B0);
		}
	}


if(modo)
{
	TCCR0A |= (1<<COM0A1) | (1<<COM0A0); //NO INVERTIDO PB6
	}else {
	TCCR0B |= (1<<COM0B1) | (1<<COM0B0);
}
}
	
	//CONFIGURAR EN MODO FAST PWM
	TCCR0A |= (1<<WGM01) | (1<<WGM00);
	
	//PRESCALER DE B
	TCCR0B |= (1 << CS01);
}

void updateDutyCicle0A(uint8_t duty)
{
	OCR0A = duty;
}

void updateDutyCicle0B(uint8_t duty)

{
	OCR0B = duty;
}