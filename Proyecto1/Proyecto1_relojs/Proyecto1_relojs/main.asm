;*****************************************************************************
; Universidad del Valle de Guatemala
; Programacion de Microcontroladores
; Archivo: Reloj_24h_Calendario_Multiplexado_Final 
; Hardware: ATMEGA328P @ 16 MHz
; Autor: Miguel Angel Donis
; Carnet: 22993
;
; MODOS:
;   0 = Mostrar hora       | 1 = Mostrar fecha
;   2 = Editar hora        | 3 = Editar fecha
;   4 = Configurar alarma
;
;*****************************************************************************

.include "M328PDEF.inc"

;=============================================================================
; DEFINICION DE REGISTROS
;=============================================================================
.def unidades   = R22          ; Unidades de minuto del reloj (0-9)
.def decenas    = R23          ; Decenas de minuto del reloj (0-5)
.def unidadesh  = R24          ; Unidades de hora del reloj (0-9)
.def decenash   = R25          ; Decenas de hora del reloj (0-2)

.def diaU       = R20          ; Unidades del dia actual (0-9)
.def diaD       = R21          ; Decenas del dia actual (0-3)
.def mesU       = R26          ; Unidades del mes actual (0-9)
.def mesD       = R27          ; Decenas del mes actual (0-1)

.def tempReg    = R16          ; Registro temporal de uso general
.def auxReg     = R17          ; Registro auxiliar (segundo temporal)
.def tickCount  = R19          ; Contador de ticks de 1 s (0-59)
.def ledmedio   = R18          ; Estado del LED central: alterna 0/1 cada segundo
.def modo       = R28          ; Modo activo del sistema (0-4)

.def disp0      = R10          ; Digito a mostrar en display PB0 (unidades derecha)
.def disp1      = R11          ; Digito a mostrar en display PB1
.def disp2      = R12          ; Digito a mostrar en display PB2
.def disp3      = R13          ; Digito a mostrar en display PB3 (decenas izquierda)

;=============================================================================
; VARIABLES EN SRAM
; Datos que necesitan persistir entre subrutinas o que no caben en registros.
;=============================================================================
.dseg
anio4:          .byte 1        ; Contador ciclico 0-3 para detectar anio bisiesto
btnCnt0:        .byte 1        ; Ticks consecutivos que PC0 lleva presionado
btnCnt1:        .byte 1        ; igual que btnCnt0 para PC1
btnCnt2:        .byte 1        ; igual que btnCnt0 para PC2
btnCnt3:        .byte 1        ; igual que btnCnt0 para PC3
btnCnt4:        .byte 1        ; igual que btnCnt0 para PC4
btnCnt5:        .byte 1        ; igual que btnCnt0 para PC5
btnLock0:       .byte 1        ; Bandera: 1 = accion ya ejecutada, esperar soltar PC0
btnLock1:       .byte 1        ; igual que btnLock0 para PC1
btnLock2:       .byte 1        ; igual que btnLock0 para PC2
btnLock3:       .byte 1        ; igual que btnLock0 para PC3
btnLock4:       .byte 1        ; igual que btnLock0 para PC4
btnLock5:       .byte 1        ; igual que btnLock0 para PC5
blinkCnt:       .byte 1        ; Reservado para uso futuro de parpadeo
alrm_hora_u:    .byte 1        ; Unidades de la hora programada en alarma (0-9)
alrm_hora_d:    .byte 1        ; Decenas de la hora programada en alarma (0-2)
alrm_min_u:     .byte 1        ; Unidades del minuto programado en alarma (0-9)
alrm_min_d:     .byte 1        ; Decenas del minuto programado en alarma (0-5)
alarma_activa:  .byte 1        ; Estado de la alarma: 0=editando 1=armada 2=sonando
tono_cnt:       .byte 1        ; Posicion dentro del ciclo ON/OFF del tono (0-49)

.cseg

;=============================================================================
; TABLA DE VECTORES DE INTERRUPCION
; Cada .org apunta a la direccion fija del vector; se coloca un salto a la ISR.
; Los vectores no usados quedan vacios (NOP implicito del hardware).
;=============================================================================
.org 0x0000
    rjmp INIT                  ; Vector de reset -> saltar a inicializacion

.org 0x0016
    rjmp ISR_TIMER1_COMPA      ; Vector Timer1 Compare A -> base de 1 segundo

.org 0x001C
    rjmp ISR_TIMER0_COMPA      ; Vector Timer0 Compare A -> tono alarma 500 Hz

; Fin de la tabla de vectores del ATmega328P (52 words = 0x0034)
.org 0x0034

;=============================================================================
; TABLA DE PATRONES 7 SEGMENTOS
; anodo comun
;=============================================================================
Tabla7seg:
.DB 0x3F,0x06,0x5B,0x4F        ; Patrones para 0, 1, 2, 3
.DB 0x66,0x6D,0x7D,0x07        ; Patrones para 4, 5, 6, 7
.DB 0x7F,0x6F                  ; Patrones para 8, 9


