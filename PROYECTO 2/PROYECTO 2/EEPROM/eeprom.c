/*
 * EEPROM.c
 *
 * Created:
 * Author:
 * Description: Libreria general para uso de EEPROM en ATmega328P
 */

/****************************************/
// Librerias
#include "EEPROM.h"
#include <avr/interrupt.h>

/****************************************/
// Funciones

void initEEPROM(void)
{
	// Dejar EEPROM sin interrupciones
	EECR &= ~(1 << EERIE);
}

uint8_t readEEPROM(uint16_t direccion)
{
	// Esperar si hay una escritura pendiente
	while (EECR & (1 << EEPE));

	// Cargar direccion de lectura
	EEAR = direccion;

	// Leer dato guardado
	EECR |= (1 << EERE);

	return EEDR;
}

void writeEEPROM(uint16_t direccion, uint8_t dato)
{
	uint8_t estadoSREG;

	// Esperar si la EEPROM sigue ocupada
	while (EECR & (1 << EEPE));

	// Preparar direccion y dato
	EEAR = direccion;
	EEDR = dato;

	// Usar modo borrar y escribir
	EECR &= ~((1 << EEPM1) | (1 << EEPM0));

	// Proteger la secuencia obligatoria de escritura
	estadoSREG = SREG;
	cli();

	EECR |= (1 << EEMPE);
	EECR |= (1 << EEPE);

	SREG = estadoSREG;
}

void updateEEPROM(uint16_t direccion, uint8_t dato)
{
	uint8_t datoActual;

	datoActual = readEEPROM(direccion);

	// Escribir solo si el dato cambio
	if (datoActual != dato)
	{
		writeEEPROM(direccion, dato);
	}
}

void readEEPROM_Block(uint16_t direccionInicial, uint8_t* datos, uint8_t cantidad)
{
	uint8_t i;

	for (i = 0; i < cantidad; i++)
	{
		datos[i] = readEEPROM(direccionInicial + i);
	}
}

void writeEEPROM_Block(uint16_t direccionInicial, uint8_t* datos, uint8_t cantidad)
{
	uint8_t i;

	for (i = 0; i < cantidad; i++)
	{
		updateEEPROM(direccionInicial + i, datos[i]);
	}
}
