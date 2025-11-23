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

const int button_pin = 2;
static unsigned buttonState = 0; //0 means cw, 1 means cc (change if needed)
String fan_direction = "clock-wise";

//Sound sensor pin placement
const int sound_sensor = A2; 
int soundValue = 0;

//Timer pins from reference
const int SDA = A4, SCL = A5;
volatile unsigned long secondsSinceStart = 0;
DS3231  rtc(SDA, SCL);

//Motor driver pins from reference
const int enA = 9, in1 = 8, in2 = 7;

//LCD Screen pin placements from reference
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//Global variable to track current fan speed for display
volatile int current_fan_speed = 0;
volatile DateTime now;

void setup(){
    Serial.begin(9600);
    Serial.println("Program Started");

    // initialize the pins
    pinMode(button_pin, INPUT);
    pinMode(in1, OUTPUT);
    pinMode(in2, OUTPUT);
    pinMode(enA, OUTPUT);

    // Get current time
    now = rtc.now();

    // initialize LCD screen
    lcd.begin(16, 2);
    lcd.print("Initializing...");

    //Setting up interrupt from LAB1
    cli();//stop interrupts

    //set timer1 interrupt at 1Hz (every second)
    TCCR1A = 0;// set entire TCCR1A register to 0
    TCCR1B = 0;// same for TCCR1B
    TCNT1  = 0;//initialize counter value to 0
    // set compare match register for 1hz increments
    OCR1A = 15624;// = (16*10^6) / (1*1024) - 1 (must be <65536)
    // turn on CTC mode
    TCCR1B |= (1 << WGM12);
    // Set CS12 and CS10 bits for 1024 prescaler
    TCCR1B |= (1 << CS12) | (1 << CS10);  
    // enable timer compare interrupt
    TIMSK1 |= (1 << OCIE1A);

    sei();//allow interrupts

}

void loop(){
    //Change the direction of fan with button press
    //Set up debounce if needed
    buttonState = digitalRead(button_pin);
    if (buttonState == HIGH){
        //Set the direction of the fan to counter clockwise
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
    } else {
        //Set the direction of the fan to clockwise
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
    }

    //Change speed of fan based on sound input
    soundValue = analogRead(sound_sensor);  //read the sound sensor
    change_fan_speed(soundValue);
    
    soundValue >>= 5; //bitshift operation 
    Serial.println(soundValue); //print the value of sound sensor

    delay(100);
}

//Timer interrupt for incrementing LCD display
ISR(TIMER1_COMPA_vect){
    //Increment the time by a second
    now = now + 1;
    secondsSinceStart++;

    //Update the LCD display
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Speed: ");
    lcd.print(current_fan_speed);
    lcd.print("%");
    
    lcd.setCursor(0, 1);
    lcd.print("Time: ");
    lcd.print(secondsSinceStart);
    lcd.print("s");
}

//Fan speed controller
void change_fan_speed(int sound_value){ //Speeds {Full, 3/4, 1/2, or 0}, analog input is out of 255
    int fan_speed = 0;
    if (sound_value >= 700){ // at higher sound values fan speed with be at full
        
        fan_speed = 255;
    }
    else if(sound_value >= 500){ //Fan speed: 3/4

        fan_speed = 191;
    }
    else if(sound_value >= 300){ // Fan speed: 1/2

        fan_speed = 127;
    }
    else{ //Fan speed: 0
        
        fan_speed = 0;
    }
    analogWrite(enA, fan_speed);
    Serial.print("Fan speed: ");
    Serial.print(fan_speed * 100 / 255);
    Serial.println("%");
}