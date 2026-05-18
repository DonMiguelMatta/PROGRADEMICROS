/*
 * UART.h
 *
 * Created:
 * Author:
 * Description: Libreria general para comunicacion serial UART en ATmega328P
 */

#ifndef UART_H_
#define UART_H_

/****************************************/
// Libraries
#include <avr/io.h>
#include <stdint.h>

/****************************************/
// Constants

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#define UART_BAUD_RATE 9600

/****************************************/
// Function prototypes

void initUART(void);
void initUART_Baud(uint32_t baudrate);

void writeChar(char caracter);
void writeString(char* string);

uint8_t availableUART(void);
char readChar(void);

void writeNumber(uint16_t numero);
void writeNewLine(void);

#endif /* UART_H_ */