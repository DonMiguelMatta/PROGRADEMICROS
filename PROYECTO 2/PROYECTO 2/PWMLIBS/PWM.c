/*
 * PWM.c
 *
 * Created:
 * Author:
 * Description:
 * Control de servomotores por software usando Timer1.
 */

/****************************************/
// Encabezado (Libraries)
#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include "PWM.h"

/****************************************/
// Constantes

#define SERVO_MAX_DEVICES     4
#define SERVO_TICK_US         10
#define SERVO_PERIOD_TICKS    2000

#define SERVO_MIN_US          500
#define SERVO_CENTER_US       1500
#define SERVO_MAX_US          3000

/****************************************/
// Variables globales internas

static servo_t *servoDevices[SERVO_MAX_DEVICES];
static volatile unsigned char servoTotal = 0;
static volatile unsigned int servoCounter = 0;

/****************************************/
// Function prototypes internos

static void Servo_ISRHandler(void);

/****************************************/
// NON-Interrupt subroutines

void Servo_init(void)
{
	unsigned char i = 0;

	for (i = 0; i < SERVO_MAX_DEVICES; i++)
	{
		servoDevices[i] = 0;
	}

	servoTotal = 0;
	servoCounter = 0;

	/*
		Limpiar Timer1.
	*/
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;

	/*
		Timer1 en modo CTC.
	*/
	TCCR1B |= (1 << WGM12);

	/*
		Comparacion cada 10 us.

		F_CPU = 16 MHz
		Prescaler = 8
		Tiempo por cuenta = 0.5 us

		OCR1A = 19
		20 cuentas * 0.5 us = 10 us
	*/
	OCR1A = 19;

	/*
		Prescaler 8.
	*/
	TCCR1B |= (1 << CS11);

	/*
		Habilitar interrupcion por comparacion A.
	*/
	TIMSK1 |= (1 << OCIE1A);
}

void Servo_attach(servo_t *servo, servo_channel_t channel)
{
	if (servo == 0)
	{
		return;
	}

	switch (channel)
	{
		case SERVO_D3:
			/*
				D3 = PD3
			*/
			servo->ddr = &DDRD;
			servo->port = &PORTD;
			servo->pin = PD3;
			break;

		case SERVO_D5:
			/*
				D5 = PD5
			*/
			servo->ddr = &DDRD;
			servo->port = &PORTD;
			servo->pin = PD5;
			break;

		case SERVO_D6:
			/*
				D6 = PD6
			*/
			servo->ddr = &DDRD;
			servo->port = &PORTD;
			servo->pin = PD6;
			break;

		case SERVO_D9:
			/*
				D9 = PB1
			*/
			servo->ddr = &DDRB;
			servo->port = &PORTB;
			servo->pin = PB1;
			break;

		default:
			return;
	}

	/*
		Pin como salida.
	*/
	*(servo->ddr) |= (1 << servo->pin);

	/*
		Estado inicial bajo.
	*/
	*(servo->port) &= ~(1 << servo->pin);

	/*
		Posicion inicial al centro.
	*/
	servo->pulseTicks = SERVO_CENTER_US / SERVO_TICK_US;

	/*
		Guardar servo en arreglo interno.
	*/
	if (servoTotal < SERVO_MAX_DEVICES)
	{
		servoDevices[servoTotal] = servo;
		servoTotal++;
	}
}

void Servo_writeMicroseconds(servo_t *servo, unsigned int pulseMicroseconds)
{
	unsigned char sreg = 0;

	if (servo == 0)
	{
		return;
	}

	if (pulseMicroseconds < SERVO_MIN_US)
	{
		pulseMicroseconds = SERVO_MIN_US;
	}

	if (pulseMicroseconds > SERVO_MAX_US)
	{
		pulseMicroseconds = SERVO_MAX_US;
	}

	/*
		Proteger escritura porque pulseTicks se usa dentro de la interrupcion.
	*/
	sreg = SREG;
	cli();

	servo->pulseTicks = (pulseMicroseconds + (SERVO_TICK_US / 2)) / SERVO_TICK_US;

	SREG = sreg;
}

unsigned int Servo_mapADC(unsigned int adcValue)
{
	if (adcValue > 1023)
	{
		adcValue = 1023;
	}

	/*
		Mapear ADC 0-1023 a pulso de 500 us a 3000 us.
	*/
	return (SERVO_MIN_US + ((unsigned long)adcValue * (SERVO_MAX_US - SERVO_MIN_US)) / 1023UL);
}

void Servo_writeADC(servo_t *servo, unsigned int adcValue)
{
	Servo_writeMicroseconds(servo, Servo_mapADC(adcValue));
}

unsigned int Servo_mapAngle(unsigned char angle)
{
	if (angle > 180)
	{
		angle = 180;
	}

	/*
		Mapear angulo 0-180 a pulso de 500 us a 3000 us.
	*/
	return (SERVO_MIN_US + ((unsigned long)angle * (SERVO_MAX_US - SERVO_MIN_US)) / 180UL);
}

void Servo_writeAngle(servo_t *servo, unsigned char angle)
{
	Servo_writeMicroseconds(servo, Servo_mapAngle(angle));
}

unsigned char Servo_getAngle(servo_t *servo)
{
	unsigned int pulseMicroseconds = 0;
	unsigned char angle = 0;

	if (servo == 0)
	{
		return 90;
	}

	pulseMicroseconds = servo->pulseTicks * SERVO_TICK_US;

	if (pulseMicroseconds < SERVO_MIN_US)
	{
		pulseMicroseconds = SERVO_MIN_US;
	}

	if (pulseMicroseconds > SERVO_MAX_US)
	{
		pulseMicroseconds = SERVO_MAX_US;
	}

	angle = (unsigned char)(((unsigned long)(pulseMicroseconds - SERVO_MIN_US) * 180UL) / (SERVO_MAX_US - SERVO_MIN_US));

	return angle;
}

/****************************************/
// Funcion interna para generar PWM de servos

static void Servo_ISRHandler(void)
{
	unsigned char i = 0;
	servo_t *servo = 0;

	servoCounter++;

	/*
		Inicio de nuevo periodo de 20 ms.
	*/
	if (servoCounter >= SERVO_PERIOD_TICKS)
	{
		servoCounter = 0;

		for (i = 0; i < servoTotal; i++)
		{
			servo = servoDevices[i];

			if (servo != 0)
			{
				*(servo->port) |= (1 << servo->pin);
			}
		}
	}

	/*
		Apagar cada servo cuando alcanza su ancho de pulso.
	*/
	for (i = 0; i < servoTotal; i++)
	{
		servo = servoDevices[i];

		if (servo != 0)
		{
			if (servoCounter == servo->pulseTicks)
			{
				*(servo->port) &= ~(1 << servo->pin);
			}
		}
	}
}

/****************************************/
// Interrupt routines

ISR(TIMER1_COMPA_vect)
{
	Servo_ISRHandler();
}