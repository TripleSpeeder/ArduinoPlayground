#include "LedControl.h"
#include <Bounce2.h>
#include "pitches.h"

/*
 Setup LedControl to work with.
 ***** These pin numbers will probably not work with your hardware *****
 pin 12 is connected to the DataIn 
 pin 11 is connected to LOAD(CS)
 pin 10 is connected to the CLK 
 We have only a single MAX72XX.
 */
LedControl lc=LedControl(12,10,11,1);

// Joystick
const int JoyBtn_pin = 2; // digital pin connected to switch output
const int JoyX_pin = 0; // analog pin connected to X output
const int JoyY_pin = 1; // analog pin connected to Y output
const Bounce joyBtn = Bounce();

// Buzzer
const int Buzzer_pin = 8;

// Position
typedef struct
{
  float x;
  float y;
} Position;

// Player
typedef struct
{
  Position position;
  unsigned int score; 
  float speed; // pixel per millisecond
} Player;
Player player;
const float MAX_PLAYER_SPEED = 0.01; // px per millisecond

// a Bullet
typedef struct 
{
  Position position;
  bool active;
  float speed; // pixel per millisecond
} Bullet;
const int MAX_BULLETS = 6;
Bullet bullets[MAX_BULLETS];

// store current inputs
Position newPosition;
int moveX;
bool newBtnState; // true -> pressed, false -> not pressed

// Refresh interval at which to set our game loop 
const int refreshInterval  = 100; // Redraw 10 times per second
// When was the last update call?
unsigned long lastUpdateTime;
// When was the last render call?
unsigned long lastRenderTime;

void setup() {
  Serial.begin(57600);
  /*
   The MAX72XX is in power-saving mode on startup,
   we have to do a wakeup call
   */
  lc.shutdown(0,false);
  /* Set the brightness (0 - 15) */
  lc.setIntensity(0,8);
  /* and clear the display */
  lc.clearDisplay(0);
  
  // Setup debouncer
  joyBtn.attach(JoyBtn_pin, INPUT_PULLUP);
  joyBtn.interval(5);

  // Setup player
  player.speed = 0.001;

  // if analog input pin 4 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(4));

  // initialize game loop
  lastUpdateTime = millis();
  lastRenderTime = lastUpdateTime;
}

void updateGame(unsigned long elapsed) {
  updatePlayerPosition(elapsed);
  updateBulletPosition(elapsed);
}

void updatePlayerPosition(unsigned long elapsed)
{
  // calculate moveSpeed per millisecond
  float deltaX = MAX_PLAYER_SPEED/512.0 * moveX;
  // multiply with elapsed milliseconds to get final delta
  deltaX *= elapsed;
  player.position.x = constrain(player.position.x + deltaX, 0.0, 7.0);
}

void updateBulletPosition(unsigned long elapsed)
{
  for (int i=0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
      float deltaY = bullets[i].speed * elapsed;
      bullets[i].position.y += deltaY;
      if (bullets[i].position.y > 7) {
        // bullet left screen
        bullets[i].active = false;
      }
    }
  }
}

void draw()
{
  lc.clearDisplay(0);

 /* Set the status of a single Led.
  * Params :
  *   addr  address of the display
  *   row   the row of the Led (0..7)
  *   col   the column of the Led (0..7)
  *   state If true the led is switched on, if false it is switched off 
  *   
  *   void setLed(int addr, int row, int col, boolean state); 
  */ 
  lc.setLed(0, round(player.position.x), round(player.position.y), true);

  for (int i=0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
      lc.setLed(0, round(bullets[i].position.x), round(bullets[i].position.y), true);
    }
  }
}

void checkInputs() {
  
  // update debouncer
  joyBtn.update();

  // Any button pressed?
  newBtnState = joyBtn.fell();
  
  // vertical position, mapped to 8x8 matrix
  // newPosition.y = analogRead(JoyY_pin)/128;
  moveX = analogRead(JoyX_pin) - 512;
}

void checkActions() {
  if (newBtnState) {
    int freeBulletIndex = -1;
    // player wants to fire bullet. Check if there is a slot available.
    for (int i = 0; i < MAX_BULLETS; i++) {
      if (!bullets[i].active){
        freeBulletIndex = i;
        break;
      }
    }
    if (freeBulletIndex > -1) {
      // launch new bullet;
      float speedRandomizer = random(0,3)/1000.0;
      bullets[freeBulletIndex].active = true;
      bullets[freeBulletIndex].speed = 0.002 + speedRandomizer; // px per millisecond
      bullets[freeBulletIndex].position.x = player.position.x;
      bullets[freeBulletIndex].position.y = player.position.y + 1;
      tone(Buzzer_pin, NOTE_A5, 50);
    }
  }
}

void loop() {
  unsigned long now = millis();
  unsigned long millisSinceLastUpdate = now - lastUpdateTime;
  unsigned long millisSinceLastRender = now - lastRenderTime;
  checkInputs();
  checkActions();
  updateGame(millisSinceLastUpdate);
  if (millisSinceLastRender < refreshInterval)
  {
    draw();
    lastRenderTime = now;
  }
  lastUpdateTime = now;
}
