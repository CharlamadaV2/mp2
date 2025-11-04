/* EECE.4520 Microprocessors II & Embedded Systems
   Project: Lab 1: Traffic Light Controller
   This project was done by Kimson Lam with the help of code from Andrew Borin
*/

#include <String.h>
#include <avr/pgmspace.h>
#include "Keypad.h"

// === OUTPUT PINS ===
const int buzzerPin   = 13;   // Buzzer output
const int redLED      = 12;   // Red traffic light
const int yellowLED   = 11;   // Yellow transition light
const int greenLED    = 10;   // Green go light

boolean initialBlink = true;  // Controls one-time startup blink

// === KEYPAD CONFIGURATION ===
const byte ROWS = 4;
const byte COLS = 4;
const char keyMap[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {28, 30, 32, 34};      // Keypad row connections
byte colPins[COLS] = {36, 38, 40, 42};      // Keypad column connections
Keypad keypad = Keypad(makeKeymap(keyMap), rowPins, colPins, ROWS, COLS);

// === UNUSED BUTTON (kept for compatibility) ===
const int buttonPin       = 2;
int       buttonState     = HIGH;
int       lastButtonState = HIGH;
bool      ledState        = false;
int       stableCount     = 0;
bool      manualMode      = false;

// === TIMING CONSTANTS (not directly used in logic) ===
const int minRedTime   = 10;
const int minGreenTime = 10;
const int yellowTime   = 3;

// === STATE MACHINE ===
enum { Initial, RedIN, GreenIN, InputComplete, RedSolid, RedBlink, GreenSolid, GreenBlink, YellowPhase };
unsigned char currentState = Initial;

// === TIMING & CONTROL VARIABLES ===
unsigned int redDuration   = 0;   // User-set red light time (seconds)
unsigned int greenDuration = 0;   // User-set green light time (seconds)
unsigned int tempCounter   = 0;   // Counts seconds or blink cycles
unsigned int activeLED     = 0;   // Currently blinking LED (red or green)

bool blinkToggle = false;         // Toggles LED/buzzer during blink phase

volatile bool oneSecTick   = false;  // 1-second timer flag
volatile bool halfSecTick  = false;  // 0.5-second timer flag
volatile bool redConfigured   = false;
volatile bool greenConfigured = false;
volatile bool setupComplete   = false;

void setup() {
  // Configure output pins
  pinMode(redLED,    OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(greenLED,  OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  Serial.begin(9600);

  // === TIMER1 SETUP: 1Hz base clock ===
  cli();  // Disable interrupts during setup
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = 15624;                    // 1 second @ 16MHz with /1024 prescaler
  TCCR1B |= (1 << WGM12);           // CTC mode
  TCCR1B |= (1 << CS12) | (1 << CS10); // Prescaler 1024
  TIMSK1 |= (1 << OCIE1A);          // Enable COMPA interrupt
  TIMSK1 &= ~(1 << OCIE1B);         // Disable COMPB initially

  sei();  // Re-enable interrupts
}

// 1-second interrupt
ISR(TIMER1_COMPA_vect) {
  oneSecTick = true;
}

// 0.5-second interrupt (used for blinking)
ISR(TIMER1_COMPB_vect) {
  halfSecTick = true;
}

// === GET TWO-DIGIT INPUT FROM KEYPAD (10–99) ===
int getUserInput() {
  while (true) {
    String inputBuffer = "";
    
    // Get exactly two digits
    for (int digit = 0; digit < 2; digit++) {
      while (true) {
        char key = keypad.getKey();
        if (key >= '0' && key <= '9') {
          inputBuffer += key;
          Serial.println("input: " + inputBuffer);
          break;
        }
      }
    }

    int value = inputBuffer.toInt();
    if (value >= 10 && value <= 99) {
      return value;
    } else {
      Serial.println("inputed number was too small or large (range is 10 - 99)");
    }
  }
}

void loop() {
  // === CHECK FOR STATE ENTRY KEYS ===
  char entryKey = keypad.getKey();
  if (entryKey == 'A') {
    currentState = RedIN;
    // Turn off all outputs when entering config
    digitalWrite(redLED,    LOW);
    digitalWrite(greenLED,  LOW);
    digitalWrite(yellowLED, LOW);
    digitalWrite(buzzerPin, LOW);
  }
  else if (entryKey == 'B') {
    currentState = GreenIN;
    digitalWrite(redLED,    LOW);
    digitalWrite(greenLED,  LOW);
    digitalWrite(yellowLED, LOW);
    digitalWrite(buzzerPin, LOW);
  }

  // === 1-SECOND TIMER TICK ===
  if (oneSecTick) {
    oneSecTick = false;

    switch (currentState) {

      case RedIN:
        TIMSK1 &= ~(1 << OCIE1A);  // Pause 1Hz timer during input
        Serial.println("Inside RedIN");
        redDuration = getUserInput();
        Serial.print("This is the RedTime: ");
        Serial.println(redDuration);

        while (true) {
          char confirmKey = keypad.getKey();
          if (confirmKey == '#') {
            if (greenDuration != 0 && greenConfigured) {
              currentState = InputComplete;
            }
            else if (!redConfigured && !setupComplete) {
              currentState = GreenIN;
            }
            else {
              currentState = RedSolid;
              tempCounter = 0;
            }
            break;
          }
          else if (confirmKey == 'A') {
            currentState = RedIN;
            break;
          }
        }
        TIMSK1 |= (1 << OCIE1A);  // Resume 1Hz timer
        break;

      case GreenIN:
        TIMSK1 &= ~(1 << OCIE1A);
        Serial.println("Inside GreenIN");
        greenDuration = getUserInput();
        Serial.print("This is the GreenTime: ");
        Serial.println(greenDuration);

        while (true) {
          char confirmKey = keypad.getKey();
          if (confirmKey == '#') {
            if (redDuration == 0) {
              currentState = RedIN;  // Must set red first
            }
            else if (!greenConfigured && redConfigured) {
              currentState = InputComplete;
            }
            else {
              currentState = GreenSolid;
              tempCounter = 0;
            }
            break;
          }
          else if (confirmKey == 'B') {
            currentState = GreenIN;
            break;
          }
        }
        TIMSK1 |= (1 << OCIE1A);
        break;

      case InputComplete:
        TIMSK1 &= ~(1 << OCIE1A);
        setupComplete = true;
        Serial.println("Waiting for '*' to continue");
        while (true) {
          char startKey = keypad.getKey();
          if (startKey == '*') {
            currentState = RedSolid;
            greenConfigured = true;
            redConfigured   = true;
            tempCounter     = 0;
            break;
          }
          else if (startKey == 'A') {
            currentState = RedIN;
            break;
          }
          else if (startKey == 'B') {
            currentState = GreenIN;
            break;
          }
        }
        TIMSK1 |= (1 << OCIE1A);
        break;

      case RedSolid:
        digitalWrite(redLED, HIGH);
        tempCounter++;
        Serial.println("Counting Red");

        if (tempCounter == (redDuration - 3)) {
          Serial.println("Transitioning to blinking");
          digitalWrite(buzzerPin, HIGH);
          tempCounter = 0;
          activeLED = redLED;

          oneSecTick = false;
          TIMSK1 &= ~(1 << OCIE1A);  // Switch to 0.5s timer
          TIMSK1 |= (1 << OCIE1B);
          OCR1B = 7812;  // 0.5 second interval
        }
        break;

      case GreenSolid:
        digitalWrite(greenLED, HIGH);
        tempCounter++;
        Serial.println("Counting Green");

        if (tempCounter == (greenDuration - 3)) {
          Serial.println("Transitioning to blinking");
          digitalWrite(buzzerPin, HIGH);
          tempCounter = 0;
          activeLED = greenLED;

          oneSecTick = false;
          TIMSK1 &= ~(1 << OCIE1A);
          TIMSK1 |= (1 << OCIE1B);
          OCR1B = 7812;
        }
        break;

      case YellowPhase:
        digitalWrite(yellowLED, HIGH);
        digitalWrite(buzzerPin, LOW);
        activeLED = yellowLED;
        tempCounter = 0;

        TIMSK1 &= ~(1 << OCIE1A);
        TIMSK1 |= (1 << OCIE1B);
        OCR1B = 7812;
        break;

      default:  // Initial state
        if (initialBlink) {
          digitalWrite(redLED, HIGH);
          initialBlink = false;
          Serial.println("ON");
        }
        else {
          digitalWrite(redLED, LOW);
          initialBlink = true;
          Serial.println("OFF");
          Serial.println("completed Initial round");
        }
        break;
    }
  }

  // === 0.5-SECOND BLINK TIMER ===
  if (halfSecTick) {
    halfSecTick = false;
    Serial.println("Entered second timer");
    OCR1A = 7812;  // Reference for timing

    if (tempCounter < 6) {  // 6 × 0.5s = 3 seconds of blinking
      if (activeLED != yellowLED) {
        // Red or Green: blink LED + buzzer together
        if (!blinkToggle) {
          digitalWrite(activeLED, LOW);
          digitalWrite(buzzerPin, LOW);
        }
        else {
          digitalWrite(activeLED, HIGH);
          digitalWrite(buzzerPin, HIGH);
        }
      }
      else {
        // Yellow: only buzzer blinks
        if (!blinkToggle) {
          digitalWrite(buzzerPin, LOW);
        }
        else {
          digitalWrite(buzzerPin, HIGH);
        }
      }
      blinkToggle = !blinkToggle;
      tempCounter++;
      Serial.println("Second Counter");
    }
    else {
      // End of 3-second blink
      tempCounter = 0;
      Serial.println("Switching Colors");

      digitalWrite(buzzerPin, LOW);
      digitalWrite(activeLED, LOW);

      if (activeLED == redLED) {
        currentState = GreenSolid;
      }
      else if (activeLED == greenLED) {
        currentState = YellowPhase;
      }
      else {
        currentState = RedSolid;  // After yellow → back to red
      }

      OCR1A = 15624;              // Restore 1-second timing
      TIMSK1 &= ~(1 << OCIE1B);   // Disable 0.5s timer
      TIMSK1 |= (1 << OCIE1A);    // Re-enable 1s timer
    }
  }
}
