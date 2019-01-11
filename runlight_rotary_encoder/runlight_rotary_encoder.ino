//www.elegoo.com
//2016.12.9 

#include <MD_REncoder.h>
#include <Bounce2.h>

const int latchPin = 11;      // (11) ST_CP [RCK] on 74HC595
const int clockPin = 9;      // (9) SH_CP [SCK] on 74HC595
const int dataPin = 12;     // (12) DS [S1] on 74HC595
const int numLEDs=8;

// For the rotary encoder
const int PinCLK=2;   // Generating interrupts using CLK signal
const int PinDT=3;    // Reading DT signal
const int PinSW=4;    // Reading Push Button switch

volatile boolean turnDetected;
volatile boolean rotationDirection; // True -> clockwise, False -> counterclockwise
unsigned int lightWidth = 1;

byte leds = 0;
long tDelay = 100;
unsigned int position = 0;

// set up encoder object
const MD_REncoder RotaryEncoder = MD_REncoder(PinCLK, PinDT);
// debouncer for the encoder button
const Bounce RotaryButton = Bounce();


// Interrupt routine runs if CLK or DT change
void isr ()  {
  uint8_t x = RotaryEncoder.read();
  if (x) {
    turnDetected = true;
    rotationDirection = (x == DIR_CW);
  }
}

void updateShiftRegister()
{
   digitalWrite(latchPin, LOW);
   shiftOut(dataPin, clockPin, LSBFIRST, leds);
   digitalWrite(latchPin, HIGH);
}

void setup() 
{
  // init serial port
  Serial.begin(9600);

  // set up LED controller
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);

  // set up rotary encoder
  RotaryEncoder.begin();
  attachInterrupt (digitalPinToInterrupt(PinCLK),isr,CHANGE);
  attachInterrupt (digitalPinToInterrupt(PinDT),isr,CHANGE);
  RotaryButton.attach(PinSW, INPUT_PULLUP);
  RotaryButton.interval(5); // debounce interval in milliseconds
  
  // set initial LED state
  bitSet(leds, 0);
  updateShiftRegister();
}

void loop() 
{
  if (turnDetected) {
    turnDetected = false;
    if (rotationDirection) {
      position++;
      Serial.print("Clockwise turn, new pos: ");
    } else {
      position--;
      Serial.print("Counterclockwise turn, new pos: ");
    }
    Serial.print(position);
    Serial.print(" -> ");
    Serial.println(position%numLEDs);
    turnonPosition(position%numLEDs);
  }

  RotaryButton.update();
  if (RotaryButton.fell()){
    // rotary button pressed, increase lightWidth
    if (lightWidth >= (numLEDs-1)) {
      lightWidth = 1;
    } else {
      lightWidth++;
    }
    turnonPosition(position%numLEDs);
  }
}

void turnonPosition(int pos)
{
    // all off
    leds = 0;

    // LEDs adjacent to pos should be on, depending on the value of ligthWidth.
    int halfWidth = lightWidth/2; // rounding down...
    int remainder = lightWidth - (2*halfWidth); // Take care of odd lightWidth values
    for (int i=-halfWidth; i<halfWidth+remainder; i++){
      int currentIndex = pos+i;
      // wrap
      if (currentIndex < 0) {
        currentIndex += numLEDs;
      } else if (currentIndex >= numLEDs) {
        currentIndex -= numLEDs;
      }
      bitSet(leds, currentIndex);
    }

    updateShiftRegister();
}
