/*
 * testpwm.c
 *
 * Created: 08/04/2026 04:55:53 p. m.
 * Author : migue
 */ 
#define F_CPU 16000
#include <avr/io.h>
#include <util/delay.h>
#include "PWM/PWM0.h"


/*
 * NombreProgra.c
 *
 * Created: 
 * Author: 
 * Description: 
 */
/****************************************/
// Encabezado (Libraries)

/****************************************/
// Function prototypes

/****************************************/
// Main Function
int main(void)
{
	uint8_t dutyCycle = 255;
	CLKPR = (1 << CLKPCE);
	CLKPR = (1 << CLKPS2);
	initPWM0();
	void updateDutyCicle0A(uint8_t);
	void updateDutyCicle0B(uint8_t);
	while (1)
	{
		void updateDutyCicle0A(uint8_t);
		void updateDutyCicle0B(uint8_t);
		dutyCycle++;
		_delay_ms(1);
	}
}
/****************************************/
// NON-Interrupt subroutines


/****************************************/
// Interrupt routines
