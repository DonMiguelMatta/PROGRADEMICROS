/****************************************/
// Encabezado (Libraries)
#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include "ServoPWM.h"

/****************************************/
// Function prototypes

/****************************************/
// Main Function

/****************************************/
// NON-Interrupt subroutines
static softpwm_t *softPWMDevice = 0;
volatile unsigned char softPWMCount = 0;

void ADC_init(void)
{
	// AVcc como referencia
	ADMUX = 0;
	ADMUX |= (1 << REFS0);

	// Habilitar ADC
	ADCSRA = 0;
	ADCSRA |= (1 << ADEN);

	// Prescaler 128
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

	// Sin configuracion extra
	ADCSRB = 0;
}

unsigned int ADC_read(unsigned char channel)
{
	// Seleccionar canal
	ADMUX &= 0xF0;
	ADMUX |= (channel & 0x0F);

	// Iniciar conversion
	ADCSRA |= (1 << ADSC);

	// Esperar fin
	while (ADCSRA & (1 << ADSC));

	// Retornar ADC
	return ADC;
}

void Servo_init(void)
{
	// Limpiar Timer 1
	TCCR1A = 0;
	TCCR1B = 0;

	// Fast PWM con ICR1
	TCCR1A |= (1 << WGM11);
	TCCR1B |= (1 << WGM13) | (1 << WGM12);

	// Prescaler 8
	TCCR1B |= (1 << CS11);

	// Periodo 20 ms
	ICR1 = 39999;
}

void Servo_attach(servo_t *servo, servo_channel_t channel)
{
	// Guardar canal
	servo->channel = channel;

	if (channel == SERVO_OC1A)
	{
		// D9 = PB1
		DDRB |= (1 << PB1);

		// Habilitar OC1A
		TCCR1A |= (1 << COM1A1);

		// Centro inicial
		OCR1A = 3000;
	}
	else
	{
		// D10 = PB2
		DDRB |= (1 << PB2);

		// Habilitar OC1B
		TCCR1A |= (1 << COM1B1);

		// Centro inicial
		OCR1B = 3000;
	}
}

void Servo_writeCounts(servo_t *servo, unsigned int value)
{
	if (servo->channel == SERVO_OC1A)
	{
		OCR1A = value;
	}
	else
	{
		OCR1B = value;
	}
}

unsigned int Servo_mapADC(unsigned int adcValue)
{
	// Mapear ADC a servo
	return (1000 + ((unsigned long)adcValue * 4000UL) / 1023UL);
}

void Servo_writeADC(servo_t *servo, unsigned int adcValue)
{
	Servo_writeCounts(servo, Servo_mapADC(adcValue));
}

void SoftPWM_init(void)
{
	// Limpiar Timer 2
	TCCR2A = 0;
	TCCR2B = 0;

	// Modo CTC
	TCCR2A |= (1 << WGM21);

	// Comparacion
	OCR2A = 79;

	// Prescaler 8
	TCCR2B |= (1 << CS21);

	// Habilitar interrupcion
	TIMSK2 |= (1 << OCIE2A);

	// Interrupciones globales
	sei();
}

void SoftPWM_attach(softpwm_t *led, volatile uint8_t *ddr, volatile uint8_t *port, uint8_t pin)
{
	// Guardar direccion
	led->ddr = ddr;
	led->port = port;
	led->pin = pin;
	led->duty = 0;

	// Pin como salida
	*(led->ddr) |= (1 << led->pin);

	// Estado inicial bajo
	*(led->port) &= ~(1 << led->pin);

	// Guardar dispositivo
	softPWMDevice = led;
}

void SoftPWM_write(softpwm_t *led, unsigned char dutyValue)
{
	led->duty = dutyValue;
}

unsigned char SoftPWM_mapADC(unsigned int adcValue)
{
	// Mapear ADC a 8 bits
	return (unsigned char)(((unsigned long)adcValue * 255UL) / 1023UL);
}

void SoftPWM_writeADC(softpwm_t *led, unsigned int adcValue)
{
	SoftPWM_write(led, SoftPWM_mapADC(adcValue));
}

/****************************************/
// Interrupt routines
ISR(TIMER2_COMPA_vect)
{
	if (softPWMDevice == 0)
	{
		return;
	}

	// Incrementar contador
	softPWMCount++;

	// Inicio del ciclo
	if (softPWMCount == 0)
	{
		if (softPWMDevice->duty > 0)
		{
			*(softPWMDevice->port) |= (1 << softPWMDevice->pin);
		}
		else
		{
			*(softPWMDevice->port) &= ~(1 << softPWMDevice->pin);
		}
	}

	// Apagar al llegar al duty
	if (softPWMCount == softPWMDevice->duty)
	{
		*(softPWMDevice->port) &= ~(1 << softPWMDevice->pin);
	}
}