/*
 * main.c
 *
 * Created:
 * Author:
 * Description:
 * Brazo robotico con 4 servos, 4 potenciometros y comunicacion UART.
 */

/****************************************/
// Librerias

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "ADCLIBS/ADC.h"
#include "PWMLIBS/PWM.h"
#include "EEPROM/EEPROM.h"
#include "UARTLIBS/UART.h"

/****************************************/
// Constantes

#define BUTTON_TIME_MS   50
#define TELEMETRY_TIME_MS 2000

#define LED_PIN          PD2
#define BTN_SAVE_PIN     PB3
#define BTN_PLAY_PIN     PB4

#define EEPROM_VALID     0
#define EEPROM_SERVO_D3  1
#define EEPROM_SERVO_D5  2
#define EEPROM_SERVO_D6  3
#define EEPROM_SERVO_D9  4
#define EEPROM_MARKER    0xA5

/****************************************/
// Prototipos

void setup(void);
void initPorts(void);
void initTimer0(void);
void sendMenuUART(void);
void checkUART(void);
void processCommand(char command);
void checkButtons(void);
void savePosition(void);
uint8_t playPosition(void);
void setManualMode(void);
void updatePotentiometers(void);
void updateManualServos(void);
void sendPotTelemetry(void);
uint32_t getTicks1ms(void);

/****************************************/
// Variables globales

servo_t servoD3;
servo_t servoD5;
servo_t servoD6;
servo_t servoD9;

volatile uint32_t ticks1ms = 0;

uint32_t lastButtonTime = 0;
uint32_t lastTelemetryTime = 0;
uint8_t manualMode = 1;

uint16_t adcPotA0 = 0;
uint16_t adcPotA1 = 0;
uint16_t adcPotA2 = 0;
uint16_t adcPotA3 = 0;

/****************************************/
// Funcion principal

int main(void)
{
	uint32_t currentTime = 0;

	cli();

	setup();

	sei();

	sendMenuUART();

	while (1)
	{
		// Leer potenciometros
		updatePotentiometers();
		
		if (manualMode)
		{
			// Mover servos en modo manual
			updateManualServos();
		}

		// Leer botones fisicos
		checkButtons();
		
		// Leer comandos desde terminal
		checkUART();
		
		currentTime = getTicks1ms();
		
		if ((uint32_t)(currentTime - lastTelemetryTime) >= TELEMETRY_TIME_MS)
		{
			lastTelemetryTime = currentTime;
			// Enviar valores a Adafruit por UART
			sendPotTelemetry();
		}
	}
}

/****************************************/
// Funciones

void setup(void)
{
	// Preparar perifericos del sistema
	initPorts();

	initADC();
	
	initEEPROM();

	initUART();

	initTimer0();

	Servo_init();

	Servo_attach(&servoD3, SERVO_D3);
	Servo_attach(&servoD5, SERVO_D5);
	Servo_attach(&servoD6, SERVO_D6);
	Servo_attach(&servoD9, SERVO_D9);
}

void initPorts(void)
{
	// Configurar TX
	DDRD |= (1 << PD1);

	// Configurar RX
	DDRD &= ~(1 << PD0);
	
	// Configurar LED indicador
	DDRD |= (1 << LED_PIN);
	PORTD &= ~(1 << LED_PIN);
	
	// Configurar botones con pull-up interno
	DDRB &= ~((1 << BTN_SAVE_PIN) | (1 << BTN_PLAY_PIN));
	PORTB |= (1 << BTN_SAVE_PIN) | (1 << BTN_PLAY_PIN);
}

void initTimer0(void)
{
	// Configurar Timer0 para contar milisegundos
	TCCR0A = 0;
	TCCR0B = 0;
	TCNT0 = 0;

	TCCR0A |= (1 << WGM01);

	OCR0A = 249;

	TIMSK0 |= (1 << OCIE0A);

	TCCR0B |= (1 << CS01) | (1 << CS00);
}

void sendMenuUART(void)
{
	writeNewLine();
	writeString("Proyecto 2");
	writeNewLine();

	writeString("1: modo manual con potenciometros");
	writeNewLine();

	writeString("2: guardar posicion actual");
	writeNewLine();

	writeString("3: reproducir posicion guardada");
	writeNewLine();
	writeNewLine();
}

void checkUART(void)
{
	char dato = 0;

	if (readCharIfAvailable(&dato))
	{
		if ((dato == '\r') || (dato == '\n'))
		{
			return;
		}

		writeString("Dato recibido: ");
		writeChar(dato);
		writeNewLine();

		processCommand(dato);
	}
}

