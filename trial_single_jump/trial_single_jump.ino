 
#include <LiquidCrystal.h>

LiquidCrystal   lcd(12, 11, 7, 6, 5, 4); // RS, E, D4, D5, D6, D7

volatile int time_slice = 0; // Keeps track of elapsed milliseconds

int button1Cursor = 0; //keeps track of cursor position for top animation
bool cycleComplete = false;
// button 1 pressed
volatile int button1Downflag = 0;

int idleTimer = 0;  // Moved out of volatile since it's only used in loop()
int button1DownLastTime_slice = 0; // Stores last recorded time
bool frogStanding = false; // Toggles between alternating idle animations

// button 1 released
volatile int button1Upflag = 0;
int jumpTimer = 0;  // Moved out of volatile since it's only used in loop()
int button1UpLastTime_slice = 0; // Stores last recorded time
int button1UpFrameState = 0; // Used for incrementing jump frames
bool button1UpToggleState = false;

bool frog1Jumping = false;

const int button1 = 8;

void setup() {

  lcd.begin(16,   2);

  attachInterrupt(digitalPinToInterrupt(2), button1ISR, CHANGE);

  cli(); // Disable interrupts

  // Timer Setup (CTC Mode with 64 Prescaler for 1ms resolution)
  TCCR0A = 0;
  TCCR0B = 0;
  OCR0A = 249; // Fires every 1ms

  // Set CTC mode
  TCCR0A |= _BV(WGM01);

  // Set clock prescaler to 64 (CS01 and CS00)
  TCCR0B |= _BV(CS01) | _BV(CS00);

  // Enable Timer Compare Match A Interrupt
  TIMSK0 |= _BV(OCIE0A);

  sei(); // Enable interrupts

  pinMode(button1, INPUT); // We have external pulldown setup
}

void loop() {
  
    // button 1 pressed
    if (button1Downflag == 1) {
      button1Upflag = 0; 
        if (time_slice != button1DownLastTime_slice) {  // Ensures we only increment once per timer tick
            idleTimer++;
            button1DownLastTime_slice = time_slice;
        }

      // Reset flagtimer when button is pressed
      if (idleTimer >= 500) {
        if (frogStanding) {
            idleDownTop0(button1Cursor);
        } else {
            idleUpTop0(button1Cursor);
        }
        //frogStanding = !frogStanding; // Flip the toggle state
        idleTimer = 0;
      }
    }
  
  // button 1 released
      if (button1Upflag == 1) {
        
        button1Downflag = 0; // set to false manually so no conflict
        // check current idle animation frame
        // if frogStanding false,then jump
        if (frogStanding == false){
          // to avoid interrupt
          frog1Jumping = true;
          // we can jump-> start frame timer
          if (time_slice != button1UpLastTime_slice) {  // Ensures we only increment once per timer tick
            jumpTimer++;
            button1UpLastTime_slice = time_slice;
          }

          if (jumpTimer >= 100 && !cycleComplete) { // Run through jump sequence
              switch (button1UpFrameState) {
                  case 0: jump1Top0(button1Cursor); break;
                  case 1: jump2Top0(button1Cursor); break;
                  case 2: jump3Top0(button1Cursor); break;
                  case 3: jump4Top0(button1Cursor); break;
                  case 4: jump5Top0(button1Cursor); break;
                  case 5: jump6Top0(button1Cursor); break;
                  case 6: jump7Top0(button1Cursor); break;
                  case 7: idleUpTop1(button1Cursor); cycleComplete = true; break;
              }

              button1UpFrameState++;   // Increment the frame state
              jumpTimer = 0;           // Reset jump timer
          } 
        }
        else {

          byte image01[8] = {B00000, B00000, B00000, B00000, B01111, B11101, B11111, B11010};


          lcd.createChar(0, image01);


          lcd.setCursor(button1Cursor, 0);
          lcd.write(byte(0));
        }
        if (cycleComplete){ // if we didn't release at the right time
          //button1Downflag = 0; // set to false manually so no conflict
		      button1UpFrameState = 0; 
          button1Cursor++;
          cycleComplete = false;
          frogStanding = true;
        }
        
    }

    // Win condition
    if (button1Cursor == 15) {
      // crown the frog!
      lcd.clear();
      winner(1);
    }


}

void button1ISR() {
    if (digitalRead(button1) == HIGH && frog1jumping == false) {
        button1Downflag = 1; // Set flag when button is pressed
        button1Upflag = 0;
    }

    if (digitalRead(button1) == LOW && frog1jumping == false) {
        button1Upflag = 1; // Set flag when button is pressed
        button1Downflag = 0; 
    }
}