;=============================================================================
; BLOQUE 1: INICIALIZACION
; 
;=============================================================================
INIT:
    clr r1                         ; R1 = 0, se usa como acarreo cero en LPM

    ldi tempReg, LOW(RAMEND)       ; Apuntar Stack Pointer al tope de SRAM
    out SPL, tempReg
    ldi tempReg, HIGH(RAMEND)
    out SPH, tempReg

    ldi tempReg, 0b11111111        ; Todo PORTD como salida (segmentos + LED PD7)
    out DDRD, tempReg

    ldi tempReg, (1<<PB0)|(1<<PB1)|(1<<PB2)|(1<<PB3)|(1<<PB4)
    out DDRB, tempReg              ; PB0-PB3 transistores displays, PB4 buzzer

    ldi tempReg, 0x00
    out DDRC, tempReg              ; Todo PORTC como entrada (botones)
    ldi tempReg, (1<<PC0)|(1<<PC1)|(1<<PC2)|(1<<PC3)|(1<<PC4)|(1<<PC5)
    out PORTC, tempReg             ; Activar pull-ups internos en PC0-PC5

    cbi PORTB, PB0                 ; Apagar todos los transistores de display
    cbi PORTB, PB1
    cbi PORTB, PB2
    cbi PORTB, PB3
    cbi PORTB, PB4                 ; Buzzer apagado al inicio
    cbi PORTD, PD7                 ; LED central apagado al inicio

    ;-------------------------------------------------------------------------
    ; TIMER1 - CTC - genera interrupcion cada 1 segundo
    ; f = 16,000,000 / (256 * 62500) = 1 Hz  |  Prescaler=256, OCR1A=62499
    ;-------------------------------------------------------------------------
    ldi tempReg, 0x00
    sts TCCR1A, tempReg            ; Modo CTC (WGM13:0 = 0100), sin salida HW

    ldi tempReg, HIGH(62499)       ; Cargar parte alta del periodo
    sts OCR1AH, tempReg
    ldi tempReg, LOW(62499)        ; Cargar parte baja del periodo
    sts OCR1AL, tempReg

    ldi tempReg, 0x00
    sts TCNT1H, tempReg            ; Reiniciar contador Timer1
    sts TCNT1L, tempReg

    ldi tempReg, (1<<WGM12)|(1<<CS12) ; CTC habilitado, prescaler 256
    sts TCCR1B, tempReg

    ldi tempReg, (1<<OCIE1A)
    sts TIMSK1, tempReg            ; Habilitar interrupcion por comparacion A

    ;-------------------------------------------------------------------------
    ; TIMER0 - CTC - ISR cada 1 ms 
    ;-------------------------------------------------------------------------
    ldi tempReg, (1<<WGM01)
    out TCCR0A, tempReg            ; Modo CTC, sin salida hardware en OC0A

    ldi tempReg, 0x00
    out TCCR0B, tempReg            ; Sin clock: timer detenido por ahora

    ldi tempReg, 249
    out OCR0A, tempReg             ; Periodo: 249+1 = 250 ciclos a prescaler 64

    ldi tempReg, 0x00
    out TCNT0, tempReg             ; Reiniciar contador Timer0

    ldi tempReg, (1<<OCIE0A)
    sts TIMSK0, tempReg            ; Habilitar interrupcion por comparacion A

    ; Valores iniciales del reloj: 00:00:00
    ldi unidades, 0
    ldi decenas, 0
    ldi unidadesh, 0
    ldi decenash, 0

    ; Fecha inicial: 01/01
    ldi diaU, 1
    ldi diaD, 0
    ldi mesU, 1
    ldi mesD, 0

    ldi tickCount, 0               ; Ningun segundo transcurrido
    ldi ledmedio, 1                ; LED central encendido al arrancar
    ldi modo, 0                    ; Arrancar en modo mostrar hora

    ; Poner en cero todas las variables SRAM
    ldi tempReg, 0
    sts anio4, tempReg             ; Primer anio del ciclo bisiesto
    sts btnCnt0, tempReg           ; Contadores antirrebote en 0
    sts btnCnt1, tempReg
    sts btnCnt2, tempReg
    sts btnCnt3, tempReg
    sts btnCnt4, tempReg
    sts btnCnt5, tempReg
    sts btnLock0, tempReg          ; Bloqueos en 0 (ningun boton bloqueado)
    sts btnLock1, tempReg
    sts btnLock2, tempReg
    sts btnLock3, tempReg
    sts btnLock4, tempReg
    sts btnLock5, tempReg
    sts alrm_hora_u, tempReg       ; Alarma inicializada en 00:00
    sts alrm_hora_d, tempReg
    sts alrm_min_u, tempReg
    sts alrm_min_d, tempReg
    sts alarma_activa, tempReg     ; Alarma en modo edicion
    sts tono_cnt, tempReg          ; Ciclo de tono desde cero

    sei                            ; Habilitar interrupciones globales


;=============================================================================
; BLOQUE 2: LOOP PRINCIPAL
;   1. Lee los botones de modo y OK
;   2. Si el modo es de edicion (>=2), lee los botones de ajuste
;   3. Carga los digitos correspondientes al modo activo
;   4. Decide si parpadear o mostrar fijo
;   5. Multiplexa los 4 displays uno por uno
;=============================================================================
MAIN_LOOP:
    rcall CHECK_BTN_PC4_MODE       ; Leer boton PC4: cambia de modo
    rcall CHECK_BTN_PC5_OK         ; Leer boton PC5: confirmar/armar

    cpi modo, 2
    brlo NO_EDIT_BUTTONS           ; Modos 0 y 1 no tienen botones de edicion
    rcall CHECK_EDIT_BUTTONS       ; Modos 2, 3, 4: leer PC0-PC3

NO_EDIT_BUTTONS:
    ; Seleccionar que datos cargar en disp0-disp3 segun el modo
    cpi modo, 0
    breq CARGAR_RELOJ              ; Modo 0: mostrar HH:MM del reloj
    cpi modo, 1
    breq CARGAR_FECHA              ; Modo 1: mostrar DD/MM
    cpi modo, 2
    breq CARGAR_RELOJ              ; Modo 2: editar hora (mismos datos)
    cpi modo, 3
    breq CARGAR_FECHA              ; Modo 3: editar fecha (mismos datos)
    rjmp CARGAR_ALARMA             ; Modo 4: mostrar HH:MM de la alarma


; Carga hora actual en disp3-disp0 (formato HH:MM)
CARGAR_RELOJ:
    mov disp0, unidades            ; PB0 <- unidades de minuto
    mov disp1, decenas             ; PB1 <- decenas de minuto
    mov disp2, unidadesh           ; PB2 <- unidades de hora
    mov disp3, decenash            ; PB3 <- decenas de hora
    rjmp CHECK_BLINK

; Carga fecha actual en disp3-disp0 (formato DD/MM)
CARGAR_FECHA:
    mov disp0, mesU                ; PB0 <- unidades de mes
    mov disp1, mesD                ; PB1 <- decenas de mes
    mov disp2, diaU                ; PB2 <- unidades de dia
    mov disp3, diaD                ; PB3 <- decenas de dia
    rjmp CHECK_BLINK

; Carga hora de la alarma desde SRAM en disp3-disp0 (formato HH:MM)
CARGAR_ALARMA:
    lds tempReg, alrm_min_u
    mov disp0, tempReg             ; PB0 <- unidades de minuto alarma
    lds tempReg, alrm_min_d
    mov disp1, tempReg             ; PB1 <- decenas de minuto alarma
    lds tempReg, alrm_hora_u
    mov disp2, tempReg             ; PB2 <- unidades de hora alarma
    lds tempReg, alrm_hora_d
    mov disp3, tempReg             ; PB3 <- decenas de hora alarma

; Decide si los displays deben parpadear o mostrarse fijos.
; Modos 0-1: siempre fijos. Modos 2-3: parpadean con ledmedio.
; Modo 4: parpadea solo mientras se edita (alarma_activa=0).
CHECK_BLINK:
    cpi modo, 2
    brlo MOSTRAR_DISPLAYS          ; Modos 0 y 1: siempre mostrar

    cpi modo, 4
    brne CHECK_BLINK_NORMAL        ; Modos 2-3: ir al chequeo normal
    lds tempReg, alarma_activa
    tst tempReg
    brne MOSTRAR_DISPLAYS          ; Armada o sonando: mostrar fijo

CHECK_BLINK_NORMAL:
    cpi ledmedio, 0                ; ledmedio=0 -> fase apagada del parpadeo
    breq BLANK_DISPLAYS
    rjmp MOSTRAR_DISPLAYS

; Apaga todos los displays durante la fase oscura del parpadeo
BLANK_DISPLAYS:
    cbi PORTB, PB0                 ; Desactivar transistores
    cbi PORTB, PB1
    cbi PORTB, PB2
    cbi PORTB, PB3
    in tempReg, PORTD
    andi tempReg, 0b10000000       ; Conservar solo PD7 (LED central), limpiar segmentos
    out PORTD, tempReg
    rcall Delayparpadeo            ; Pequeno retardo para suavizar el parpadeo
    rjmp MAIN_LOOP


