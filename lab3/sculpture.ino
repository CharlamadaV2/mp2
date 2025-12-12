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
#include <RTClib.h>

// Button
const int button_pin = 2;
// Sound sensor
const int sound_sensor = A2;


// RTC instance
RTC_DS1307 rtc;

// Motor driver L293D
const int enA = 9;
const int in1 = 8;
const int in2 = 7;

// LCD pins()
LiquidCrystal lcd(12, 11, 5, 4, 3, 6);

// Use NON-volatile arrays + separate volatile flags if needed
char dirLabel[4] = "C";      // "C", "CC" → max 3 chars + '\0'
char speedLabel[6] = "0";    // "Full" is 4 + '\0' = 5, so [6] is safe

// Software clock
volatile int hours = 0;
volatile int minutes = 0;
volatile int seconds = 0;

void setup() {

   Serial.begin(9600);
  pinMode(button_pin, INPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(enA, OUTPUT);

  lcd.begin(16, 2);
  lcd.print("Initializing...");
  delay(1000);

  // RTC setup
  if (!rtc.begin()) {
    lcd.clear();
    lcd.print("RTC ERROR");
    while (1);
  }

  if (!rtc.isrunning()) {
    lcd.clear();
    lcd.print("Setting RTC...");
    // THIS IS THE CORRECT WAY:
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Read initial time once
  DateTime t = rtc.now();
  hours = t.hour();
  minutes = t.minute();
  seconds = t.second();

  // Timer1 interrupt at 1 Hz (16MHz / 1024 / 15625 = 1Hz)
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 15624;                    // 15625 - 1 because count starts at 0
  TCCR1B |= (1 << WGM12);           // CTC mode
  TCCR1B |= (1 << CS12) | (1 << CS10); // 1024 prescaler
  TIMSK1 |= (1 << OCIE1A);          // Enable compare interrupt
  sei();
}

void loop() {
  // Motor direction from button
  if (digitalRead(button_pin) == HIGH) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    strcpy(dirLabel, "CC");   // counter-clockwise
  } else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    strcpy(dirLabel, "C");    // clockwise
  }

  // Read sound and set speed
  int raw = analogRead(sound_sensor);
  update_fan_speed(raw);

  delay(50);
}

ISR(TIMER1_COMPA_vect) {
  seconds++;
  if (seconds >= 60) {
    seconds = 0;
    minutes++;
    if (minutes >= 60) {
      minutes = 0;
      hours = (hours + 1) % 24;
    }
  }

  lcd.clear();
  // Row 1
  lcd.setCursor(0, 0);
  lcd.print("Dir:");
  lcd.print(dirLabel);        // now safe – not volatile
  lcd.print(" Sp:");
  lcd.print(speedLabel);

  // Row 2 - clock
  lcd.setCursor(0, 1);
  if (hours < 10) lcd.print("0");
  lcd.print(hours);
  lcd.print(":");
  if (minutes < 10) lcd.print("0");
  lcd.print(minutes);
  lcd.print(":");
  if (seconds < 10) lcd.print("0");
  lcd.print(seconds);
}

void update_fan_speed(int sound_value) {
  int out = 0;
  if (sound_value >= 700) {
    out = 255;
    strcpy(speedLabel, "Full");
  } else if (sound_value >= 500) {
    out = 191;
    strcpy(speedLabel, "3/4");
  } else if (sound_value >= 300) {
    out = 127;
    strcpy(speedLabel, "1/2");
  } else {
    out = 0;
    strcpy(speedLabel, "0");
  }
  analogWrite(enA, out);
}

