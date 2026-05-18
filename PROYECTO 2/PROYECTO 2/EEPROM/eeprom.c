/*
 * EEPROM.c
 *
 * Created:
 * Author:
 * Description: Libreria general para uso de EEPROM en ATmega328P
 */

/****************************************/
// Libraries
#include "EEPROM.h"
#include <avr/interrupt.h>

/****************************************/
// NON-Interrupt subroutines

void initEEPROM(void)
{
	/*
		No se necesita configuracion especial para usar EEPROM.
		Se asegura que la interrupcion de EEPROM quede deshabilitada.
	*/
	EECR &= ~(1 << EERIE);
}

uint8_t readEEPROM(uint16_t direccion)
{
	/*
		Esperar si hay una escritura en proceso.
		Esto no es un delay, solo espera a que el hardware este listo.
	*/
	while (EECR & (1 << EEPE));

	/*
		Cargar direccion.
	*/
	EEAR = direccion;

	/*
		Iniciar lectura.
	*/
	EECR |= (1 << EERE);

	/*
		Retornar dato leido.
	*/
	return EEDR;
}

void writeEEPROM(uint16_t direccion, uint8_t dato)
{
	uint8_t estadoSREG;

	/*
		Esperar si hay una escritura en proceso.
	*/
	while (EECR & (1 << EEPE));

	/*
		Cargar direccion y dato.
	*/
	EEAR = direccion;
	EEDR = dato;

	/*
		Modo erase and write.
	*/
	EECR &= ~((1 << EEPM1) | (1 << EEPM0));

	/*
		La secuencia EEMPE -> EEPE debe hacerse sin interrupciones
		durante pocos ciclos para que la escritura sea valida.

		Esta libreria NO contiene ISR y NO contiene delays.
	*/
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

	/*
		Solo escribir si el dato cambio.
		Esto ayuda a cuidar la vida util de la EEPROM.
	*/
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