; Muestra los 4 displays de forma multiplexada.
; Para cada display: apagar todos, cargar patron desde tabla, encender display,
; esperar DelayPequeno, apagar. Los segmentos se fusionan con PD7 para no
; alterar el LED central.
MOSTRAR_DISPLAYS:

    ;--- Display PB0: digito disp0 ---
    cbi PORTB, PB0                 ; Apagar todos los transistores antes de cambiar segmentos
    cbi PORTB, PB1
    cbi PORTB, PB2
    cbi PORTB, PB3
    ldi ZH, HIGH(Tabla7seg<<1)     ; Apuntar Z a la tabla en flash (byte address)
    ldi ZL, LOW(Tabla7seg<<1)
    add ZL, disp0                  ; Offset = valor del digito a mostrar
    adc ZH, r1                     ; Propagar acarreo (R1 = 0)
    lpm auxReg, Z                  ; Leer patron de segmentos de flash
    in tempReg, PORTD
    andi tempReg, 0b10000000       ; Aislar estado actual de PD7
    or auxReg, tempReg             ; Combinar patron con PD7
    out PORTD, auxReg              ; Enviar segmentos al puerto
    sbi PORTB, PB0                 ; Activar transistor de este display
    rcall DelayPequeno             ; Mantener encendido un breve tiempo
    cbi PORTB, PB0                 ; Apagar transistor (ghosting prevention)

    ;--- Display PB1: digito disp1 ---
    ; (igual que en Display PB0, usando disp1 y PB1)
    cbi PORTB, PB0
    cbi PORTB, PB1
    cbi PORTB, PB2
    cbi PORTB, PB3
    ldi ZH, HIGH(Tabla7seg<<1)
    ldi ZL, LOW(Tabla7seg<<1)
    add ZL, disp1
    adc ZH, r1
    lpm auxReg, Z
    in tempReg, PORTD
    andi tempReg, 0b10000000
    or auxReg, tempReg
    out PORTD, auxReg
    sbi PORTB, PB1
    rcall DelayPequeno
    cbi PORTB, PB1

    ;--- Display PB2: digito disp2 ---
    ; (igual que en Display PB0, usando disp2 y PB2)
    cbi PORTB, PB0
    cbi PORTB, PB1
    cbi PORTB, PB2
    cbi PORTB, PB3
    ldi ZH, HIGH(Tabla7seg<<1)
    ldi ZL, LOW(Tabla7seg<<1)
    add ZL, disp2
    adc ZH, r1
    lpm auxReg, Z
    in tempReg, PORTD
    andi tempReg, 0b10000000
    or auxReg, tempReg
    out PORTD, auxReg
    sbi PORTB, PB2
    rcall DelayPequeno
    cbi PORTB, PB2

    ;--- Display PB3: digito disp3 ---
    ; (igual que en Display PB0, usando disp3 y PB3)
    cbi PORTB, PB0
    cbi PORTB, PB1
    cbi PORTB, PB2
    cbi PORTB, PB3
    ldi ZH, HIGH(Tabla7seg<<1)
    ldi ZL, LOW(Tabla7seg<<1)
    add ZL, disp3
    adc ZH, r1
    lpm auxReg, Z
    in tempReg, PORTD
    andi tempReg, 0b10000000
    or auxReg, tempReg
    out PORTD, auxReg
    sbi PORTB, PB3
    rcall DelayPequeno
    cbi PORTB, PB3

    rjmp MAIN_LOOP                 ; Volver al inicio del loop


;=============================================================================
; BLOQUE 3: BOTONES DE MODO (PC4) Y OK (PC5)
; Cada boton implementa antirrebote por software con contador (btnCntX) y
; bloqueo (btnLockX) para disparar la accion exactamente una vez por pulsacion.
; El patron es: contar 8 ticks estable -> ejecutar accion y bloquear ->
; al soltar, reiniciar contador y bloqueo.
;=============================================================================

; CHECK_BTN_PC4_MODE: cicla entre modos 0-4. Si la alarma esta sonando,
; la detiene antes de cambiar de modo.
CHECK_BTN_PC4_MODE:
    sbic PINC, PC4                 ; Saltar si PC4=0 (presionado)
    rjmp PC4_SUELTO                ; PC4=1: boton suelto

    lds tempReg, btnCnt4
    cpi tempReg, 8                 ; Esperar 8 lecturas estables para confirmar pulsacion
    brsh PC4_ESTABLE
    inc tempReg                    ; Incrementar contador de estabilidad
    sts btnCnt4, tempReg
    ret

PC4_ESTABLE:
    lds tempReg, btnLock4
    cpi tempReg, 1
    breq FIN_PC4                   ; Ya se ejecuto: ignorar hasta que se suelte

    ldi tempReg, 1
    sts btnLock4, tempReg          ; Bloquear para no repetir la accion

    lds tempReg, alarma_activa
    cpi tempReg, 2
    brne PC4_CAMBIO_MODO
    rcall DETENER_ALARMA           ; Silenciar alarma antes de salir del modo

PC4_CAMBIO_MODO:
    inc modo
    cpi modo, 5                    ; Modo maximo es 4; si llega a 5, volver a 0
    brne FIN_PC4
    ldi modo, 0

FIN_PC4:
    ret

PC4_SUELTO:
    ldi tempReg, 0
    sts btnCnt4, tempReg           ; Reiniciar contador al soltar
    sts btnLock4, tempReg          ; Liberar bloqueo
    ret


; CHECK_BTN_PC5_OK: confirma la edicion o arma/desarma/silencia la alarma.
; Segun el modo activo:
;   Modo 2 -> vuelve a modo 0 (guarda hora)
;   Modo 3 -> vuelve a modo 1 (guarda fecha)
;   Modo 4 -> arma (0->1), desarma (1->0) o silencia (2->0)
CHECK_BTN_PC5_OK:
    sbic PINC, PC5                 ; Saltar si PC5=0 (presionado)
    rjmp PC5_SUELTO

    lds tempReg, btnCnt5
    cpi tempReg, 8                 ; igual que en CHECK_BTN_PC4_MODE
    brsh PC5_ESTABLE
    inc tempReg
    sts btnCnt5, tempReg
    ret

PC5_ESTABLE:
    lds tempReg, btnLock5
    cpi tempReg, 1
    breq FIN_PC5
    ldi tempReg, 1
    sts btnLock5, tempReg

    cpi modo, 2                    ; Editar hora -> volver a mostrar hora
    brne CHECK_OK_FECHA
    ldi modo, 0
    rjmp FIN_PC5

CHECK_OK_FECHA:
    cpi modo, 3                    ; Editar fecha -> volver a mostrar fecha
    brne CHECK_OK_ALARMA
    ldi modo, 1
    rjmp FIN_PC5

