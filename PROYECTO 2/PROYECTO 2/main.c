/*
 * main.c
 *
 * Created:
 * Author:
 * Description:
 * Brazo robotico con 4 servos, 4 potenciometros y memoria EEPROM.
 * Control por modo manual y reproduccion desde EEPROM.
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
#include "UARTLIBS/UART.h"

/****************************************/
// Constantes

#define SERVOS_USADOS        4
#define POSICIONES_TOTAL     3

#define EEPROM_BASE          0

#define MODO_MANUAL          0
#define MODO_EEPROM          1

#define ANTIREBOTE_TICKS     5

#define BOTON_DDR            DDRB
#define BOTON_PORT           PORTB
#define BOTON_PIN            PINB
#define PIN_GUARDAR          PB3    // D11
#define PIN_REPRODUCIR       PB4    // D12

#define LED_DDR              DDRD
#define LED_PORT             PORTD
#define PIN_LED              PD2    // D2

#define LED_GUARDAR_TICKS    50
#define LED_BLINK_ON_TICKS   20
#define LED_BLINK_OFF_TICKS  20
#define LED_BLINKS_TOTAL     2

/****************************************/
// Estados internos del LED

typedef enum
{
	LED_IDLE = 0,
	LED_ON_FIJO,
	LED_BLINK_ON,
	LED_BLINK_OFF
} EstadoLED;

/****************************************/
// Variables globales

static servo_t servoD3;
static servo_t servoD5;
static servo_t servoD6;
static servo_t servoD9;

static servo_t *servos[SERVOS_USADOS];

volatile uint8_t ticks10ms = 0;

static uint8_t modoActual = MODO_MANUAL;

static uint8_t indiceGuardar = 0;
static uint8_t indiceReproducir = 0;

static EstadoLED estadoLED = LED_IDLE;
static uint8_t tickInicioLED = 0;
static uint8_t contParpadeos = 0;

/****************************************/
// Function prototypes

static void inicializarHardware(void);
static void inicializarBotones(void);
static void inicializarLED(void);

static void actualizarServosManual(void);
static void guardarPosicion(uint8_t idx);
static void cargarPosicion(uint8_t idx);

static uint8_t botonPresionado(uint8_t pin, uint8_t *estadoAnt, uint8_t *ultimoTick);

static void iniciarLEDGuardar(void);
static void iniciarLEDReproducir(void);
static void gestionarLED(void);

static void imprimirNumero(uint8_t n);

/****************************************/
// Main Function

int main(void)
{
	inicializarHardware();

	while (1)
	{
		if (modoActual == MODO_MANUAL)
		{
			actualizarServosManual();
		}

		/****************************************/
		// Boton GUARDAR - D11 / PB3

		{
			static uint8_t estAnt = 1;
			static uint8_t ultTick = 0;

			if (botonPresionado(PIN_GUARDAR, &estAnt, &ultTick))
			{
				guardarPosicion(indiceGuardar);

				writeString("Posicion ");
				imprimirNumero((uint8_t)(indiceGuardar + 1));
				writeString(" guardada");
				writeNewLine();

				indiceGuardar++;

				if (indiceGuardar >= POSICIONES_TOTAL)
				{
					indiceGuardar = 0;
				}

				iniciarLEDGuardar();

				modoActual = MODO_MANUAL;
			}
		}

		/****************************************/
		// Boton REPRODUCIR - D12 / PB4

		{
			static uint8_t estAnt = 1;
			static uint8_t ultTick = 0;

			if (botonPresionado(PIN_REPRODUCIR, &estAnt, &ultTick))
			{
				cargarPosicion(indiceReproducir);

				writeString("Posicion ");
				imprimirNumero((uint8_t)(indiceReproducir + 1));
				writeString(" reproducida");
				writeNewLine();

				indiceReproducir++;

				if (indiceReproducir >= POSICIONES_TOTAL)
				{
					indiceReproducir = 0;
				}

				iniciarLEDReproducir();

				modoActual = MODO_EEPROM;
			}
		}

		gestionarLED();
	}
}

/****************************************/
// NON-Interrupt subroutines

static void inicializarHardware(void)
{
	cli();

	initADC();
	initEEPROM();
	initUART();

	inicializarBotones();
	inicializarLED();

	/*
		Timer1 se usa exclusivamente dentro de la libreria PWM
		para generar los pulsos de los servos.
	*/
	Servo_init();

	servos[0] = &servoD3;
	servos[1] = &servoD5;
	servos[2] = &servoD6;
	servos[3] = &servoD9;

	Servo_attach(&servoD3, SERVO_D3);
	Servo_attach(&servoD5, SERVO_D5);
	Servo_attach(&servoD6, SERVO_D6);
	Servo_attach(&servoD9, SERVO_D9);

	/*
		Timer0 para base de tiempo de 10 ms.
		Se usa para debounce y LED.
	*/
	initTimer0_CTC(timer0_usToTicks(10000UL, TIMER_PRESCALER_1024), TIMER_PRESCALER_1024);
	TIMSK0 |= (1 << OCIE0A);

	sei();

	writeString("Sistema iniciado");
	writeNewLine();
}