void processCommand(char command)
{
	if (command == '1')
	{
		setManualMode();
		writeString("Modo manual activo");
		writeNewLine();
	}
	else if (command == '2')
	{
		savePosition();
		writeString("Posicion guardada en EEPROM");
		writeNewLine();
	}
	else if (command == '3')
	{
		if (playPosition())
		{
			writeString("Posicion EEPROM reproducida");
			writeNewLine();
		}
		else
		{
			writeString("No hay posicion guardada");
			writeNewLine();
		}
	}
	else
	{
		writeString("Comando no valido");
		writeNewLine();
		sendMenuUART();
	}
}

void checkButtons(void)
{
	static uint8_t lastSaveState = 1;
	static uint8_t lastPlayState = 1;
	uint8_t saveState = 0;
	uint8_t playState = 0;
	uint32_t currentTime = 0;
	
	currentTime = getTicks1ms();
	
	if ((uint32_t)(currentTime - lastButtonTime) < BUTTON_TIME_MS)
	{
		return;
	}
	
	saveState = (PINB & (1 << BTN_SAVE_PIN)) ? 1 : 0;
	playState = (PINB & (1 << BTN_PLAY_PIN)) ? 1 : 0;
	
	// Guardar posicion con D11
	if ((lastSaveState == 1) && (saveState == 0))
	{
		lastButtonTime = currentTime;
		savePosition();
		writeString("D11: posicion guardada en EEPROM");
		writeNewLine();
	}
	
	// Reproducir posicion con D12
	if ((lastPlayState == 1) && (playState == 0))
	{
		lastButtonTime = currentTime;
		
		if (playPosition())
		{
			writeString("D12: posicion EEPROM reproducida");
			writeNewLine();
		}
		else
		{
			writeString("D12: no hay posicion guardada");
			writeNewLine();
		}
	}
	
	lastSaveState = saveState;
	lastPlayState = playState;
}

void savePosition(void)
{
	// Guardar posicion actual en EEPROM
	updateEEPROM(EEPROM_VALID, EEPROM_MARKER);
	updateEEPROM(EEPROM_SERVO_D3, Servo_getAngle(&servoD9));
	updateEEPROM(EEPROM_SERVO_D5, Servo_getAngle(&servoD6));
	updateEEPROM(EEPROM_SERVO_D6, Servo_getAngle(&servoD5));
	updateEEPROM(EEPROM_SERVO_D9, Servo_getAngle(&servoD3));
	
	PORTD |= (1 << LED_PIN);
}

uint8_t playPosition(void)
{
	// Verificar si ya hay una posicion guardada
	if (readEEPROM(EEPROM_VALID) != EEPROM_MARKER)
	{
		return 0;
	}
	
	manualMode = 0;
	
	Servo_writeAngle(&servoD9, readEEPROM(EEPROM_SERVO_D3));
	Servo_writeAngle(&servoD6, readEEPROM(EEPROM_SERVO_D5));
	Servo_writeAngle(&servoD5, readEEPROM(EEPROM_SERVO_D6));
	Servo_writeAngle(&servoD3, readEEPROM(EEPROM_SERVO_D9));
	
	PORTD &= ~(1 << LED_PIN);
	
	return 1;
}

void setManualMode(void)
{
	// Regresar al control con potenciometros
	manualMode = 1;
	PORTD |= (1 << LED_PIN);
}

void updatePotentiometers(void)
{
	// Leer entradas analogicas
	adcPotA0 = readADC(0);
	adcPotA1 = readADC(1);
	adcPotA2 = readADC(2);
	adcPotA3 = readADC(3);
}

void updateManualServos(void)
{
	// Aplicar mapeo fisico de potenciometros a servos
	Servo_writeADC(&servoD9, adcPotA0);
	Servo_writeADC(&servoD6, adcPotA1);
	Servo_writeADC(&servoD5, adcPotA2);
	Servo_writeADC(&servoD3, adcPotA3);
}

void sendPotTelemetry(void)
{
	// Mandar datos para el dashboard
	writeString("P:");
	writeNumber(adcPotA0);
	writeChar(',');
	writeNumber(adcPotA1);
	writeChar(',');
	writeNumber(adcPotA2);
	writeChar(',');
	writeNumber(adcPotA3);
	writeNewLine();
}

uint32_t getTicks1ms(void)
{
	uint8_t sreg = 0;
	uint32_t currentTicks = 0;
	
	sreg = SREG;
	cli();
	
	currentTicks = ticks1ms;
	
	SREG = sreg;
	
	return currentTicks;
}

/****************************************/
// Interrupciones

ISR(TIMER0_COMPA_vect)
{
	ticks1ms++;
}