; Logica de OK especifica para el modo alarma
CHECK_OK_ALARMA:
    cpi modo, 4
    brne FIN_PC5

    lds tempReg, alarma_activa

    cpi tempReg, 2                 ; Sonando -> silenciar
    brne CHECK_OK_ALR_ARMAR
    rcall DETENER_ALARMA
    rjmp FIN_PC5

CHECK_OK_ALR_ARMAR:
    tst tempReg
    breq ARMAR_ALARMA              ; Editando (0) -> armar (1)
    ldi tempReg, 0
    sts alarma_activa, tempReg     ; Armada (1) -> desarmar (0)
    rjmp FIN_PC5

ARMAR_ALARMA:
    ldi tempReg, 1
    sts alarma_activa, tempReg     ; Guardar hora y armar la alarma

FIN_PC5:
    ret

PC5_SUELTO:
    ldi tempReg, 0
    sts btnCnt5, tempReg           ; igual que en PC4_SUELTO
    sts btnLock5, tempReg
    ret


;=============================================================================
; DETENER_ALARMA
; Para el Timer0 (corta el tono), apaga el buzzer y resetea el estado
; de la alarma a "editando" (0). Se llama desde PC4 y PC5.
;=============================================================================
DETENER_ALARMA:
    ldi tempReg, 0
    out TCCR0B, tempReg            ; Quitar el clock de Timer0: lo detiene
    cbi PORTB, PB4                 ; Forzar buzzer apagado
    sts alarma_activa, tempReg     ; Volver a estado "editando"
    sts tono_cnt, tempReg          ; Reiniciar posicion en el ciclo ON/OFF
    ret


;=============================================================================
; BLOQUE 4: DESPACHO Y RUTINAS DE EDICION
; CHECK_EDIT_BUTTONS redirige a la rutina correcta segun el modo.
; Cada rutina EDITAR_X llama a los 4 handlers de boton correspondientes.
; La edicion de alarma queda bloqueada si alarma_activa != 0.
;=============================================================================
CHECK_EDIT_BUTTONS:
    cpi modo, 2
    breq EDITAR_HORA               ; Modo 2: ajustar hora del reloj
    cpi modo, 3
    breq EDITAR_FECHA              ; Modo 3: ajustar dia y mes
    rjmp EDITAR_ALARMA             ; Modo 4: ajustar hora de la alarma

EDITAR_HORA:
    rcall CHECK_BTN_PC2_INC_LEFT_H ; PC2 -> incrementar hora
    rcall CHECK_BTN_PC3_DEC_LEFT_H ; PC3 -> decrementar hora
    rcall CHECK_BTN_PC0_INC_RIGHT_H ; PC0 -> incrementar minuto
    rcall CHECK_BTN_PC1_DEC_RIGHT_H ; PC1 -> decrementar minuto
    ret

EDITAR_FECHA:
    rcall CHECK_BTN_PC2_INC_LEFT_F ; PC2 -> incrementar dia
    rcall CHECK_BTN_PC3_DEC_LEFT_F ; PC3 -> decrementar dia
    rcall CHECK_BTN_PC0_INC_RIGHT_F ; PC0 -> incrementar mes
    rcall CHECK_BTN_PC1_DEC_RIGHT_F ; PC1 -> decrementar mes
    ret

EDITAR_ALARMA:
    lds tempReg, alarma_activa
    tst tempReg
    brne FIN_EDITAR_ALARMA         ; Si armada o sonando, no permitir cambios
    rcall CHECK_BTN_PC2_INC_LEFT_A ; PC2 -> incrementar hora alarma
    rcall CHECK_BTN_PC3_DEC_LEFT_A ; PC3 -> decrementar hora alarma
    rcall CHECK_BTN_PC0_INC_RIGHT_A ; PC0 -> incrementar minuto alarma
    rcall CHECK_BTN_PC1_DEC_RIGHT_A ; PC1 -> decrementar minuto alarma
FIN_EDITAR_ALARMA:
    ret


;=============================================================================
; BLOQUE 4B: HANDLERS DE BOTONES INDIVIDUALES
; Cada handler implementa antirrebote identico al de PC4/PC5.
; Solo se comenta en detalle la primera variante (PC2 modo hora).
; Las demas son iguales; unicamente cambia el pin leido y la subrutina
; de edicion que se llama al confirmar la pulsacion.
;=============================================================================

; PC2 incrementa el valor izquierdo en modo HORA (accion: INC_HORA)
CHECK_BTN_PC2_INC_LEFT_H:
    sbic PINC, PC2                 ; Saltar si PC2=0 (presionado)
    rjmp PC2_H_SUELTO
    lds tempReg, btnCnt2
    cpi tempReg, 8                 ; Esperar 8 ticks estables
    brsh PC2_H_ESTABLE
    inc tempReg                    ; Acumular ticks de estabilidad
    sts btnCnt2, tempReg
    ret
PC2_H_ESTABLE:
    lds tempReg, btnLock2
    cpi tempReg, 1
    breq FIN_PC2_H                 ; Accion ya ejecutada: esperar soltar
    ldi tempReg, 1
    sts btnLock2, tempReg          ; Bloquear
    rcall INC_HORA                 ; Ejecutar accion
FIN_PC2_H:
    ret
PC2_H_SUELTO:
    ldi tempReg, 0
    sts btnCnt2, tempReg           ; Reiniciar al soltar
    sts btnLock2, tempReg
    ret

; PC2 incrementa el valor izquierdo en modo FECHA (accion: INC_DIA)
; (igual que en CHECK_BTN_PC2_INC_LEFT_H)
CHECK_BTN_PC2_INC_LEFT_F:
    sbic PINC, PC2
    rjmp PC2_F_SUELTO
    lds tempReg, btnCnt2
    cpi tempReg, 8
    brsh PC2_F_ESTABLE
    inc tempReg
    sts btnCnt2, tempReg
    ret
PC2_F_ESTABLE:
    lds tempReg, btnLock2
    cpi tempReg, 1
    breq FIN_PC2_F
    ldi tempReg, 1
    sts btnLock2, tempReg
    rcall INC_DIA
FIN_PC2_F:
    ret
PC2_F_SUELTO:
    ldi tempReg, 0
    sts btnCnt2, tempReg
    sts btnLock2, tempReg
    ret

; PC2 incrementa el valor izquierdo en modo ALARMA (accion: INC_ALRM_HORA)
; (igual que en CHECK_BTN_PC2_INC_LEFT_H)
CHECK_BTN_PC2_INC_LEFT_A:
    sbic PINC, PC2
    rjmp PC2_A_SUELTO
    lds tempReg, btnCnt2
    cpi tempReg, 8
    brsh PC2_A_ESTABLE
    inc tempReg
    sts btnCnt2, tempReg
    ret
PC2_A_ESTABLE:
    lds tempReg, btnLock2
    cpi tempReg, 1
    breq FIN_PC2_A
    ldi tempReg, 1
    sts btnLock2, tempReg
    rcall INC_ALRM_HORA
FIN_PC2_A:
    ret
PC2_A_SUELTO:
    ldi tempReg, 0
    sts btnCnt2, tempReg
    sts btnLock2, tempReg
    ret

