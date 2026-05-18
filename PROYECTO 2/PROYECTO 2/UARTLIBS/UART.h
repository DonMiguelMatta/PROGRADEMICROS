/*
 * UART.h
 *
 * Created:
 * Author:
 * Description:
 * Libreria para comunicacion USART/UART en ATmega328P.
 */

#ifndef UART_H_
#define UART_H_

/****************************************/
// Librerias

#include <avr/io.h>
#include <stdint.h>

/****************************************/
// Prototipos

void USART_Init(void);
void USART_Transmit(char data);
void USART_SendString(const char *text);
char USART_Receive(void);
uint8_t USART_Available(void);
uint8_t USART_ReadIfAvailable(char *data);
void USART_NewLine(void);
void USART_SendNumber(uint16_t numero);

/****************************************/
// Alias para usar nombres cortos

#define initUART            USART_Init
#define writeChar           USART_Transmit
#define writeString         USART_SendString
#define readChar            USART_Receive
#define availableUART       USART_Available
#define readCharIfAvailable USART_ReadIfAvailable
#define writeNewLine        USART_NewLine
#define writeNumber         USART_SendNumber

#endif /* UART_H_ */
