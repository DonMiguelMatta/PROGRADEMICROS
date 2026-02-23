;*****************************************************************************
; Universidad del Valle de Guatemala
; Programación de Microcontroladores
; Archivo: Contador_Timer0_7Seg
; Hardware: ATMEGA328P
; Autor: Miguel Angel Donis
; Carnet: 22993
;
; Descripción:
; - Contador manual 4 bits en LEDs (PC0–PC3)
; - Botones en PC4 (incremento) y PC5 (decremento)
; - Contador hexadecimal automático con Timer0
; - Interrupción cada ~10 ms
; - Cambio cada ~1000 ms
; - Display 7 segmentos en PD0–PD6
;*****************************************************************************

.include "M328PDEF.inc"          ; Incluir definiciones del ATmega328P

;=============================
; DEFINICIÓN DE REGISTROS
;=============================
.def regManual = R21             ; Guarda contador manual (0–15)
.def regHex    = R22             ; Guarda contador hexadecimal (0–F)
.def tempReg   = R16             ; Registro temporal
.def auxReg    = R17             ; Registro auxiliar
.def tickCount = R19             ; Cuenta interrupciones de 10 ms

.cseg

;=============================
; VECTORES DE INTERRUPCIÓN
;=============================
.org 0x0000
    rjmp INIT                    ; Vector Reset ? salto a inicialización

.org 0x0008
    rjmp ISR_PCINT1              ; Vector Pin Change PORTC

.org 0x0020
    rjmp ISR_TIMER0_OVF          ; Vector Timer0 Overflow


;=============================
; TABLA DISPLAY 7 SEGMENTOS
; Cátodo común 0–F
;=============================
Tabla7seg:
.DB 0x3F,0x06,0x5B,0x4F          ; 0,1,2,3
.DB 0x66,0x6D,0x7D,0x07          ; 4,5,6,7
.DB 0x7F,0x6F,0x77,0x7C          ; 8,9,A,B
.DB 0x39,0x5E,0x79,0x71          ; C,D,E,F


;*****************************************************************************
; INICIALIZACIÓN
;*****************************************************************************
INIT:

    clr r1                       ; Mantener R1 en 0 (necesario para ADC con acarreo)

    ldi tempReg, LOW(RAMEND)     ; Cargar parte baja del final de RAM
    out SPL, tempReg             ; Configurar Stack Pointer bajo

    ldi tempReg, HIGH(RAMEND)    ; Cargar parte alta del final de RAM
    out SPH, tempReg             ; Configurar Stack Pointer alto

    ldi tempReg, 0b00001111      ; PC0–PC3 como salidas
    out DDRC, tempReg            ; Escribir configuración en DDRC

    ldi tempReg, (1<<PC4)|(1<<PC5) ; Activar pull-up en PC4 y PC5
    out PORTC, tempReg           ; Escribir en PORTC

    ldi tempReg, 0b01111111      ; PD0–PD6 como salida
    out DDRD, tempReg            ; Configurar puerto D

    ldi tempReg, (1<<PCINT12)|(1<<PCINT13) ; Habilitar PC4 y PC5
    sts PCMSK1, tempReg          ; Guardar en máscara PCINT1

    ldi tempReg, (1<<PCIE1)      ; Habilitar grupo PORTC
    sts PCICR, tempReg           ; Guardar en registro control

    ldi tempReg, 100             ; Precarga del Timer0
    out TCNT0, tempReg           ; Inicializar contador Timer0

    ldi tempReg, (1<<CS02)|(1<<CS00) ; Prescaler 1024
    out TCCR0B, tempReg          ; Configurar Timer0

    ldi tempReg, (1<<TOIE0)      ; Habilitar interrupción overflow
    sts TIMSK0, tempReg          ; Guardar en máscara Timer0

    ldi regManual, 0             ; Inicializar contador manual
    ldi regHex, 0                ; Inicializar contador hexadecimal
    ldi tickCount, 0             ; Inicializar contador de ticks

    ldi ZH, HIGH(Tabla7seg<<1)   ; Cargar parte alta de dirección tabla
    ldi ZL, LOW(Tabla7seg<<1)    ; Cargar parte baja
    lpm auxReg, Z                ; Leer primer valor (0)
    out PORTD, auxReg            ; Mostrar 0 en display

    sei                          ; Habilitar interrupciones globales


;*****************************************************************************
; LOOP PRINCIPAL
;*****************************************************************************
MAIN_LOOP:

    in auxReg, PORTC             ; Leer estado actual de PORTC
    andi auxReg, 0b11110000      ; Limpiar bits PC0–PC3
    or auxReg, regManual         ; Insertar valor contador manual
    out PORTC, auxReg            ; Actualizar LEDs

    rjmp MAIN_LOOP               ; Repetir indefinidamente


;*****************************************************************************
; ISR BOTONES (PCINT1)
;*****************************************************************************
ISR_PCINT1:

    push tempReg                 ; Guardar tempReg en pila
    in tempReg, SREG             ; Leer registro de estado
    push tempReg                 ; Guardar SREG

    in tempReg, PINC             ; Leer estado actual de PORTC

    sbrc tempReg, PC4            ; Si PC4=1 (no presionado)
    rjmp CHECK_DEC               ; Saltar a verificar PC5

    inc regManual                ; Incrementar contador manual
    andi regManual, 0x0F         ; Limitar a 4 bits
    rjmp END_PCINT               ; Ir a salida

CHECK_DEC:

    sbrc tempReg, PC5            ; Si PC5=1
    rjmp END_PCINT               ; No hacer nada

    dec regManual                ; Decrementar contador manual
    andi regManual, 0x0F         ; Limitar a 4 bits

END_PCINT:

    ldi tempReg, (1<<PCIF1)      ; Preparar limpieza bandera
    sts PCIFR, tempReg           ; Limpiar bandera PCINT1

    pop tempReg                  ; Recuperar SREG guardado
    out SREG, tempReg            ; Restaurar SREG
    pop tempReg                  ; Recuperar tempReg
    reti                         ; Retornar de interrupción


;*****************************************************************************
; ISR TIMER0 OVERFLOW
;*****************************************************************************
ISR_TIMER0_OVF:

    push tempReg                 ; Guardar tempReg
    push auxReg                  ; Guardar auxReg
    in tempReg, SREG             ; Leer SREG
    push tempReg                 ; Guardar SREG

    ldi tempReg, 100             ; Recargar valor inicial
    out TCNT0, tempReg           ; Reiniciar conteo Timer0

    inc tickCount                ; Incrementar contador de ticks
    cpi tickCount, 100           ; ¿Ya pasó ~1 segundo?
    brne END_TIMER               ; Si no, salir

    ldi tickCount, 0             ; Reiniciar contador de ticks

    inc regHex                   ; Incrementar contador hexadecimal
    andi regHex, 0x0F            ; Limitar 0–F

    ldi ZH, HIGH(Tabla7seg<<1)   ; Dirección alta tabla
    ldi ZL, LOW(Tabla7seg<<1)    ; Dirección baja tabla
    add ZL, regHex               ; Desplazarse según valor
    adc ZH, r1                   ; Ajustar acarreo
    lpm auxReg, Z                ; Leer valor tabla

    out PORTD, auxReg            ; Mostrar en display

END_TIMER:

    pop tempReg                  ; Recuperar SREG
    out SREG, tempReg            ; Restaurar SREG
    pop auxReg                   ; Recuperar auxReg
    pop tempReg                  ; Recuperar tempReg
    reti                         ; Retornar