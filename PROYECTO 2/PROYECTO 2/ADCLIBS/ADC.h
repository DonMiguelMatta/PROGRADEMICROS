/*
 * ADC.h
 *
 * Created:
 * Author:
 * Description: Libreria general para el uso del ADC en ATmega328P
 */

#ifndef ADC_H_
#define ADC_H_

/****************************************/
// Libraries
#include <avr/io.h>
#include <stdint.h>

/****************************************/
// Function prototypes
void initADC(void);
uint16_t readADC(uint8_t canal);
uint8_t readADC_8bits(uint8_t canal);

#endif /* ADC_H_ */