; PC3 decrementa el valor izquierdo en modo HORA (accion: DEC_HORA)
; (igual que en CHECK_BTN_PC2_INC_LEFT_H)
CHECK_BTN_PC3_DEC_LEFT_H:
    sbic PINC, PC3
    rjmp PC3_H_SUELTO
    lds tempReg, btnCnt3
    cpi tempReg, 8
    brsh PC3_H_ESTABLE
    inc tempReg
    sts btnCnt3, tempReg
    ret
PC3_H_ESTABLE:
    lds tempReg, btnLock3
    cpi tempReg, 1
    breq FIN_PC3_H
    ldi tempReg, 1
    sts btnLock3, tempReg
    rcall DEC_HORA
FIN_PC3_H:
    ret
PC3_H_SUELTO:
    ldi tempReg, 0
    sts btnCnt3, tempReg
    sts btnLock3, tempReg
    ret

; PC3 decrementa el valor izquierdo en modo FECHA (accion: DEC_DIA)
; (igual que en CHECK_BTN_PC2_INC_LEFT_H)
CHECK_BTN_PC3_DEC_LEFT_F:
    sbic PINC, PC3
    rjmp PC3_F_SUELTO
    lds tempReg, btnCnt3
    cpi tempReg, 8
    brsh PC3_F_ESTABLE
    inc tempReg
    sts btnCnt3, tempReg
    ret
PC3_F_ESTABLE:
    lds tempReg, btnLock3
    cpi tempReg, 1
    breq FIN_PC3_F
    ldi tempReg, 1
    sts btnLock3, tempReg
    rcall DEC_DIA
FIN_PC3_F:
    ret
PC3_F_SUELTO:
    ldi tempReg, 0
    sts btnCnt3, tempReg
    sts btnLock3, tempReg
    ret

; PC3 decrementa el valor izquierdo en modo ALARMA (accion: DEC_ALRM_HORA)
; (igual que en CHECK_BTN_PC2_INC_LEFT_H)
CHECK_BTN_PC3_DEC_LEFT_A:
    sbic PINC, PC3
    rjmp PC3_A_SUELTO
    lds tempReg, btnCnt3
    cpi tempReg, 8
    brsh PC3_A_ESTABLE
    inc tempReg
    sts btnCnt3, tempReg
    ret
PC3_A_ESTABLE:
    lds tempReg, btnLock3
    cpi tempReg, 1
    breq FIN_PC3_A
    ldi tempReg, 1
    sts btnLock3, tempReg
    rcall DEC_ALRM_HORA
FIN_PC3_A:
    ret
PC3_A_SUELTO:
    ldi tempReg, 0
    sts btnCnt3, tempReg
    sts btnLock3, tempReg
    ret

; PC0 incrementa el valor derecho en modo HORA (accion: INC_MINUTO)
; (igual que en CHECK_BTN_PC2_INC_LEFT_H)
CHECK_BTN_PC0_INC_RIGHT_H:
    sbic PINC, PC0
    rjmp PC0_H_SUELTO
    lds tempReg, btnCnt0
    cpi tempReg, 8
    brsh PC0_H_ESTABLE
    inc tempReg
    sts btnCnt0, tempReg
    ret
PC0_H_ESTABLE:
    lds tempReg, btnLock0
    cpi tempReg, 1
    breq FIN_PC0_H
    ldi tempReg, 1
    sts btnLock0, tempReg
    rcall INC_MINUTO
FIN_PC0_H:
    ret
PC0_H_SUELTO:
    ldi tempReg, 0
    sts btnCnt0, tempReg
    sts btnLock0, tempReg
    ret

; PC0 incrementa el valor derecho en modo FECHA (accion: INC_MES)
; (igual que en CHECK_BTN_PC2_INC_LEFT_H)
CHECK_BTN_PC0_INC_RIGHT_F:
    sbic PINC, PC0
    rjmp PC0_F_SUELTO
    lds tempReg, btnCnt0
    cpi tempReg, 8
    brsh PC0_F_ESTABLE
    inc tempReg
    sts btnCnt0, tempReg
    ret
PC0_F_ESTABLE:
    lds tempReg, btnLock0
    cpi tempReg, 1
    breq FIN_PC0_F
    ldi tempReg, 1
    sts btnLock0, tempReg
    rcall INC_MES
FIN_PC0_F:
    ret
PC0_F_SUELTO:
    ldi tempReg, 0
    sts btnCnt0, tempReg
    sts btnLock0, tempReg
    ret

; PC0 incrementa el valor derecho en modo ALARMA (accion: INC_ALRM_MIN)
; (igual que en CHECK_BTN_PC2_INC_LEFT_H)
CHECK_BTN_PC0_INC_RIGHT_A:
    sbic PINC, PC0
    rjmp PC0_A_SUELTO
    lds tempReg, btnCnt0
    cpi tempReg, 8
    brsh PC0_A_ESTABLE
    inc tempReg
    sts btnCnt0, tempReg
    ret
PC0_A_ESTABLE:
    lds tempReg, btnLock0
    cpi tempReg, 1
    breq FIN_PC0_A
    ldi tempReg, 1
    sts btnLock0, tempReg
    rcall INC_ALRM_MIN
FIN_PC0_A:
    ret
PC0_A_SUELTO:
    ldi tempReg, 0
    sts btnCnt0, tempReg
    sts btnLock0, tempReg
    ret

; PC1 decrementa el valor derecho en modo HORA (accion: DEC_MINUTO)
; (igual que en CHECK_BTN_PC2_INC_LEFT_H)
CHECK_BTN_PC1_DEC_RIGHT_H:
    sbic PINC, PC1
    rjmp PC1_H_SUELTO
    lds tempReg, btnCnt1
    cpi tempReg, 8
    brsh PC1_H_ESTABLE
    inc tempReg
    sts btnCnt1, tempReg
    ret
PC1_H_ESTABLE:
    lds tempReg, btnLock1
    cpi tempReg, 1
    breq FIN_PC1_H
    ldi tempReg, 1
    sts btnLock1, tempReg
    rcall DEC_MINUTO
FIN_PC1_H:
    ret
PC1_H_SUELTO:
    ldi tempReg, 0
    sts btnCnt1, tempReg
    sts btnLock1, tempReg
    ret

; PC1 decrementa el valor derecho en modo FECHA (accion: DEC_MES)
; (igual que en CHECK_BTN_PC2_INC_LEFT_H)
CHECK_BTN_PC1_DEC_RIGHT_F:
    sbic PINC, PC1
    rjmp PC1_F_SUELTO
    lds tempReg, btnCnt1
    cpi tempReg, 8
    brsh PC1_F_ESTABLE
    inc tempReg
    sts btnCnt1, tempReg
    ret
PC1_F_ESTABLE:
    lds tempReg, btnLock1
    cpi tempReg, 1
    breq FIN_PC1_F
    ldi tempReg, 1
    sts btnLock1, tempReg
    rcall DEC_MES
FIN_PC1_F:
    ret
