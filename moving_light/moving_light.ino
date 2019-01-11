#include "LedControl.h"
#include <Bounce2.h>

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

// a Bullet
typedef struct 
{
  Position position;
  bool active;
  float speed; // pixel per millisecond
} Bullet;
const int numBullets = 8;
Bullet bullets[numBullets];

// store current inputs
Position newPosition;
bool newBtnState; // true -> pressed, false -> not pressed

// Refresh interval at which to set our game loop 
// To avoid having the game run at different speeds depending on hardware
const int refreshInterval  = 100; // Redraw 10 times per second
// Used to calculate the delta between loops for a steady frame-rate
unsigned long lastRefreshTime;

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

  // initialize game loop
  lastRefreshTime = millis();
}

void updateGame(unsigned long elapsed) {
  updatePlayerPosition(elapsed);
  updateBulletPosition(elapsed);
}

void updatePlayerPosition(unsigned long elapsed)
{
  if ((newPosition.x != player.position.x) || (newPosition.y != player.position.y)) {
    player.position.x = newPosition.x;
    player.position.y = 0; // instead of newY, as i want the player to stick to the bottom of the screen
  }
}

void updateBulletPosition(unsigned long elapsed)
{
  for (int i=0; i < numBullets; i++) {
    if (bullets[i].active) {
      float deltaY = bullets[i].speed * elapsed;
      bullets[i].position.y += deltaY;
      // Serial.println(bullets[i].position.y);
      if (bullets[i].position.y > 7) {
        // bullet left screen
        bullets[i].active = false;
      }
    }
  }
}

void drawPosition()
{
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

  for (int i=0; i < numBullets; i++) {
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
  
  // Joystick input, mapped to 8x8 matrix
  newPosition.x = analogRead(JoyX_pin)/128;
  newPosition.y = analogRead(JoyY_pin)/128;
  
}

void checkActions() {
  if (newBtnState) {
    // player pressed button. Check current X position
    int column = round(player.position.x);
    
    if (bullets[column].active) {
      // cancel current bullet
      bullets[column].active = false;
    } else {
      // launch new bullet;
      bullets[column].active = true;
      bullets[column].speed = 0.002; // px per millisecond
      bullets[column].position.x = player.position.x;
      bullets[column].position.y = player.position.y + 1;
    }
  }
}

void loop() {
  unsigned long now = millis();
  unsigned long elapsed = now - lastRefreshTime; // usually 3-4 ms
  checkInputs();
  checkActions();
  updateGame(elapsed);
  lc.clearDisplay(0);
  drawPosition();
  lastRefreshTime = now;
}