// Timer ISR (1ms per tick)
ISR(TIMER0_COMPA_vect) {
    time_slice++;  // Lightweight ISR, only updates a single variable
}

void idleDownTop0(int button1Cursor) {
lcd.clear();

byte image01[8] = {B00000, B00000, B00000, B00000, B00000, B01111, B11101, B11111};


lcd.createChar(0, image01);


lcd.setCursor(button1Cursor, 0);
lcd.write(byte(0));
frogStanding = false;
}

void idleUpTop0(int button1Cursor) {
lcd.clear();

byte image01[8] = {B00000, B00000, B00000, B00000, B01111, B11101, B11111, B11010};


lcd.createChar(0, image01);


lcd.setCursor(button1Cursor, 0);
lcd.write(byte(0));
frogStanding = true;

}

void jump1Top0(int button1Cursor) {
lcd.clear();

byte image01[8] = {B00000, B00000, B00000, B01111, B11101, B11111, B10010, B01000};


lcd.createChar(0, image01);


lcd.setCursor(button1Cursor, 0);
lcd.write(byte(0));

}

void jump2Top0(int button1Cursor) {
lcd.clear();

byte image01[8] = {B00000, B00000, B00111, B01110, B01111, B01001, B10000, B00000};


lcd.createChar(0, image01);


lcd.setCursor(button1Cursor, 0);
lcd.write(byte(0));

}

void jump3Top0(int button1Cursor) {
lcd.clear();

byte image01[8] = {B00000, B00000, B00011, B00111, B00111, B01100, B00000, B00000};
byte image02[8] = {B00000, B00000, B10000, B10000, B10000, B00000, B00000, B00000};


lcd.createChar(0, image01);
lcd.createChar(1, image02);


lcd.setCursor(button1Cursor, 0);
lcd.write(byte(0));
lcd.setCursor(button1Cursor + 1, 0);
lcd.write(byte(1));

}

void jump4Top0(int button1Cursor) {
lcd.clear();

byte image01[8] = {B00000, B00000, B00001, B00011, B00011, B00010, B00100, B00000};
byte image02[8] = {B00000, B00000, B11000, B01000, B11000, B10000, B00000, B00000};


lcd.createChar(0, image01);
lcd.createChar(1, image02);


lcd.setCursor(button1Cursor, 0);
lcd.write(byte(0));
lcd.setCursor(button1Cursor + 1, 0);
lcd.write(byte(1));

}

void jump5Top0(int button1Cursor) {
lcd.clear();

byte image01[8] = {B00000, B00000, B00000, B00000, B00001, B00001, B00001, B00010};
byte image02[8] = {B00000, B00000, B00000, B11100, B10100, B11100, B01000, B00000};


lcd.createChar(0, image01);
lcd.createChar(1, image02);


lcd.setCursor(button1Cursor, 0);
lcd.write(byte(0));
lcd.setCursor(button1Cursor + 1, 0);
lcd.write(byte(1));

}

void jump6Top0(int button1Cursor) {
lcd.clear();

byte image02[8] = {B00000, B00000, B00000, B11110, B11010, B11110, B00100, B00000};


lcd.createChar(0, image02);


lcd.setCursor(button1Cursor + 1, 0);
lcd.write(byte(0));

}

void jump7Top0(int button1Cursor) {
lcd.clear();

byte image02[8] = {B00000, B00000, B00000, B01111, B11101, B11111, B10010, B01000};


lcd.createChar(0, image02);


lcd.setCursor(button1Cursor + 1, 0);
lcd.write(byte(0));

}

void idleUpTop1(int button1Cursor) {
lcd.clear();

byte image02[8] = {B00000, B00000, B00000, B00000, B11111, B11101, B11111, B11010};


lcd.createChar(0, image02);


lcd.setCursor(button1Cursor + 1, 0);
lcd.write(byte(0));

}

void idleDownTop1(int button1Cursor) {
lcd.clear();

byte image02[8] = {B00000, B00000, B00000, B00000, B00000, B11111, B11101, B11111};


lcd.createChar(0, image02);


lcd.setCursor(1, 0);
lcd.write(byte(0));

}

void winner(int player) {
  
  byte image02[8] = {B10101, B11111, B11111, B00000, B01111, B11101, B11111, B11010};
  
  
  lcd.createChar(0, image02);
  
  
  lcd.setCursor(15, player-1);
  lcd.write(byte(0));
  
  }
