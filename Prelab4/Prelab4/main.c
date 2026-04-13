/*
 * ADC_Contador_Hex.c
 *
 * Author: Miguel Donis
 *
 *
 */

/****************************************/
// Librerías
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

/****************************************/
// Constantes
#define T1Value 0xFF06

/****************************************/
// Prototipos
void setup(void);
void initADC(void);
void initTMR1(void);
void write7Segments(uint8_t value);
void updateButtons(void);
void updateAlarm(void);

/****************************************/
// Variables globales
volatile uint8_t contador       = 0;
volatile uint8_t adcValue       = 0;
volatile uint8_t displayMS      = 0;
volatile uint8_t displayLS      = 0;
volatile uint8_t currentDisplay = 0;

volatile uint8_t incLock = 0;
volatile uint8_t decLock = 0;

/*
 * Tabla 7 segmentos
 */
const uint8_t hexTable[16] =
{
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F, // 9
    0x77, // A
    0x7C, // b
    0x39, // C
    0x5E, // d
    0x79, // E
    0x71  // F
};

/****************************************/
// Main
int main(void)
{
    cli();
    setup();
    initADC();
    initTMR1();

    ADCSRA |= (1 << ADSC) | (1 << ADIE);   // iniciar conversión + IRQ ADC
    TIMSK1 |= (1 << TOIE1);                 // IRQ Timer1 overflow
    sei();

    while(1) {}
}

/****************************************/
// Subrutinas

void setup(void)
{
    DDRD  = 0xFF;
    PORTD = 0x00;
    DDRC  = 0x3F;
    PORTC = 0x00;
    DDRB  = 0x27;
    PORTB = 0x18;
}

void initADC(void)
{
    ADMUX  = 0;
    ADCSRA = 0;

    // AVcc como referencia, left-adjust, canal ADC7
    ADMUX  = (1 << REFS0) | (1 << ADLAR) |
             (1 << MUX2)  | (1 << MUX1)  | (1 << MUX0);

    // Habilitar ADC, prescaler 128
    ADCSRA = (1 << ADEN) |
             (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

void initTMR1(void)
{
    TCCR1A = 0x00;
    TCCR1B = 0x00;
    TCCR1B |= (1 << CS11) | (1 << CS10);   // prescaler 64
    TCNT1   = T1Value;
}

void write7Segments(uint8_t value)
{
    uint8_t pattern = hexTable[value & 0x0F];

    // Segmentos a-f en PC0-PC5 (respetar PC6 = /RESET)
    PORTC = (PORTC & 0xC0) | (pattern & 0x3F);

    // Segmento g en PB2 (respetar otros bits de PORTB)
    if (pattern & (1 << 6))
        PORTB |=  (1 << PB2);
    else
        PORTB &= ~(1 << PB2);
}

void updateButtons(void)
{
    /* --- Botón AUMENTAR: D12 / PB4 (activo bajo) --- */
    if (!(PINB & (1 << PB4)))
    {
        if (incLock == 0)
        {
            contador++;
            PORTD   = contador;
            incLock = 1;
        }
    }
    else
    {
        incLock = 0;
    }

    /* --- Botón DISMINUIR: D11 / PB3 (activo bajo) --- */
    if (!(PINB & (1 << PB3)))
    {
        if (decLock == 0)
        {
            contador--;
            PORTD   = contador;
            decLock = 1;
        }
    }
    else
    {
        decLock = 0;
    }
}

void updateAlarm(void)
{
    /*
     * Si el valor del potenciómetro (ADC, 0-255)
     * es mayor que el valor del contador (0-255)
     * se enciende el LED integrado.
     */
    if (adcValue > contador)
        PORTB |=  (1 << PB5);   // alarma ON  -> D13 encendido
    else
        PORTB &= ~(1 << PB5);   // alarma OFF -> D13 apagado
}

/****************************************/
// Interrupciones

ISR(ADC_vect)
{
    // Leer 8 bits más significativos
    adcValue  = ADCH;
    displayMS = (adcValue >> 4) & 0x0F;     // nibble alto
    displayLS =  adcValue       & 0x0F;     // nibble bajo

    // Iniciar siguiente conversión
    ADCSRA |= (1 << ADSC);
}

ISR(TIMER1_OVF_vect)
{
    TCNT1 = T1Value;

    // Apagar ambos displays antes de cambiar patrón 
    PORTB &= ~((1 << PB0) | (1 << PB1));

    // Multiplexado de displays
    if (currentDisplay == 0)
    {
        write7Segments(displayMS);
        PORTB        |= (1 << PB0);     // encender display MS
        currentDisplay = 1;
    }
    else
    {
        write7Segments(displayLS);
        PORTB        |= (1 << PB1);     // encender display LS
        currentDisplay = 0;
    }

    updateButtons();    // leer botones -> actualizar contador
    updateAlarm();      // comparar ADC vs contador -> LED alarma
}