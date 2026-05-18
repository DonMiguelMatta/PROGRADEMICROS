/*
 * EEPROM.h
 *
 * Created:
 * Author:
 * Description: Libreria general para uso de EEPROM en ATmega328P
 */

#ifndef EEPROM_H_
#define EEPROM_H_

/****************************************/
// Librerias
#include <avr/io.h>
#include <stdint.h>

/****************************************/
// Prototipos

void initEEPROM(void);

uint8_t readEEPROM(uint16_t direccion);
void writeEEPROM(uint16_t direccion, uint8_t dato);
void updateEEPROM(uint16_t direccion, uint8_t dato);

void readEEPROM_Block(uint16_t direccionInicial, uint8_t* datos, uint8_t cantidad);
void writeEEPROM_Block(uint16_t direccionInicial, uint8_t* datos, uint8_t cantidad);

#endif /* EEPROM_H_ */
