 
#include <LiquidCrystal.h>

#define BUTTON1_PIN 8
#define BUTTON2_PIN 3

LiquidCrystal   lcd(12, 11, 7, 6, 5, 4); // RS, E, D4, D5, D6, D7

volatile int time_slice = 0; // Keeps track of elapsed milliseconds
volatile int button1state;
volatile int button2state;

int button1Cursor = 0; //keeps track of cursor position for top animation
int button2Cursor = 0;
bool cycle1Complete = false;
bool cycle2Complete = false;

int idle1Timer = 0;  // Moved out of volatile since it's only used in loop()
int button1DownLastTime_slice = 0; // Stores last recorded time

int idle2Timer = 0;  // Moved out of volatile since it's only used in loop()
int button2DownLastTime_slice = 0; // Stores last recorded time

bool frog1Standing = true; // Toggles between alternating idle animations
bool frog2Standing = true;

int jump1Timer = 0;  // Moved out of volatile since it's only used in loop()
int jump2Timer = 0;

int button1UpLastTime_slice = 0; // Stores last recorded time
int frog1JumpFrame = 0; // Used for incrementing jump frames
bool button1UpToggleState = false;

int button2UpLastTime_slice = 0; // Stores last recorded time
int frog2JumpFrame = 0; // Used for incrementing jump frames
bool button2UpToggleState = false;

bool frog1Jumping = false;
bool frog2Jumping =false;



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

	pinMode(BUTTON1_PIN, INPUT); // We have external pulldown setup
}

void loop() {
  
    // button 1 pressed
    if (button1state == 1) {
        if (time_slice != button1DownLastTime_slice) {  // Ensures we only increment once per timer tick
            idle1Timer++;
            button1DownLastTime_slice = time_slice;
        }

		// Reset flagtimer when button is pressed
		if (idle1Timer >= 500) {
			if (frog1Standing) {
				idleDownTop0(button1Cursor, 0);
			} else {
				idleUpTop0(button1Cursor, 0);
			}
			//frog1Standing = !frog1Standing; // Flip the toggle state
			idle1Timer = 0;
		}

    if (button2state == 1) {
        if (time_slice != button2DownLastTime_slice) {  // Ensures we only increment once per timer tick
            idle2Timer++;
            button2DownLastTime_slice = time_slice;
        }

		// Reset flagtimer when button is pressed
		if (idle2Timer >= 500) {
			if (frog2Standing) {
				idleDownTop0(button2Cursor, 1);
			} else {
				idleUpTop0(button2Cursor, 1);
			}
			//frog1Standing = !frog1Standing; // Flip the toggle state
			idle2Timer = 0;
		}
    }
    }
  
  // button 1 released
      if (button1state == 0) {
        
        // check current idle animation frame
        // if frog1Standing false,then jump
        if (frog1Standing == false){
			// to avoid interrupt
			frog1Jumping = true;
			frogJump(cycle1Complete, button1Cursor, jump1Timer, frog1JumpFrame, button1UpLastTime_slice);
        }
        else {
			frogFreezeStand(button1Cursor, 0);
        }
        if (cycle1Complete){ // if we didn't release at the right time
          //button1Downflag = 0; // set to false manually so no conflict
			frog1JumpFrame = 0; 
			button1Cursor++;
			cycle1Complete = false;
			frog1Standing = true;
			frog1Jumping = false;
        }
        
    }

	  // button 2 released
      if (button2state == 0) {
        
        // check current idle animation frame
        // if frog1Standing false,then jump
        if (frog2Standing == false){
			// to avoid interrupt
			frog2Jumping = true;
			frogJump(cycle2Complete, button2Cursor, jump2Timer, frog2JumpFrame, button2UpLastTime_slice);
        }
        else {
			frogFreezeStand(button2Cursor, 1);
        }
        if (cycle2Complete){ // if we didn't release at the right time
          //button1Downflag = 0; // set to false manually so no conflict
			frog2JumpFrame = 0; 
			button2Cursor++;
			cycle2Complete = false;
			frog2Standing = true;
			frog2Jumping = false;
        }
        
    }
    // Win condition
    if (button1Cursor == 15) {
		// crown the frog!
		winner(1);
    }
}

