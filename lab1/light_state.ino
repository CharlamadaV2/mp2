#include <Keypad.h> 

extern "C" // function prototypes external to C sketch
{
    void start();
    void red_led(byte);
    void green_led(byte);
    void yellow_led(byte);
    void buzzer(byte);
}

const byte ROWS = 4; //four rows 
const byte COLS = 4; //four columns 

//keypad configuration
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {28, 26, 24, 22}; //connect to the row pinouts of the keypad 
byte colPins[COLS] = {29, 27, 25, 23}; //connect to the column pinouts of the keypad 
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// State variables
bool red_configured = false;
bool green_configured = false;
uint16_t red_seconds = 0;
uint16_t green_seconds = 0;
char input_buffer[10];
uint8_t input_index = 0;

void setup(){
    start();
    // start function will initialize all the pins

    cli(); // Disable global interrupts
  
    // Configure Timer1 for a 1Hz interrupt
    TCCR1A = 0; // Clear the TCCR1A register
    TCCR1B = 0; // Clear the TCCR1B register
    TCNT1  = 0; // Initialize the counter to 0
    
    // Set compare match register for 1Hz increments
    // OCR1A = (16,000,000 / (1 * 1024)) - 1 = 7812 (with 1024 prescaler)
    OCR1A = 7812; // Set the value for .5Hz
    
    // Enable CTC (Clear Timer on Compare Match) mode
    TCCR1B |= (1 << WGM12);
    
    // Set CS12 and CS10 bits for a 1024 prescaler
    TCCR1B |= (1 << CS12) | (1 << CS10);
    
    // Enable Timer1 compare match interrupt
    TIMSK1 |= (1 << OCIE1A);
    
    sei(); // Enable global interrupts

    while (!red_configured || !green_configured || !check_start()){
    // Flash red LED (1s on/off) until both LEDs configured and * pressed
        red_led(1);
        delay(1000);
        red_led(0);
        delay(1000);
        handle_keypad();
    }
}

//checks if ready to start
bool check_start() {
  char key = keypad.getKey();
  if (key == '*') {
    return red_configured && green_configured;
  }
  return false;
}

// process keypad inputs
void handle_keypad() {
    char key = keypad.getKey();
    if (!key) return;

    // Store key in buffer
    if (input_index < 9) {
        input_buffer[input_index++] = key;
        input_buffer[input_index] = '\0'; // Null-terminate
    }

  // Process input on '#' press
    if (key == '#') {
        process_input();
        input_index = 0; // Reset buffer
    }
}

// For implementation of Red (A) and Green (B): 
// On the 16 button keypad
// Press the respective letter, followed with the number of seconds, then # to configure.
void process_input() {
    if (input_index < 2) return; // Need at least letter + number

    char led_type = input_buffer[0];
    uint16_t seconds = 0;

    // Parse number (assuming single or double-digit)
    for (uint8_t i = 1; i < input_index - 1; i++) {
        if (input_buffer[i] >= '0' && input_buffer[i] <= '9') {
            seconds = seconds * 10 + (input_buffer[i] - '0');
        } else {
            return; // Invalid input
        }
    }

    // Assign seconds to red or green LED
    if (led_type == 'A' && seconds > 0) {
        red_seconds = seconds;
        red_configured = true;
        Serial.print("Red configured: ");
        Serial.println(red_seconds);
    } else if (led_type == 'B' && seconds > 0) {
        green_seconds = seconds;
        green_configured = true;
        Serial.print("Green configured: ");
        Serial.println(green_seconds);
    }
}

// Pressing will start the light operation

// For light function
// Red will go for s amount of time
// Then during last 3 seconds red light will blink 0.5s on and off
// Green light will turn on after red light, then will blink 0.5s
// Yellow light turns on for 3 s  after green light, then the pattern repeats

// During the last 3 second state change, the buzzer will go off before the light changes

// On start red light flash 1s on 1s off until, red and green light are configured and "*" is pressed.
void loop() {
    run_light_pattern(red_seconds, green_seconds);
}

// Modified run_light_pattern to include delays
void run_light_pattern(uint16_t red_secs, uint16_t green_secs) {
    // Red LED on
    red_led(1);
    delay(red_seconds * 1000 - 3000);
    red_led(0);
    blink_red();

    // Green LED on
    green_led(1);
    delay(green_seconds * 1000 - 3000);
    green_led(0);
    blink_green();

    // Buzzer before yellow
    buzzer(1);
    // Yellow LED on for 3s
    yellow_led(1);
    delay(3000);
    yellow_led(0);
    buzzer(0);

    //loops to red
}

//This will blink the red led for 3 seconds, with a 0.5 on/off
void blink_red(){
    buzzer(1);
    for (int count = 0; count < 3; count++){
        red_led(1);
        delay(500);
        red_led(0);
        delay(500);
    }
    buzzer(0);
}

//This will blink the green led for 3 seconds, with a 0.5 on/off
void blink_green(){
    buzzer(1);
    for (int count = 0; count < 3; count++){
        green_led(1);
        delay(500);
        green_led(0);
        delay(500);
    }
    buzzer(0);
}