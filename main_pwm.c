/*
 * File:   main_pwm.c
 * Author: Rodolfo E. Escobar U.
 *
 * Created on 28 de febrero de 2021, 05:15 PM
 */

#include <xc.h>
#include "config.h"
#include <pic16f15313.h>

#define _XTAL_FREQ 32000000UL

//**** Periodo TMR0 ****
// Tint = 4*TOSC*Prescaler*(65535-TMR0)
// 65535 - Tint/(4*TOSC*Prescaler) = TMR0
#define Tin 65385 // 48 kHz 
//**********************

//---- Señal Sinc(ind) ----
volatile char ind = 0; // variable de indice
#define N 158  // Numero de muestras
const char Y[N] = {44,44,44,44,43,43,43,43,43,43,44,44,44,44,44,44,44,43,43,
                   43,43,43,43,44,45,45,45,45,44,43,42,41,41,41,42,44,46,47,
                   48,47,46,44,41,39,37,37,39,42,47,50,53,53,51,46,40,34,30,
                   28,31,37,46,55,64,67,66,57,44,27,11,1,0,11,35,71,115,162,
                   205,237,255,255,237,205,162,115,71,35,11,0,1,11,27,44,57,
                   66,67,64,55,46,37,31,28,30,34,40,46,51,53,53,50,47,42,39,
                   37,37,39,41,44,46,47,48,47,46,44,42,41,41,41,42,43,44,45,
                   45,45,45,44,43,43,43,43,43,43,44,44,44,44,44,44,44,43,43,
                   43,43,43,43,44,44,44,44};

//----------------------

// XC8 C Compiler User?s Guide (DS50002737A), pag. 134
void __interrupt(high_priority) IntTMR0(void){
    if(PIR0bits.TMR0IF == 1){
        
        TMR0H = (Tin) >> 8; //Carga de periodo (parte alta)
        TMR0L =  Tin;       //Carga de periodo (parte baja)
        
        CCPR1L = Y[ind++]; // 8 primeros bits de ancho de pulso (pag. 308)
        if (ind>=N) PIE0bits.TMR0IE = 0; // Desactivar interrupción por TMR0
        PIR0bits.TMR0IF = 0;
    }
return;
}

void PWM(int x){
    CCPR1L = x;    // 8 primeros bits de ancho de pulso (pag. 308)
    CCPR1H = x>>8; // últimos 2 bits de ancho de pulso (pag. 309)
}

void main(void) {
    //--- Setup ---
    TRISAbits.TRISA1 = 0; // LED de estado
    TRISAbits.TRISA2 = 0; // Salida PWM
    
    // Mapeo de pin
    RA2PPS = 0x09; //CCP1 a A2 (pag. 190 y 185[Sec. 15.2]) 
    
    //***** TMR0 *****
    INTCONbits.GIE = 1;       // Activar interrupciones 
    PIE0bits.TMR0IE = 1;      // Activar interrupción por TMR0
    T0CON0bits.T0EN = 0;      //Desctivar TMR0 (pag. 263)
    T0CON0bits.T016BIT = 1;   //Modo 16 bits (pag. 263)
    T0CON1bits.T0CS = 0b010;  // Fuente de reloj: FOSC/4 (pag. 264)
    T0CON1bits.T0PS = 0b000;  // Prescaler TMR0 1:1 (pag. 264) 
    T0CON0bits.T0EN = 1;    //Activar TMR0
    //***************
    
    //***** PWM *****
    TRISAbits.TRISA2 = 1;
    // -- TMR2 --
    T2CONbits.TMR2ON = 0;     // Apagar TMR2 (pag. 295)
    T2CLKCONbits.CS = 0b0001; // Fuente FOSC/4 (pag. 294)
    TMR2 = 0x00;              // Reiniciar timer
    T2CONbits.T2CKPS = 0b00;  // Prescaler TMR2 1:1
    //--- Periodo PWM = (PR2+1)*4*Tosc*T2_PRESCALER
    //    Tpwm/(4*Tosc*T2PSC)-1 = PR2
    PR2 = 0x3F; // 124.3 kHz a 8 bits  (ver Tabla 28-2, pag. 305)
    CCP1CONbits.CCP1MODE = 0b1111; // Modo PWM con TMR2 (pag. 307)
    CCP1CONbits.CCP1FMT = 0;       // Modo alineación derecha (pag. 306)
                                   //    -> CCPR1H[0:1]|CCPR1L[0:7] 
    T2CONbits.TMR2ON = 1;    // Encender TMR2 (pag. 295)
    CCP1CONbits.CCP1EN = 1;  // Habilitar CCP1
    TRISAbits.TRISA2 = 0;
    //***************
    PWM(43); // Nivel base
    //------------
    
    while(1) // Baliza Sinc (1 impulso por segundo)
    {
        LATAbits.LATA1 = 1;
        ind = 0;
        TMR0H = (Tin) >> 8;  //Carga de periodo (parte alta)
        TMR0L =  Tin;        //Carga de periodo (parte baja)
        PIE0bits.TMR0IE = 1; // Activar interrupción por TMR0
        __delay_ms(2);
        LATAbits.LATA1 = 0;
        __delay_ms(1000);
    }
    return;
}
