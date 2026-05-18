/*
 * PWM.h
 *
 * Created:
 * Author:
 * Description:
 * Libreria para controlar servomotores por software
 * en D3, D5, D6 y D9 usando Timer1.
 */

#ifndef PWM_H_
#define PWM_H_

/****************************************/
// Librerias
#include <avr/io.h>
#include <stdint.h>

/****************************************/
// Prototipos

typedef enum
{
	SERVO_D3 = 0,
	SERVO_D5,
	SERVO_D6,
	SERVO_D9
} servo_channel_t;

typedef struct
{
	volatile uint8_t *ddr;
	volatile uint8_t *port;
	uint8_t pin;
	volatile unsigned int pulseTicks;
} servo_t;

void Servo_init(void);
void Servo_attach(servo_t *servo, servo_channel_t channel);

void Servo_writeMicroseconds(servo_t *servo, unsigned int pulseMicroseconds);
void Servo_writeADC(servo_t *servo, unsigned int adcValue);
void Servo_writeAngle(servo_t *servo, unsigned char angle);

unsigned int Servo_mapADC(unsigned int adcValue);
unsigned int Servo_mapAngle(unsigned char angle);

unsigned char Servo_getAngle(servo_t *servo);

#endif /* PWM_H_ */