void button1ISR() {
    if (digitalRead(BUTTON1_PIN) == HIGH && frog1Jumping == false) {
        button1state = 1;
    }

    if (digitalRead(BUTTON1_PIN) == LOW && frog1Jumping == false) {
        button1state = 0;
    }
    
    if (digitalRead(BUTTON2_PIN) == HIGH && frog2Jumping == false) {
        button2state = 1;
    }

    if (digitalRead(BUTTON2_PIN) == LOW && frog2Jumping == false) {
        button2state = 0;
    }

}

// Timer ISR (1ms per tick)
ISR(TIMER0_COMPA_vect) {
    time_slice++;  // Lightweight ISR, only updates a single variable
}

void frogJump(bool &cycleComplete, int &buttonCursor, int &jumpTimer, int &frogJumpFrame, int &buttonUpLastTime_slice) {

	if (time_slice != buttonUpLastTime_slice) {  // Ensures we only increment once per timer tick
		jumpTimer++;
		buttonUpLastTime_slice = time_slice;
	}

	if (jumpTimer >= 80 && !cycleComplete) { // Run through jump sequence
		switch (frogJumpFrame) {
			case 0: jump1Top0(buttonCursor, 0); break;
			case 1: jump2Top0(buttonCursor, 0); break;
			case 2: jump3Top0(buttonCursor, 0); break;
			case 3: jump4Top0(buttonCursor, 0); break;
			case 4: jump5Top0(buttonCursor, 0); break;
			case 5: jump6Top0(buttonCursor, 0); break;
			case 6: jump7Top0(buttonCursor, 0); break;
			case 7: idleUpTop1(buttonCursor, 0); cycleComplete = true; break;
		}

		frogJumpFrame++;   // Increment the frame state
		jumpTimer = 0;           // Reset jump timer
	} 
}

void frogFreezeStand(int buttonCursor, int row) {
	byte image01[8] = {B00000, B00000, B00000, B00000, B01111, B11101, B11111, B11010};


	lcd.createChar(0, image01);


	lcd.setCursor(buttonCursor, row);
	lcd.write(byte(0));
}

void idleDownTop0(int buttonCursor, int row) {
	lcd.setCursor(buttonCursor, row);
	lcd.print(" ");

	byte image01[8] = {B00000, B00000, B00000, B00000, B00000, B01111, B11101, B11111};


	lcd.createChar(0, image01);


	lcd.setCursor(buttonCursor, row);
	lcd.write(byte(0));
	if (row == 0) {
		frog1Standing = false;
	} else {
		frog2Standing = false;
	}
}

void idleUpTop0(int buttonCursor, int row) {
	lcd.setCursor(buttonCursor, row);
	lcd.print(" ");


	byte image01[8] = {B00000, B00000, B00000, B00000, B01111, B11101, B11111, B11010};


	lcd.createChar(0, image01);


	lcd.setCursor(buttonCursor, row);
	lcd.write(byte(0));
	if (row == 0) {
		frog1Standing = true;
	} else {
		frog2Standing = true;
	}

}

void jump1Top0(int buttonCursor, int row) {
	lcd.setCursor(buttonCursor, row);
	lcd.print(" ");
	byte image01[8] = {B00000, B00000, B00000, B01111, B11101, B11111, B10010, B01000};


	lcd.createChar(0, image01);


	lcd.setCursor(buttonCursor, row);
	lcd.write(byte(0));

}

void jump2Top0(int buttonCursor, int row) {
	lcd.setCursor(buttonCursor, row);
	lcd.print(" ");

	byte image01[8] = {B00000, B00000, B00111, B01110, B01111, B01001, B10000, B00000};


	lcd.createChar(0, image01);


	lcd.setCursor(buttonCursor, row);
	lcd.write(byte(0));

}