static void inicializarBotones(void)
{
	/*
		PB3 y PB4 como entradas con pull-up interno.
		Boton presionado = 0.
		Boton suelto = 1.
	*/
	BOTON_DDR &= ~((1 << PIN_GUARDAR) | (1 << PIN_REPRODUCIR));
	BOTON_PORT |= (1 << PIN_GUARDAR) | (1 << PIN_REPRODUCIR);
}

static void inicializarLED(void)
{
	LED_DDR |= (1 << PIN_LED);
	LED_PORT &= ~(1 << PIN_LED);
}

static void actualizarServosManual(void)
{
	uint8_t i;
	uint16_t valorADC;

	for (i = 0; i < SERVOS_USADOS; i++)
	{
		valorADC = readADC(i);
		Servo_writeADC(servos[i], valorADC);
	}
}

static void guardarPosicion(uint8_t idx)
{
	uint16_t dir;
	uint8_t datos[SERVOS_USADOS];
	uint8_t i;

	if (idx >= POSICIONES_TOTAL)
	{
		return;
	}

	dir = EEPROM_BASE + ((uint16_t)idx * SERVOS_USADOS);

	for (i = 0; i < SERVOS_USADOS; i++)
	{
		datos[i] = Servo_getAngle(servos[i]);
	}

	writeEEPROM_Block(dir, datos, SERVOS_USADOS);
}

static void cargarPosicion(uint8_t idx)
{
	uint16_t dir;
	uint8_t datos[SERVOS_USADOS];
	uint8_t i;

	if (idx >= POSICIONES_TOTAL)
	{
		return;
	}

	dir = EEPROM_BASE + ((uint16_t)idx * SERVOS_USADOS);

	readEEPROM_Block(dir, datos, SERVOS_USADOS);

	for (i = 0; i < SERVOS_USADOS; i++)
	{
		if (datos[i] > 180)
		{
			datos[i] = 90;
		}

		Servo_writeAngle(servos[i], datos[i]);
	}
}

static uint8_t botonPresionado(uint8_t pin, uint8_t *estadoAnt, uint8_t *ultimoTick)
{
	uint8_t estadoActual;
	uint8_t tickActual;
	uint8_t evento = 0;

	estadoActual = (BOTON_PIN & (1 << pin)) ? 1 : 0;
	tickActual = ticks10ms;

	if ((*estadoAnt == 1) && (estadoActual == 0))
	{
		if ((uint8_t)(tickActual - *ultimoTick) >= ANTIREBOTE_TICKS)
		{
			evento = 1;
			*ultimoTick = tickActual;
		}
	}

	*estadoAnt = estadoActual;

	return evento;
}

static void iniciarLEDGuardar(void)
{
	LED_PORT |= (1 << PIN_LED);
	estadoLED = LED_ON_FIJO;
	tickInicioLED = ticks10ms;
}

static void iniciarLEDReproducir(void)
{
	LED_PORT |= (1 << PIN_LED);
	estadoLED = LED_BLINK_ON;
	tickInicioLED = ticks10ms;
	contParpadeos = 0;
}

static void gestionarLED(void)
{
	uint8_t tickActual;
	uint8_t ticksTransc;

	tickActual = ticks10ms;
	ticksTransc = (uint8_t)(tickActual - tickInicioLED);

	switch (estadoLED)
	{
		case LED_ON_FIJO:
			if (ticksTransc >= LED_GUARDAR_TICKS)
			{
				LED_PORT &= ~(1 << PIN_LED);
				estadoLED = LED_IDLE;
			}
			break;

		case LED_BLINK_ON:
			if (ticksTransc >= LED_BLINK_ON_TICKS)
			{
				LED_PORT &= ~(1 << PIN_LED);
				estadoLED = LED_BLINK_OFF;
				tickInicioLED = tickActual;
				contParpadeos++;
			}
			break;

		case LED_BLINK_OFF:
			if (ticksTransc >= LED_BLINK_OFF_TICKS)
			{
				if (contParpadeos < LED_BLINKS_TOTAL)
				{
					LED_PORT |= (1 << PIN_LED);
					estadoLED = LED_BLINK_ON;
					tickInicioLED = tickActual;
				}
				else
				{
					estadoLED = LED_IDLE;
				}
			}
			break;

		case LED_IDLE:
		default:
			break;
	}
}

static void imprimirNumero(uint8_t n)
{
	writeChar((char)('0' + (n % 10)));
}

/****************************************/
// Interrupt routines

ISR(TIMER0_COMPA_vect)
{
	ticks10ms++;
}