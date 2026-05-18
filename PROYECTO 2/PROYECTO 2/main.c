/*
 * main.c
 *
 * Created:
 * Author:
 * Description:
 * Brazo robotico con 4 servos, 4 potenciometros y memoria EEPROM
 */

/****************************************/
// Encabezado (Libraries)

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "EEPROM/EEPROM.h"
#include "ADCLIBS/ADC.h"
#include "PWMLIBS/PWM.h"
#include "TIMERSLIBS/TIMER.h"

/****************************************/
// Function prototypes

void setup(void);

void initBotones(void);
void initLEDModo(void);

void actualizarServosManual(void);
void guardarPosicionEEPROM(unsigned char posicion);
void cargarPosicionEEPROM(unsigned char posicion);

unsigned char botonGuardarPresionado(void);
unsigned char botonReproducirPresionado(void);

void actualizarLEDModo(void);

/****************************************/
// Constants

#define SERVOS_USADOS             4
#define POSICIONES_TOTAL          3

#define EEPROM_BASE_POSICIONES    0
#define EEPROM_BYTES_POSICION     SERVOS_USADOS

#define MODO_MANUAL               0
#define MODO_EEPROM               1

#define ANTIREBOTE_TICKS          5

#define BOTON_DDR                 DDRB
#define BOTON_PORT                PORTB
#define BOTON_PIN                 PINB

#define BOTON_GUARDAR_PIN         PB3
#define BOTON_REPRODUCIR_PIN      PB4

#define LED_MODO_DDR              DDRB
#define LED_MODO_PORT             PORTB
#define LED_MODO_PIN              PB5

/****************************************/
// Global variables

servo_t servoD3;
servo_t servoD5;
servo_t servoD6;
servo_t servoD9;

servo_t *servos[SERVOS_USADOS] =
{
	&servoD3,
	&servoD5,
	&servoD6,
	&servoD9
};

volatile unsigned char contador10ms = 0;

unsigned char modoFuncionamiento = MODO_MANUAL;

unsigned char posicionGuardar = 0;
unsigned char posicionReproducir = 0;

/****************************************/
// Main Function

int main(void)
{
	setup();

	while (1)
	{
		if (modoFuncionamiento == MODO_MANUAL)
		{
			actualizarServosManual();
		}

		if (botonGuardarPresionado())
		{
			guardarPosicionEEPROM(posicionGuardar);

			posicionGuardar++;

			if (posicionGuardar >= POSICIONES_TOTAL)
			{
				posicionGuardar = 0;
			}

			modoFuncionamiento = MODO_MANUAL;
		}

		if (botonReproducirPresionado())
		{
			cargarPosicionEEPROM(posicionReproducir);

			posicionReproducir++;

			if (posicionReproducir >= POSICIONES_TOTAL)
			{
				posicionReproducir = 0;
			}

			modoFuncionamiento = MODO_EEPROM;
		}

		actualizarLEDModo();
	}
}

/****************************************/
// NON-Interrupt subroutines

void setup(void)
{
	cli();

	initADC();
	initEEPROM();

	initBotones();
	initLEDModo();

	/*
		Servos en D3, D5, D6 y D9.
		Esta funcion configura Timer1 para PWM por software.
	*/
	Servo_init();

	Servo_attach(&servoD3, SERVO_D3);
	Servo_attach(&servoD5, SERVO_D5);
	Servo_attach(&servoD6, SERVO_D6);
	Servo_attach(&servoD9, SERVO_D9);

	/*
		Timer0 para base de tiempo de botones.
		Periodo aproximado: 10 ms.
	*/
	initTimer0_CTC(timer0_usToTicks(10000, TIMER_PRESCALER_1024), TIMER_PRESCALER_1024);
	TIMSK0 |= (1 << OCIE0A);

	sei();
}

void initBotones(void)
{
	/*
		Boton guardar: D11 / PB3
		Boton reproducir: D12 / PB4

		Se usan pull-ups internos.
		Los botones van conectados hacia GND.
	*/
	BOTON_DDR &= ~((1 << BOTON_GUARDAR_PIN) |
	               (1 << BOTON_REPRODUCIR_PIN));

	BOTON_PORT |= (1 << BOTON_GUARDAR_PIN) |
	              (1 << BOTON_REPRODUCIR_PIN);
}