void jump3Top0(int buttonCursor, int row) {
	lcd.setCursor(buttonCursor, row);
	lcd.print(" ");

	byte image01[8] = {B00000, B00000, B00011, B00111, B00111, B01100, B00000, B00000};
	byte image02[8] = {B00000, B00000, B10000, B10000, B10000, B00000, B00000, B00000};


	lcd.createChar(0, image01);
	lcd.createChar(1, image02);


	lcd.setCursor(buttonCursor, row);
	lcd.write(byte(0));
	lcd.setCursor(buttonCursor + 1, row);
	lcd.write(byte(1));

}

void jump4Top0(int buttonCursor, int row) {
	lcd.setCursor(buttonCursor-1, row);
	lcd.print(" ");
	lcd.setCursor(buttonCursor, row);
	lcd.print(" ");

	byte image01[8] = {B00000, B00000, B00001, B00011, B00011, B00010, B00100, B00000};
	byte image02[8] = {B00000, B00000, B11000, B01000, B11000, B10000, B00000, B00000};


	lcd.createChar(0, image01);
	lcd.createChar(1, image02);


	lcd.setCursor(button1Cursor, row);
	lcd.write(byte(0));
	lcd.setCursor(button1Cursor + 1, row);
	lcd.write(byte(1));

}

void jump5Top0(int buttonCursor, int row) {
	lcd.setCursor(buttonCursor-1, row);
	lcd.print(" ");
	lcd.setCursor(buttonCursor, row);
	lcd.print(" ");

	byte image01[8] = {B00000, B00000, B00000, B00000, B00001, B00001, B00001, B00010};
	byte image02[8] = {B00000, B00000, B00000, B11100, B10100, B11100, B01000, B00000};


	lcd.createChar(0, image01);
	lcd.createChar(1, image02);


	lcd.setCursor(buttonCursor, row);
	lcd.write(byte(0));
	lcd.setCursor(buttonCursor + 1, row);
	lcd.write(byte(1));

}

void jump6Top0(int buttonCursor, int row) {
	lcd.setCursor(buttonCursor-1, row);
	lcd.print(" ");
	lcd.setCursor(buttonCursor, row);
	lcd.print(" ");

	byte image02[8] = {B00000, B00000, B00000, B11110, B11010, B11110, B00100, B00000};


	lcd.createChar(0, image02);


	lcd.setCursor(buttonCursor + 1, row);
	lcd.write(byte(0));

}

void jump7Top0(int buttonCursor, int row) {
	lcd.setCursor(buttonCursor-1, row);
	lcd.print(" ");
	lcd.setCursor(buttonCursor, row);
	lcd.print(" ");

	byte image02[8] = {B00000, B00000, B00000, B01111, B11101, B11111, B10010, B01000};


	lcd.createChar(0, image02);


	lcd.setCursor(buttonCursor + 1, row);
	lcd.write(byte(0));

}

void idleUpTop1(int buttonCursor, int row) {
	lcd.setCursor(buttonCursor, row);
	lcd.print(" ");

	byte image02[8] = {B00000, B00000, B00000, B00000, B11111, B11101, B11111, B11010};


	lcd.createChar(0, image02);


	lcd.setCursor(buttonCursor + 1, row);
	lcd.write(byte(0));

}

// when is this ever called?
void idleDownTop1(int buttonCursor, int row) {
	lcd.setCursor(buttonCursor, row);
	lcd.print(" ");

	byte image02[8] = {B00000, B00000, B00000, B00000, B00000, B11111, B11101, B11111};


	lcd.createChar(0, image02);


	lcd.setCursor(1, row);
	lcd.write(byte(0));

}

void winner(int player) {
	
	byte image02[8] = {B10101, B11111, B11111, B00000, B01111, B11101, B11111, B11010};
	
	
	lcd.createChar(0, image02);
	
	
	lcd.setCursor(15, player-1);
	lcd.write(byte(0));
  
  }