PC1_F_SUELTO:
    ldi tempReg, 0
    sts btnCnt1, tempReg
    sts btnLock1, tempReg
    ret

; PC1 decrementa el valor derecho en modo ALARMA (accion: DEC_ALRM_MIN)
; (igual que en CHECK_BTN_PC2_INC_LEFT_H)
CHECK_BTN_PC1_DEC_RIGHT_A:
    sbic PINC, PC1
    rjmp PC1_A_SUELTO
    lds tempReg, btnCnt1
    cpi tempReg, 8
    brsh PC1_A_ESTABLE
    inc tempReg
    sts btnCnt1, tempReg
    ret
PC1_A_ESTABLE:
    lds tempReg, btnLock1
    cpi tempReg, 1
    breq FIN_PC1_A
    ldi tempReg, 1
    sts btnLock1, tempReg
    rcall DEC_ALRM_MIN
FIN_PC1_A:
    ret
PC1_A_SUELTO:
    ldi tempReg, 0
    sts btnCnt1, tempReg
    sts btnLock1, tempReg
    ret


;=============================================================================
; BLOQUE 5: SUBRUTINAS DE EDICION DEL RELOJ Y FECHA
; Cada subrutina ajusta un campo BCD con wrap en sus limites.
; INC sube el valor; si supera el maximo, vuelve al minimo.
; DEC baja el valor; si ya esta en el minimo, salta al maximo.
;=============================================================================

; INC_MINUTO: 00-59 con wrap. Incrementa unidades; si llega a 10 hace borrow
; a decenas; si decenas llegan a 6, wrap completo a 00.
INC_MINUTO:
    inc unidades
    cpi unidades, 10
    brne FIN_INC_MINUTO
    ldi unidades, 0
    inc decenas
    cpi decenas, 6
    brne FIN_INC_MINUTO
    ldi decenas, 0
FIN_INC_MINUTO:
    ret

; DEC_MINUTO: 59 <- 00 con wrap.
DEC_MINUTO:
    cpi decenas, 0
    brne CHECK_DEC_MIN_2
    cpi unidades, 0
    brne CHECK_DEC_MIN_2
    ldi decenas, 5                 ; Si estaba en 00, saltar a 59
    ldi unidades, 9
    ret
CHECK_DEC_MIN_2:
    cpi unidades, 0
    brne DEC_MIN_U
    ldi unidades, 9                ; Borrow: unidades pasan a 9, decenas--
    dec decenas
    ret
DEC_MIN_U:
    dec unidades
    ret

; INC_HORA: 00-23 con wrap. Maneja el limite especial de 24 .
INC_HORA:
    inc unidadesh
    cpi decenash, 2
    brne INC_HORA_CHK10
    cpi unidadesh, 4               ; Si hora = 24, resetear a 00
    brne FIN_INC_HORA
    ldi unidadesh, 0
    ldi decenash, 0
    ret
INC_HORA_CHK10:
    cpi unidadesh, 10
    brne FIN_INC_HORA
    ldi unidadesh, 0               ; Borrow: pasar de X9 a (X+1)0
    inc decenash
FIN_INC_HORA:
    ret

; DEC_HORA: 23 <- 00 con wrap.
DEC_HORA:
    cpi decenash, 0
    brne CHECK_DEC_HORA_2
    cpi unidadesh, 0
    brne CHECK_DEC_HORA_2
    ldi decenash, 2                ; Si estaba en 00, saltar a 23
    ldi unidadesh, 3
    ret
CHECK_DEC_HORA_2:
    cpi unidadesh, 0
    brne DEC_HORA_U
    ldi unidadesh, 9               ; Borrow
    dec decenash
    cpi decenash, 2                ; Si quedo decenas=2, limite superior es 3 (no 9)
    brne FIN_DEC_HORA
    ldi unidadesh, 3
    ret
DEC_HORA_U:
    dec unidadesh
FIN_DEC_HORA:
    ret

; INC_DIA: incrementa el dia con wrap al maximo del mes actual.
; Llama a GET_MAX_DIA para obtener el tope antes de incrementar.
INC_DIA:
    rcall GET_MAX_DIA              ; tempReg=decenas max, auxReg=unidades max
    inc diaU
    cpi diaU, 10
    brne CHECK_OVERFLOW_DIA
    ldi diaU, 0
    inc diaD
CHECK_OVERFLOW_DIA:
    cp diaD, tempReg               ; Comparar decenas del dia con el maximo
    brlo FIN_INC_DIA
    breq CHECK_OVERFLOW_DIA_U
    ldi diaD, 0                    ; Supero el maximo: reiniciar a dia 01
    ldi diaU, 1
    ret
CHECK_OVERFLOW_DIA_U:
    cp diaU, auxReg                ; Verificar unidades cuando decenas son iguales
    brlo FIN_INC_DIA
    breq FIN_INC_DIA
    ldi diaD, 0
    ldi diaU, 1
FIN_INC_DIA:
    ret

; DEC_DIA: decrementa el dia; si estaba en 01, salta al ultimo dia del mes.
DEC_DIA:
    cpi diaD, 0
    brne CHECK_DEC_DIA_2
    cpi diaU, 1
    brne CHECK_DEC_DIA_2
    rcall GET_MAX_DIA              ; Wrap al ultimo dia del mes
    mov diaD, tempReg
    mov diaU, auxReg
    ret
CHECK_DEC_DIA_2:
    cpi diaU, 0
    brne DEC_DIA_U
    ldi diaU, 9                    ; Borrow: unidades a 9, decenas--
    dec diaD
    ret
DEC_DIA_U:
    dec diaU
    ret

; INC_MES: incrementa el mes (01-12) y ajusta el dia si el mes nuevo
; tiene menos dias. Al pasar de diciembre avanza el ciclo bisiesto.
INC_MES:
    cpi mesD, 0
    brne INC_MES_DEC1              ; Meses 10-12: decenas = 1
    inc mesU
    cpi mesU, 10                   ; Mes 09->10: cambiar decenas
    brne CLAMP_DIA_INC_MES
    ldi mesU, 0
    ldi mesD, 1
    rjmp CLAMP_DIA_INC_MES
INC_MES_DEC1:
    inc mesU
    cpi mesU, 3                    ; Mes 12->01: wrap y avanzar ciclo bisiesto
    brne CLAMP_DIA_INC_MES
    ldi mesU, 1
    ldi mesD, 0
    lds tempReg, anio4
    inc tempReg
    cpi tempReg, 4
    brne SAVE_ANIO4_INC
    ldi tempReg, 0                 ; Cada 4 anios reiniciar ciclo bisiesto
SAVE_ANIO4_INC:
    sts anio4, tempReg
CLAMP_DIA_INC_MES:
    rcall CLAMP_DIA_A_MES          ; Ajustar dia si excede el nuevo mes
    ret

