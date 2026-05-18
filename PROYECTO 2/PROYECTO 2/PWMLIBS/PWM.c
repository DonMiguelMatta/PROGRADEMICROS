/*
 * PWM.c
 *
 * Created:
 * Author:
 * Description:
 * Control de servomotores por software usando Timer1.
 */

/****************************************/
// Librerias
#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include "PWM.h"

/****************************************/
// Constantes

#define SERVO_MAX_DEVICES     4
#define SERVO_TICK_US         10
#define SERVO_PERIOD_TICKS    2000
#define SERVO_TIMER_COUNTS    20

#define SERVO_MIN_US          500
#define SERVO_CENTER_US       1500
#define SERVO_MAX_US          2000

/****************************************/
// Variables globales internas

static servo_t *servoDevices[SERVO_MAX_DEVICES];
static volatile unsigned char servoTotal = 0;
static volatile unsigned int servoElapsedTicks = 0;
static volatile unsigned int servoNextEventTicks = 0;

/****************************************/
// Prototipos internos

static void Servo_ISRHandler(void);
static void Servo_setNextCompare(unsigned int intervalTicks);
static unsigned int Servo_findNextEvent(unsigned int elapsedTicks);

/****************************************/
// Funciones

void Servo_init(void)
{
	unsigned char i = 0;

	for (i = 0; i < SERVO_MAX_DEVICES; i++)
	{
		servoDevices[i] = 0;
	}

	servoTotal = 0;
	servoElapsedTicks = 0;
	servoNextEventTicks = SERVO_PERIOD_TICKS;

	// Preparar Timer1 desde cero
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;

	// Usar Timer1 en modo CTC
	TCCR1B |= (1 << WGM12);

	// Calcular eventos de servo con pasos de 10 us
	Servo_setNextCompare(SERVO_PERIOD_TICKS);

	// Arrancar Timer1 con prescaler 8
	TCCR1B |= (1 << CS11);

	// Habilitar interrupcion del Timer1
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
			// Usar pin D3
			servo->ddr = &DDRD;
			servo->port = &PORTD;
			servo->pin = PD3;
			break;

		case SERVO_D5:
			// Usar pin D5
			servo->ddr = &DDRD;
			servo->port = &PORTD;
			servo->pin = PD5;
			break;

		case SERVO_D6:
			// Usar pin D6
			servo->ddr = &DDRD;
			servo->port = &PORTD;
			servo->pin = PD6;
			break;

		case SERVO_D9:
			// Usar pin D9
			servo->ddr = &DDRB;
			servo->port = &PORTB;
			servo->pin = PB1;
			break;

		default:
			return;
	}

	// Configurar pin del servo
	*(servo->ddr) |= (1 << servo->pin);

	// Dejar salida apagada al inicio
	*(servo->port) &= ~(1 << servo->pin);

	// Colocar servo al centro
	servo->pulseTicks = SERVO_CENTER_US / SERVO_TICK_US;

	// Agregar servo a la lista de control
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

	// Actualizar pulso sin chocar con la interrupcion
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

	// Convertir lectura ADC al rango del servo
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

	// Convertir angulo al rango del servo
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
// PWM de servos

static void Servo_setNextCompare(unsigned int intervalTicks)
{
	unsigned long timerCounts = 0;

	if (intervalTicks == 0)
	{
		intervalTicks = 1;
	}

	timerCounts = (unsigned long)intervalTicks * SERVO_TIMER_COUNTS;

	if (timerCounts > 65535UL)
	{
		timerCounts = 65535UL;
	}

	OCR1A = (unsigned int)(timerCounts - 1);
	TCNT1 = 0;
}

static unsigned int Servo_findNextEvent(unsigned int elapsedTicks)
{
	unsigned char i = 0;
	unsigned int nextEvent = SERVO_PERIOD_TICKS;
	unsigned int pulseTicks = 0;
	servo_t *servo = 0;

	for (i = 0; i < servoTotal; i++)
	{
		servo = servoDevices[i];

		if (servo != 0)
		{
			pulseTicks = servo->pulseTicks;

			if ((pulseTicks > elapsedTicks) && (pulseTicks < nextEvent))
			{
				nextEvent = pulseTicks;
			}
		}
	}

	return nextEvent;
}

static void Servo_ISRHandler(void)
{
	unsigned char i = 0;
	servo_t *servo = 0;
	unsigned int nextEvent = 0;
	unsigned int intervalTicks = 0;

	servoElapsedTicks = servoNextEventTicks;

	// Iniciar nuevo periodo de 20 ms
	if (servoElapsedTicks >= SERVO_PERIOD_TICKS)
	{
		servoElapsedTicks = 0;

		for (i = 0; i < servoTotal; i++)
		{
			servo = servoDevices[i];

			if (servo != 0)
			{
				*(servo->port) |= (1 << servo->pin);
			}
		}
	}
	else
	{
		// Apagar servos al terminar su pulso
		for (i = 0; i < servoTotal; i++)
		{
			servo = servoDevices[i];

			if (servo != 0)
			{
				if (servo->pulseTicks <= servoElapsedTicks)
				{
					*(servo->port) &= ~(1 << servo->pin);
				}
			}
		}
	}

	nextEvent = Servo_findNextEvent(servoElapsedTicks);

	if (nextEvent > SERVO_PERIOD_TICKS)
	{
		nextEvent = SERVO_PERIOD_TICKS;
	}

	servoNextEventTicks = nextEvent;
	intervalTicks = servoNextEventTicks - servoElapsedTicks;

	if (intervalTicks == 0)
	{
		intervalTicks = 1;
		servoNextEventTicks = servoElapsedTicks + 1;
	}

	Servo_setNextCompare(intervalTicks);
}

/****************************************/
// Interrupciones

ISR(TIMER1_COMPA_vect)
{
	Servo_ISRHandler();
}
