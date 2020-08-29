
// Midi Control Panel 
// * * *******  12/08/2020
//
//

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MIDIUSB.h>
// #include <ArduinoDebugger.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

const byte oledAddress = 0x3C; // I2C Adresss OLED display

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Define constant array for analog pins that are directly read in
// A0 is Master Lvl Slider 
// A1 - A7 are Track Lvl Slider Tracks 1 thru 7
// A8 - A10 are Rotary Pots for Pan Master and Pan Track 1 & 2
const byte analogPins[] = {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10};
// Define constant array for Analog controls to MIDI controller mapping, we need 11 controllers
// Not entirely following the MIDI spec here, could also calculate these values but might change them later
const byte analogControllers[] = {20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30};

// Define constant array for Multiplexer (connected to pin A11)
// 0 - 4 are  Rotary Pots for Pan Track 3 thru 7
// 5 - 12 are  Rotary Pots for Functions 1 thru 8
const byte multiplexPins[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
const byte mutliplexAnalogInput = A11;
// Define constant array for Multiplexer controls to MIDI controller mapping, we need 13 controllers
const byte multiplexControllers[] = {31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43};

// Define constant array for Digital IO Pins
// 46 - 53 are switches for track muting, 46 is master, 47 - 53 are tracks 1 thru 7
// 39 - 45 are switches for record arming tracks 1 - 7 (spare switch under Master track is for arduino reset function)
// 31 - 36 are switches for control - back, forward, play, pause, record, stop
const byte digitalPins[] = {46, 47, 48, 49, 50, 51, 52, 53, 39, 40, 41, 42, 43, 44, 45, 31, 32, 33, 34, 35, 36};

// Define constant array for button controls to MIDI controller mapping, we need 21 controllers
const byte digitalControllers[] = {44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64};

// define constant array for multiplexer control pins, MSB is last down to LSB (19 is MSB)
const byte multiplexControl[] = {16, 17, 18, 19};

// define the default midi channel to send on
const byte defaultMidichannel = 1;

// define var array for analog pins lastvalue
int analogLastvalue[sizeof(analogPins)];

// define var array for multiplexer pins lastvalue
int multiplexLastvalue[sizeof(multiplexPins)];

// define var array for digital pins lastvalue
int digitalLastvalue[sizeof(digitalPins)];

// define an array to hold the toggles for switches
byte digitalControllerToggle[sizeof(digitalPins)];

// Read input value
int inputVal = 0;

void setup() {
  byte index=0;
  
  // Setup Environment
    Serial.begin(9600);
    // Serial.println("Starting Program Init");

    // Setup input ports using the internal Pullup Resistors and initialize lastvalue for digital pins/toggle

    for (index = 0; index < sizeof(digitalPins); index++) {
      pinMode(digitalPins[index], INPUT_PULLUP);
      digitalLastvalue[index]=HIGH;
      digitalControllerToggle[index]=0;
    }

    // Setup output ports for multiplexer and set low
    for (index = 0; index < sizeof(multiplexControl); index++) {
      pinMode(multiplexControl[index], OUTPUT);
      digitalWrite(multiplexControl[index],LOW);
    }   

    // Initialize the analog lastvalue array - using a value that is outside 0-1023 ensures first reading is always sent over midi
    for (index = 0; index < sizeof(analogPins); index++) {
      analogLastvalue[index]=1500;
    }

    // Initialize the multiplexer lastvalue array
    for (index = 0; index < sizeof(multiplexPins); index++) {
      multiplexLastvalue[index]=1500;
    }

    // Initialize Display
    
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    // Serial.println("Starting Display");
    if(!display.begin(SSD1306_SWITCHCAPVCC, oledAddress)) {
      Serial.println(F("SSD1306 allocation failed"));
    
    }
    // Show initial display buffer contents on the screen --
    // the library initializes this with an Adafruit splash screen.
    display.display();
    outputdata ("USB Midi Controller","READY for use","Arduino Due/V1.0", "YOUR NAME HERE");
    delay(1000); // Pause for 1 second
    // Serial.println("Ended Program Init");
}

// Output data to the display 
void outputdata (String line1, String line2, String line3, String line4) {
    display.clearDisplay(); 
    display.setTextSize(1);             // Normal 1:1 pixel scale
    display.setCursor(0,0);             // Start at top-left corner
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
    display.println("---------------------");
    display.setTextColor(SSD1306_WHITE);        // Draw white text
    display.println(line1);
    display.println(line2);
    display.println(line3);
    display.println(line4); 
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
    display.println("---------------------");
    display.display();  
}

// Prepare and send MIDI CC data
// Params are the Midi controller number, Value to send and an outputText for debugging on display
void send_midi_data (byte controller, int value, String outputText) {
    
    midiEventPacket_t midiData;
    char debugLine[100];

    // Build Midi CC command
    // First parameter is the event type (0x0B = control change).
    // Second parameter is the event type, combined with the channel.
    // Third parameter is the control number number (0-119).
    // Fourth parameter is the control value (0-127).

    midiData= {0x0B, 0xB0 | defaultMidichannel, lowByte(controller), lowByte(value)};
    MidiUSB.sendMIDI(midiData);
    
    // Put this on the display for debugging
    display.clearDisplay(); 
    display.setTextSize(1);             // Normal 1:1 pixel scale
    display.setCursor(0,0);             // Start at top-left corner
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
    display.println("---------------------");
    display.setTextColor(SSD1306_WHITE);        // Draw white text
    display.println("Midi Event Sent");
    display.println(outputText);
    sprintf(debugLine, "Channel: %u",defaultMidichannel);
    display.println(debugLine);
        sprintf(debugLine, "Controller: %u",controller);
    display.println(debugLine);
    sprintf(debugLine, "Value: %u",value);
    display.println(debugLine);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
    display.println("---------------------");
    display.display(); 
   
}

void loop() {
  byte index=0;
  byte pinIndex=0;
  char debugLine[100];
  
  // Read the analog values in and do midi output on any (significant) change - use 8 here because Midi is 127 values and the Arduino pots are at 1024 resolution
  for (index = 0; index < sizeof(analogPins); index++) {
      inputVal = analogRead(analogPins[index]);
      // small pause for ADC selection and read again
      delay (5);
      inputVal = analogRead(analogPins[index]);
      // Serial.print ("Analog ");
      // Serial.print (analogPins[index]);
      // Serial.println (inputVal);
      if (inputVal-8 > analogLastvalue[index] || inputVal+8 < analogLastvalue[index]) {
        // Serial.print("Rotary Change detected values as follows : Pin - Value - Last Value");
        // Serial.println(analogPins[index]);
        // Serial.println(inputVal);
        // Serial.println(analogLastvalue[index]);
        // set last value now we found a change
        analogLastvalue[index] = inputVal;
        // And now generate a MIDI event
        // Have to convert from 0-1023 to 0-127 for Midi velocity - factor of 8
        sprintf(debugLine, "Analog Move %u",index);
        send_midi_data (analogControllers[index], inputVal/8, debugLine);                
      }
  }

  // Read the analog values in from the multiplexer and do midi output on any (significant) change - use 8 here because Midi is 127 values and the Arduino pots are at 1024 resolution

  for (index = 0; index < sizeof(multiplexPins); index++) {
      // Select multiplexer address here
      // Set using the pins in multiplexControl[], LSB first
      for (pinIndex = 0; pinIndex < sizeof(multiplexControl); pinIndex++) {
        // Serial.print("Calc pin for multiplexControl[pinIndex], index, pinIndex,bitRead (index, pinIndex)");
        // Serial.print (multiplexControl[pinIndex]);
        // Serial.print (index);
        // Serial.print (pinIndex);
        // Serial.println (bitRead (index, pinIndex));
        digitalWrite (multiplexControl[pinIndex], bitRead (index, pinIndex)); 
      }
      // Read the multiplex adress
      inputVal = analogRead(mutliplexAnalogInput);
      // small pause for ADC selection and read again
      delay (5);
      inputVal = analogRead(mutliplexAnalogInput);
      if (inputVal-8 > multiplexLastvalue[index] || inputVal+8 < multiplexLastvalue[index]) {
        // Serial.println("Multiplex Rotary Change detected values as follows : Pin - Value - Last Value");
        // Serial.println("PIN");
        // Serial.println(multiplexPins[index]);
        // Serial.println("Value");
        // Serial.println(inputVal);
        // Serial.println("Last Value");
        // Serial.println(multiplexLastvalue[index]);
        // set last value now we found a change
        multiplexLastvalue[index] = inputVal;
        // And now generate a MIDI event
        // Have to convert from 0-1023 to 0-127 for Midi velocity - factor of 8
        sprintf(debugLine, "MultiPlx Move %u",index);
        send_midi_data (multiplexControllers[index], inputVal/8, debugLine);        
      }
  }

  // Read the digital switches in 
  for (index = 0; index < sizeof(digitalPins); index++) {
      inputVal = digitalRead(digitalPins[index]);
      // Not doing any debounce detection due to the size of the analog loop above it seems unlikely to be needed
      if (inputVal != digitalLastvalue[index]) {
        // Serial.print("Digital detected values as follows : Pin - Value - Last Value");
        // Serial.print(digitalPins[index]);
        // Serial.print(inputVal);
        // Serial.println(digitalLastvalue[index]);
        // set last value now we found a change
        digitalLastvalue[index] = inputVal;
        // and now generate a MIDI event *IF* this is a button press (ie pin changed to LOW)
        if (inputVal == LOW) {
            // Generate Midi Event
            // Serial.println("Generating Midi Digital");          
            sprintf(debugLine, "DigitalPress %u",index);
            // velocity needs to be 0 or 127 based on what was last sent (to toggle a button in reaper)
            if (digitalControllerToggle[index]==0) {
              digitalControllerToggle[index]=127;
            } else {
              digitalControllerToggle[index]=0;
            }
            send_midi_data (digitalControllers[index], digitalControllerToggle[index], debugLine);
        }
      }
  }  
// Serial.println ("looping now");
  
}
