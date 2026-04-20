/*
 * ControlPWM.h
 *
 * Created:
 * Author:
 * Description:
 */

#ifndef CONTROLPWM_H_
#define CONTROLPWM_H_

/****************************************/
// Encabezado (Libraries)
#include <avr/io.h>

/****************************************/
// Function prototypes
typedef enum
{
	SERVO_OC1A = 0,
	SERVO_OC1B
} servo_channel_t;

typedef struct
{
	servo_channel_t channel;
} servo_t;

typedef struct
{
	volatile uint8_t *ddr;
	volatile uint8_t *port;
	uint8_t pin;
	volatile uint8_t duty;
} softpwm_t;

void ADC_init(void);
unsigned int ADC_read(unsigned char channel);

void Servo_init(void);
void Servo_attach(servo_t *servo, servo_channel_t channel);
void Servo_writeCounts(servo_t *servo, unsigned int value);
void Servo_writeADC(servo_t *servo, unsigned int adcValue);
unsigned int Servo_mapADC(unsigned int adcValue);

void SoftPWM_init(void);
void SoftPWM_attach(softpwm_t *led, volatile uint8_t *ddr, volatile uint8_t *port, uint8_t pin);
void SoftPWM_write(softpwm_t *led, unsigned char dutyValue);
void SoftPWM_writeADC(softpwm_t *led, unsigned int adcValue);
unsigned char SoftPWM_mapADC(unsigned int adcValue);

#endif /* CONTROLPWM_H_ */