#define __SFR_OFFSET 0x00

; include avr/io.h to access definitions for PORTB, SREG, etc
#include "avr/io.h"


#define RED_LED_PIN 2 ; PORTD2 for Red LED (pin 10) 
#define YELLOW_LED_PIN 3 ; PORTD3 for Yellow LED (pin 11) 
#define GREEN_LED_PIN 4 ; PORTD4 for Green LED (pin 12) 
#define BUZZER_PIN 5 ; PORTD5 for Buzzer (pin 13)

.global start
.global red_led 
.global green_led 
.global yellow_led 
.global buzzer

.section .text    ; Defines a code section

start:
    SBI DDRD, RED_LED_PIN ; Set Red LED pin (PORTD2) as output 
    SBI DDRD, YELLOW_LED_PIN ; Set Yellow LED pin (PORTD3) as output 
    SBI DDRD, GREEN_LED_PIN ; Set Green LED pin (PORTD4) as output 
    SBI DDRD, BUZZER_PIN ; Set Buzzer pin (PORTD5) as output 
    RET

;used for red_led at pin 2
red_led:
    CPI R24, 0x00
    BREQ  red_ledOFF
    SBI PORTB, RED_LED_PIN 
    RET

red_ledOFF:
    CBI PORTB, RED_LED_PIN
    RET

; Green LED at pin 12 (PORTD4) 
green_led: 
    CPI R24, 0x00 
    BREQ green_ledOFF 
    SBI PORTD, GREEN_LED_PIN 
    RET

green_ledOFF: 
    CBI PORTD, GREEN_LED_PIN 
    RET

; Yellow LED at pin 11 (PORTD3) 
    yellow_led: CPI R24, 0x00 
    BREQ yellow_ledOFF 
    SBI PORTD, YELLOW_LED_PIN 
    RET

yellow_ledOFF: 
    CBI PORTD, YELLOW_LED_PIN 
    RET

; Buzzer at pin 13 (PORTD5) 
buzzer: 
    CPI R24, 0x00 
    BREQ buzzerOFF 
    SBI PORTD, BUZZER_PIN 
    RET

buzzerOFF: 
    CBI PORTD, BUZZER_PIN 
    RET





