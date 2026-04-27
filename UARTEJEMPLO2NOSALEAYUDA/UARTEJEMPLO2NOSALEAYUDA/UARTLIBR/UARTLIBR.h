/*
 * UART.h
 *
 * Created: 22/04/2026 04:13:27 p. m.
 *  Author: migue
 */ 




#ifndef UART_H_
#define UART_H_

#include <avr/io.h>
#include "UARTLIBR/UARTLIBR.c"
void initUART();
void writeChar(char caracter);
void writeString(char* string);


#endif /* UART_H_ */
