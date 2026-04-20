/*
 * Lab5
 *
 * Author: Miguel Donis 22993
 * Description:
 */
/****************************************/
// Encabezado (Libraries)
#define F_CPU 16000000UL
#include <avr/io.h>
#include "ServoPWM.h"

/****************************************/
// Function prototypes
void setup(void);

/****************************************/
// Main Function
servo_t servo1;
servo_t servo2;
softpwm_t led1;

int main(void)
{
	unsigned int adcServo1 = 0;
	unsigned int adcServo2 = 0;
	unsigned int adcLed = 0;

	setup();

	while (1)
	{
		// A7 -> servo en D10
		adcServo1 = ADC_read(7);
		Servo_writeADC(&servo1, adcServo1);

		// A6 -> servo en D9
		adcServo2 = ADC_read(6);
		Servo_writeADC(&servo2, adcServo2);

		// A5 -> led en D3
		adcLed = ADC_read(5);
		SoftPWM_writeADC(&led1, adcLed);
	}
}

/****************************************/
// NON-Interrupt subroutines
void setup(void)
{
	ADC_init();

	Servo_init();
	Servo_attach(&servo1, SERVO_OC1B);
	Servo_attach(&servo2, SERVO_OC1A);

	SoftPWM_init();
	SoftPWM_attach(&led1, &DDRD, &PORTD, PD3);
}

/****************************************/
// Interrupt routines