; DEC_MES: decrementa el mes (12 <- 01) y ajusta el dia.
; Al pasar de enero a diciembre retrocede el ciclo bisiesto.
DEC_MES:
    cpi mesD, 0
    brne DEC_MES_DEC1
    cpi mesU, 1                    ; Si es enero, saltar a diciembre
    brne DEC_MES_NORMAL0
    ldi mesD, 1
    ldi mesU, 2
    lds tempReg, anio4
    cpi tempReg, 0
    brne DEC_ANIO4_NORMAL
    ldi tempReg, 3                 ; Si ciclo estaba en 0, retroceder a 3
    rjmp SAVE_ANIO4_DEC
DEC_ANIO4_NORMAL:
    dec tempReg
SAVE_ANIO4_DEC:
    sts anio4, tempReg
    rjmp CLAMP_DIA_DEC_MES
DEC_MES_NORMAL0:
    dec mesU
    rjmp CLAMP_DIA_DEC_MES
DEC_MES_DEC1:
    cpi mesU, 0                    ; Si es octubre (10), pasar a septiembre (09)
    brne DEC_MES_NORMAL1
    ldi mesD, 0
    ldi mesU, 9
    rjmp CLAMP_DIA_DEC_MES
DEC_MES_NORMAL1:
    dec mesU
CLAMP_DIA_DEC_MES:
    rcall CLAMP_DIA_A_MES
    ret


;=============================================================================
; BLOQUE 5B: SUBRUTINAS DE EDICION DE LA ALARMA
; Misma logica BCD que las subrutinas del reloj, pero operando sobre
; alrm_hora_u/d y alrm_min_u/d en SRAM en lugar de registros directos.
;=============================================================================

; INC_ALRM_HORA: 00-23 con wrap, igual que INC_HORA pero en SRAM.
INC_ALRM_HORA:
    lds tempReg, alrm_hora_u
    inc tempReg
    lds auxReg, alrm_hora_d
    cpi auxReg, 2
    brne INC_ALRM_HORA_CHK10
    cpi tempReg, 4                 ; hora_d=2, hora_u llego a 4 -> reset a 00:00
    brne INC_ALRM_HORA_SAVE_U
    ldi tempReg, 0
    sts alrm_hora_u, tempReg
    sts alrm_hora_d, tempReg
    ret
INC_ALRM_HORA_CHK10:
    cpi tempReg, 10                ; Si unidades llegan a 10, borrow a decenas
    brne INC_ALRM_HORA_SAVE_U
    ldi tempReg, 0
    sts alrm_hora_u, tempReg
    inc auxReg
    sts alrm_hora_d, auxReg
    ret
INC_ALRM_HORA_SAVE_U:
    sts alrm_hora_u, tempReg
    ret

; DEC_ALRM_HORA: 23 <- 00 con wrap, igual que DEC_HORA pero en SRAM.
DEC_ALRM_HORA:
    lds auxReg, alrm_hora_d
    lds tempReg, alrm_hora_u
    tst auxReg
    brne DEC_ALRM_HORA_CHK
    tst tempReg
    brne DEC_ALRM_HORA_CHK
    ldi tempReg, 2                 ; Estaba en 00:00 -> saltar a 23
    sts alrm_hora_d, tempReg
    ldi tempReg, 3
    sts alrm_hora_u, tempReg
    ret
DEC_ALRM_HORA_CHK:
    tst tempReg
    brne DEC_ALRM_HORA_U
    ldi tempReg, 9                 ; Borrow: unidades a 9, decenas--
    sts alrm_hora_u, tempReg
    dec auxReg
    sts alrm_hora_d, auxReg
    cpi auxReg, 2                  ; Si decenas quedaron en 2, max unidades es 3
    brne FIN_DEC_ALRM_HORA
    ldi tempReg, 3
    sts alrm_hora_u, tempReg
    ret
DEC_ALRM_HORA_U:
    dec tempReg
    sts alrm_hora_u, tempReg
FIN_DEC_ALRM_HORA:
    ret

; INC_ALRM_MIN: 00-59 con wrap, igual que INC_MINUTO pero en SRAM.
INC_ALRM_MIN:
    lds tempReg, alrm_min_u
    inc tempReg
    cpi tempReg, 10
    brne SAVE_ALRM_MIN_U
    ldi tempReg, 0
    sts alrm_min_u, tempReg
    lds tempReg, alrm_min_d
    inc tempReg
    cpi tempReg, 6                 ; Si decenas llegan a 6, wrap a 00
    brne SAVE_ALRM_MIN_D
    ldi tempReg, 0
SAVE_ALRM_MIN_D:
    sts alrm_min_d, tempReg
    ret
SAVE_ALRM_MIN_U:
    sts alrm_min_u, tempReg
    ret

; DEC_ALRM_MIN: 59 <- 00 con wrap, igual que DEC_MINUTO pero en SRAM.
DEC_ALRM_MIN:
    lds auxReg, alrm_min_d
    lds tempReg, alrm_min_u
    tst auxReg
    brne DEC_ALRM_MIN_CHK
    tst tempReg
    brne DEC_ALRM_MIN_CHK
    ldi tempReg, 5                 ; Estaba en 00 -> saltar a 59
    sts alrm_min_d, tempReg
    ldi tempReg, 9
    sts alrm_min_u, tempReg
    ret
DEC_ALRM_MIN_CHK:
    tst tempReg
    brne DEC_ALRM_MIN_U
    ldi tempReg, 9                 ; Borrow: unidades a 9, decenas--
    sts alrm_min_u, tempReg
    dec auxReg
    sts alrm_min_d, auxReg
    ret
DEC_ALRM_MIN_U:
    dec tempReg
    sts alrm_min_u, tempReg
    ret


;=============================================================================
; BLOQUE 6: SUBRUTINAS DE CALENDARIO
;=============================================================================

; GET_MAX_DIA: determina el ultimo dia del mes activo.
; Retorna: tempReg = decenas max, auxReg = unidades max
GET_MAX_DIA:
    cpi mesD, 0                    ; Meses 01-09: decenas de mes = 0
    brne MAX_MES_DEC1
    cpi mesU, 2
    breq MAX_FEB                   ; Febrero: verificar bisiesto
    cpi mesU, 4
    breq MAX_30                    ; Abril: 30 dias
    cpi mesU, 6
    breq MAX_30                    ; Junio: 30 dias
    cpi mesU, 9
    breq MAX_30                    ; Septiembre: 30 dias
    rjmp MAX_31                    ; Resto: 31 dias
MAX_MES_DEC1:                      ; Meses 10-12
    cpi mesU, 1
    breq MAX_30                    ; Noviembre: 30 dias
    rjmp MAX_31                    ; Octubre y Diciembre: 31 dias
MAX_FEB:
    lds tempReg, anio4
    cpi tempReg, 0
    breq MAX_29                    ; anio4=0 -> bisiesto: 29 dias
    ldi tempReg, 2                 ; No bisiesto: 28 dias
    ldi auxReg, 8
    ret
MAX_29:
    ldi tempReg, 2
    ldi auxReg, 9
    ret
MAX_30:
    ldi tempReg, 3
    ldi auxReg, 0
    ret
