/*
 * IncFile1.h
 *
 * Created: 08/04/2026 05:16:25 p. m.
 *  Author: migue
 */ 


#ifndef INCFILE1_H_
#define INCFILE1_H_

#include <avr/io.h>
#define no_invertido 0
#define invertido 1
#define fastPWM 0 
#define phasePWM 1



void initPWM0(unit8_t invert, unit8_t mode, uint16_t);
void updateDutyCicle0A(uint8_t duty);
void updateDutyCicle0B(uint8_t duty);


#endif /* INCFILE1_H_ */