void initLEDModo(void)
{
	/*
		LED de modo: D13 / PB5
	*/
	LED_MODO_DDR |= (1 << LED_MODO_PIN);
	LED_MODO_PORT &= ~(1 << LED_MODO_PIN);
}

void actualizarServosManual(void)
{
	unsigned int adcServoD3 = 0;
	unsigned int adcServoD5 = 0;
	unsigned int adcServoD6 = 0;
	unsigned int adcServoD9 = 0;

	/*
		A0 -> servo en D3
	*/
	adcServoD3 = readADC(0);
	Servo_writeADC(&servoD3, adcServoD3);

	/*
		A1 -> servo en D5
	*/
	adcServoD5 = readADC(1);
	Servo_writeADC(&servoD5, adcServoD5);

	/*
		A2 -> servo en D6
	*/
	adcServoD6 = readADC(2);
	Servo_writeADC(&servoD6, adcServoD6);

	/*
		A3 -> servo en D9
	*/
	adcServoD9 = readADC(3);
	Servo_writeADC(&servoD9, adcServoD9);
}

void guardarPosicionEEPROM(unsigned char posicion)
{
	unsigned int direccionInicial = 0;
	unsigned char datos[SERVOS_USADOS];
	unsigned char i = 0;

	if (posicion >= POSICIONES_TOTAL)
	{
		return;
	}

	direccionInicial = EEPROM_BASE_POSICIONES + (posicion * EEPROM_BYTES_POSICION);

	for (i = 0; i < SERVOS_USADOS; i++)
	{
		datos[i] = Servo_getAngle(servos[i]);
	}

	writeEEPROM_Block(direccionInicial, datos, SERVOS_USADOS);
}

void cargarPosicionEEPROM(unsigned char posicion)
{
	unsigned int direccionInicial = 0;
	unsigned char datos[SERVOS_USADOS];
	unsigned char i = 0;

	if (posicion >= POSICIONES_TOTAL)
	{
		return;
	}

	direccionInicial = EEPROM_BASE_POSICIONES + (posicion * EEPROM_BYTES_POSICION);

	readEEPROM_Block(direccionInicial, datos, SERVOS_USADOS);

	for (i = 0; i < SERVOS_USADOS; i++)
	{
		if (datos[i] > 180)
		{
			datos[i] = 90;
		}

		Servo_writeAngle(servos[i], datos[i]);
	}
}

unsigned char botonGuardarPresionado(void)
{
	static unsigned char estadoAnterior = 1;
	static unsigned char ultimoTick = 0;

	unsigned char estadoActual = 0;
	unsigned char evento = 0;
	unsigned char tickActual = 0;

	estadoActual = (BOTON_PIN & (1 << BOTON_GUARDAR_PIN)) ? 1 : 0;
	tickActual = contador10ms;

	if ((estadoAnterior == 1) && (estadoActual == 0))
	{
		if ((unsigned char)(tickActual - ultimoTick) >= ANTIREBOTE_TICKS)
		{
			evento = 1;
			ultimoTick = tickActual;
		}
	}

	estadoAnterior = estadoActual;

	return evento;
}

unsigned char botonReproducirPresionado(void)
{
	static unsigned char estadoAnterior = 1;
	static unsigned char ultimoTick = 0;

	unsigned char estadoActual = 0;
	unsigned char evento = 0;
	unsigned char tickActual = 0;

	estadoActual = (BOTON_PIN & (1 << BOTON_REPRODUCIR_PIN)) ? 1 : 0;
	tickActual = contador10ms;

	if ((estadoAnterior == 1) && (estadoActual == 0))
	{
		if ((unsigned char)(tickActual - ultimoTick) >= ANTIREBOTE_TICKS)
		{
			evento = 1;
			ultimoTick = tickActual;
		}
	}

	estadoAnterior = estadoActual;

	return evento;
}

void actualizarLEDModo(void)
{
	if (modoFuncionamiento == MODO_EEPROM)
	{
		LED_MODO_PORT |= (1 << LED_MODO_PIN);
	}
	else
	{
		LED_MODO_PORT &= ~(1 << LED_MODO_PIN);
	}
}

/****************************************/
// Interrupt routines

ISR(TIMER0_COMPA_vect)
{
	/*
		Base de tiempo para antirebote.
		Aproximadamente cada 10 ms.
	*/
	contador10ms++;
}