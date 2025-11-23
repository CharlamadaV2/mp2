/* EECE.4520 Microprocessors II & Embedded Systems
Project: Lab 3: Audio-Driven Kinetic Sculpture with Infrared Control
Kimson Lam and Nathan Hoang

References:
https://docs.arduino.cc/tutorials/motor-shield-rev3/msr3-controlling-dc-motor/
https://docs.arduino.cc/learn/electronics/lcd-displays/
https://soldered.com/learn/hum-rtc-real-time-clock-ds1307/
https://sensorkit.arduino.cc/sensorkit/module/lessons/lesson/06-the-sound-sensor
https://lastminuteengineers.com/l293d-dc-motor-arduino-tutorial/
*/

#include <LiquidCrystal.h>
#include <DS3231.h>
#include <Wire.h>

// Button
const int button_pin = 2;

// Sound sensor
const int sound_sensor = A2;

// RTC (I2C)
DS3231 rtc(A4, A5);
volatile DateTime now;

// Motor driver (L293D)
const int enA = 9;
const int in1 = 8;
const int in2 = 7;

// LCD pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 6);

// Globals
volatile String dirLabel = "C";     // “C” or “CC”
volatile String speedLabel = "0";   // “Full”, “3/4”, “1/2”, or “0”

void setup() {
    Serial.begin(9600);

    pinMode(button_pin, INPUT);
    pinMode(in1, OUTPUT);
    pinMode(in2, OUTPUT);
    pinMode(enA, OUTPUT);

    now = rtc.now();

    lcd.begin(16, 2);
    lcd.print("Initializing...");

    // Timer1 interrupt at 1 Hz
    cli();
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    OCR1A = 15624;                       // 1 Hz
    TCCR1B |= (1 << WGM12);
    TCCR1B |= (1 << CS12) | (1 << CS10); // 1024 prescaler
    TIMSK1 |= (1 << OCIE1A);
    sei();
}

void loop() {

    // Motor direction from button 
    if (digitalRead(button_pin) == HIGH) {
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
        dirLabel = "CC";   // counter-clockwise
    } else {
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
        dirLabel = "C";    // clockwise
    }

    // Read sound and set speed 
    int raw = analogRead(sound_sensor);
    update_fan_speed(raw);

    delay(50);
}

// Timer interrupt: update LCD every second 
ISR(TIMER1_COMPA_vect) {
    now = rtc.now();  // Get actual HH:MM:SS

    lcd.clear();

    // Row 1: Direction + Speed label
    lcd.setCursor(0, 0);
    lcd.print("Dir:");
    lcd.print(dirLabel);
    lcd.print(" Sp:");
    lcd.print(speedLabel);

    // Row 2: Display RTC clock HH:MM:SS
    lcd.setCursor(0, 1);
    lcd.print(now.hour() < 10 ? "0" : ""); lcd.print(now.hour());
    lcd.print(":");
    lcd.print(now.minute() < 10 ? "0" : ""); lcd.print(now.minute());
    lcd.print(":");
    lcd.print(now.second() < 10 ? "0" : ""); lcd.print(now.second());
}

// ---- Convert sound into speed levels ----
void update_fan_speed(int sound_value) {
    int out = 0;

    if (sound_value >= 700) {
        out = 255;
        speedLabel = "Full";
    }
    else if (sound_value >= 500) {
        out = 191;
        speedLabel = "3/4";
    }
    else if (sound_value >= 300) {
        out = 127;
        speedLabel = "1/2";
    }
    else {
        out = 0;
        speedLabel = "0";
    }

    analogWrite(enA, out);
}