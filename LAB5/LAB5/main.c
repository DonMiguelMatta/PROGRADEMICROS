/*
 * Brazo Robotico - 4 Servos
 *
 * Description:
 * Control de 4 servomotores en D3, D5, D6 y D9
 * mediante potenciometros conectados en A0, A1, A2 y A3.
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
servo_t servoD3;
servo_t servoD5;
servo_t servoD6;
servo_t servoD9;

int main(void)
{
	unsigned int adcServoD3 = 0;
	unsigned int adcServoD5 = 0;
	unsigned int adcServoD6 = 0;
	unsigned int adcServoD9 = 0;

	setup();

	while (1)
	{
		// A0 -> servo en D3
		adcServoD3 = ADC_read(0);
		Servo_writeADC(&servoD3, adcServoD3);

		// A1 -> servo en D5
		adcServoD5 = ADC_read(1);
		Servo_writeADC(&servoD5, adcServoD5);

		// A2 -> servo en D6
		adcServoD6 = ADC_read(2);
		Servo_writeADC(&servoD6, adcServoD6);

		// A3 -> servo en D9
		adcServoD9 = ADC_read(3);
		Servo_writeADC(&servoD9, adcServoD9);
	}
}

/****************************************/
// NON-Interrupt subroutines
void setup(void)
{
	ADC_init();

	Servo_init();

	Servo_attach(&servoD3, SERVO_D3);
	Servo_attach(&servoD5, SERVO_D5);
	Servo_attach(&servoD6, SERVO_D6);
	Servo_attach(&servoD9, SERVO_D9);
}

/****************************************/
// Interrupt routines