MAX_31:
    ldi tempReg, 3
    ldi auxReg, 1
    ret

; CLAMP_DIA_A_MES: fuerza el dia al maximo si lo supera .
CLAMP_DIA_A_MES:
    rcall GET_MAX_DIA
    cp diaD, tempReg               ; Comparar decenas del dia con el tope
    brlo FIN_CLAMP
    breq CLAMP_CHECK_U
    mov diaD, tempReg              ; Decenas superan el tope: clampear
    mov diaU, auxReg
    ret
CLAMP_CHECK_U:
    cp diaU, auxReg                ; Decenas iguales: revisar unidades
    brlo FIN_CLAMP
    breq FIN_CLAMP
    mov diaD, tempReg
    mov diaU, auxReg
FIN_CLAMP:
    ret

; INCREMENTAR_DIA: avanza el dia en 1. Si supera el mes, reinicia a 01
; e incrementa el mes.
INCREMENTAR_DIA:
    rcall GET_MAX_DIA
    inc diaU
    cpi diaU, 10
    brne CHECK_WRAP_DIA
    ldi diaU, 0
    inc diaD
CHECK_WRAP_DIA:
    cp diaD, tempReg
    brlo FIN_INC_DIA_AUTO
    breq CHECK_WRAP_DIA_U
    ldi diaD, 0                    ; Supero el mes: reiniciar a dia 01 y avanzar mes
    ldi diaU, 1
    rcall INC_MES
    ret
CHECK_WRAP_DIA_U:
    cp diaU, auxReg
    brlo FIN_INC_DIA_AUTO
    breq FIN_INC_DIA_AUTO
    ldi diaD, 0
    ldi diaU, 1
    rcall INC_MES
FIN_INC_DIA_AUTO:
    ret


;=============================================================================
; BLOQUE 7A:  Generador de tono 500 Hz
;=============================================================================
ISR_TIMER0_COMPA:
    push tempReg                   ; Preservar registros usados en la ISR
    push auxReg
    in tempReg, SREG
    push tempReg                   ; Preservar flags

    lds tempReg, tono_cnt
    inc tempReg                    ; Avanzar 1 ms en el ciclo

    cpi tempReg, 50
    brlo T0_GUARDAR_CNT
    ldi tempReg, 0                 ; Fin de ciclo: reiniciar a 0

T0_GUARDAR_CNT:
    sts tono_cnt, tempReg          ; Guardar posicion actualizada

    cpi tempReg, 25
    brlo T0_TOGGLE_PB4             ; 0-24: fase de tono

    cbi PORTB, PB4                 ; 25-49: fase de silencio, forzar PB4 bajo
    rjmp T0_FIN

T0_TOGGLE_PB4:
    in auxReg, PORTB
    ldi tempReg, (1<<PB4)
    eor auxReg, tempReg            ; Invertir solo PB4, sin alterar PB0-PB3
    out PORTB, auxReg

T0_FIN:
    pop tempReg
    out SREG, tempReg              ; Restaurar flags
    pop auxReg
    pop tempReg
    reti


;=============================================================================
; BLOQUE 7B: ISR TIMER1 COMPARE A - Base de tiempo de 1 segundo
; Se dispara cada 1 segundo. Realiza tres tareas:
;   1. Alterna ledmedio y PD7 (parpadeo de edicion a 1 Hz)
;   2. Si la alarma esta armada, compara HH:MM con el reloj; al coincidir
;      arranca Timer0 y pone alarma_activa = 2
;   3. Avanza el reloj: segundos -> minutos -> horas -> dia (a las 24:00)
;=============================================================================
ISR_TIMER1_COMPA:
    push tempReg                   ; igual que en ISR_TIMER0_COMPA: preservar contexto
    push auxReg
    in tempReg, SREG
    push tempReg

    inc tickCount                  ; Contar segundos transcurridos (0-59)
    ldi tempReg, 1
    eor ledmedio, tempReg          ; Invertir bandera de parpadeo (cada 1 s)
    in tempReg, PORTD
    ldi auxReg, (1<<PD7)
    eor tempReg, auxReg            ; Invertir LED central en hardware
    out PORTD, tempReg

    ; --- Verificar coincidencia de alarma (solo si alarma_activa = 1) ---
    lds tempReg, alarma_activa
    cpi tempReg, 1
    brne ISR1_RELOJ                ; No armada: saltar comparacion

    lds tempReg, alrm_hora_d
    cp tempReg, decenash           ; Comparar decenas de hora
    brne ISR1_RELOJ
    lds tempReg, alrm_hora_u
    cp tempReg, unidadesh          ; Comparar unidades de hora
    brne ISR1_RELOJ
    lds tempReg, alrm_min_d
    cp tempReg, decenas            ; Comparar decenas de minuto
    brne ISR1_RELOJ
    lds tempReg, alrm_min_u
    cp tempReg, unidades           ; Comparar unidades de minuto
    brne ISR1_RELOJ

    ; Coincidencia HH:MM -> activar tono
    ldi tempReg, 2
    sts alarma_activa, tempReg     ; Marcar alarma como sonando
    ldi tempReg, 0
    sts tono_cnt, tempReg          ; Iniciar ciclo de tono desde el principio
    ldi tempReg, (1<<CS01)|(1<<CS00)
    out TCCR0B, tempReg            ; Arrancar Timer0 con prescaler 64

    ; --- Avance del reloj (cada 60 ticks = 1 minuto) ---
ISR1_RELOJ:
    cpi tickCount, 60
    brne FIN_ISR

    ldi tickCount, 0               ; Reiniciar contador de segundos

    inc unidades                   ; Incrementar unidades de minuto
    cpi unidades, 10
    brne FIN_ISR
    ldi unidades, 0
    inc decenas                    ; Incrementar decenas de minuto

    cpi decenas, 6                 ; 60 minutos = 1 hora
    brne FIN_ISR
    ldi decenas, 0

    inc unidadesh                  ; Incrementar unidades de hora
    cpi unidadesh, 10
    brne CHECK_24H
    ldi unidadesh, 0
    inc decenash                   ; Incrementar decenas de hora

CHECK_24H:
    cpi decenash, 2                ; Verificar si son las 24:00
    brne FIN_ISR
    cpi unidadesh, 4
    brne FIN_ISR

    ldi unidadesh, 0               ; Reiniciar reloj a 00:00
    ldi decenash, 0
    rcall INCREMENTAR_DIA          ; Avanzar la fecha automaticamente

FIN_ISR:
    pop tempReg                    ; Restaurar contexto
    out SREG, tempReg
    pop auxReg
    pop tempReg
    reti


;=============================================================================
; BLOQUE 8: RETARDOS DE MULTIPLEXADO
;=============================================================================
DelayPequeno:
    ldi tempReg, 50                ; Retardo largo para tiempo encendido por display

Delayparpadeo:
    ldi tempReg, 10                ; Retardo corto para la fase apagada del parpadeo

DelayLoop:
    dec tempReg                    ; Decrementar hasta llegar a 0
    brne DelayLoop
    ret