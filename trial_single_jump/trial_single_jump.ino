#include <LiquidCrystal.h>

#define BUTTON1_PIN 8
#define BUTTON2_PIN 9
#define COMBINEDANALOG_PIN A1
#define FROG1_CHAR_START 0  // Character slots 0-3 for frog 1
#define FROG2_CHAR_START 4  // Character slots 4-7 for frog 2

LiquidCrystal   lcd(12, 11, 7, 6, 5, 4); // RS, E, D4, D5, D6, D7

volatile int time_slice = 0; // Keeps track of elapsed milliseconds
volatile int button1state;
volatile int button2state;

int button1Cursor = 0; //keeps track of cursor position for top animation
int button2Cursor = 0;
bool cycle1Complete = false;
bool cycle2Complete = false;

bool frog1IsIdling = false;
bool frog2IsIdling = false;

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

int idleAnimationRate = 300;
bool frog1UpdateIdle = false;
bool frog2UpdateIdle = false;

int frog1PrevIdleTimeSlice = 0;
int frog2PrevIdleTimeSlice = 0;



void setup() {

	lcd.begin(16,   2);
	// Serial.begin(9600); 

	attachInterrupt(digitalPinToInterrupt(2), buttonsISR, RISING);

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
	pinMode(BUTTON2_PIN, INPUT); // We have external pulldown setup
	pinMode(COMBINEDANALOG_PIN, INPUT);
}

void loop() {
    // frog 1 idle
    if (button1state % 2== 1) {
		if (time_slice - frog1PrevIdleTimeSlice >= idleAnimationRate) {
			frog1UpdateIdle = true;
			frog1PrevIdleTimeSlice = time_slice;
		}
		// Reset flagtimer when button is pressed
		if (frog1UpdateIdle) {
			// Serial.println(frog1Standing);

			if (frog1Standing) {
				idleDownTop0(button1Cursor, 0);
			} else {
				idleUpTop0(button1Cursor, 0);
			}
			//frog1Standing = !frog1Standing; // Flip the toggle stat
			frog1UpdateIdle = false;
		}
	} else {
		// check current idle animation frame
		// if frog1Standing false,then jump
		if (frog1Standing == false){
			// to avoid interrupt
			frog1Jumping = true;
			frogJump(cycle1Complete, button1Cursor, jump1Timer, frog1JumpFrame, button1UpLastTime_slice, 0);
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

	// frog 2 idle
    if (button2state % 2 == 1) {
		if (time_slice - frog2PrevIdleTimeSlice >= idleAnimationRate) {
			frog2UpdateIdle = true;
			frog2PrevIdleTimeSlice = time_slice;
		}

		// Reset flagtimer when button is pressed
		if (frog2UpdateIdle) {
			if (frog2Standing) {
				idleDownTop0(button2Cursor, 1);
			} else {
				idleUpTop0(button2Cursor, 1);
			}
			//frog1Standing = !frog1Standing; // Flip the toggle state
			frog2UpdateIdle = false;
		}
    } else {
		// frog 2 jump then stand
		// check current idle animation frame
        // if frog1Standing false,then jump
        if (frog2Standing == false){
			// to avoid interrupt
			frog2Jumping = true;
			frogJump(cycle2Complete, button2Cursor, jump2Timer, frog2JumpFrame, button2UpLastTime_slice, 2);
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

void buttonsISR() {
    if (digitalRead(BUTTON1_PIN) == HIGH && frog1Jumping == false) {
        button1state++;
    }

    // if (digitalRead(BUTTON1_PIN) == LOW && frog1Jumping == false) {
    //     button1state = 0;
    // }
    
    if (digitalRead(BUTTON2_PIN) == HIGH && frog2Jumping == false) {
        button2state++;
    }

    // if (digitalRead(BUTTON2_PIN) == LOW && frog2Jumping == false) {
    //     button2state = 0;
    // }

}

// Timer ISR (1ms per tick)
ISR(TIMER0_COMPA_vect) {
    time_slice++;  // Lightweight ISR, only updates a single variable
}

void frogJump(bool &cycleComplete, int &buttonCursor, int &jumpTimer, int &frogJumpFrame, int &buttonUpLastTime_slice, int row) {

	if (time_slice != buttonUpLastTime_slice) {  // Ensures we only increment once per timer tick
		jumpTimer++;
		buttonUpLastTime_slice = time_slice;
	}

	if (jumpTimer >= 20 && !cycleComplete) { // Run through jump sequence
		switch (frogJumpFrame) {
			case 0: jump1Top0(buttonCursor, row); break;
			case 1: jump2Top0(buttonCursor, row); break;
      case 2: jump3Top0(buttonCursor, row); break;
      case 3: jump4Top0(buttonCursor, row); break;
			case 4: jump5Top0(buttonCursor, row); break;
			case 5: jump6Top0(buttonCursor, row); break;
			case 6: jump7Top0(buttonCursor, row); break;
			case 7: idleUpTop1(buttonCursor, row); cycleComplete = true; break;
		}

		frogJumpFrame++;   // Increment the frame state
		jumpTimer = 0;           // Reset jump timer
	} 
}

void clearRow(int row) {
	//byte imgEmpty[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};


  //lcd.createChar(0, imgEmpty);


  lcd.setCursor(0, row);
  lcd.write("               ");

	
}

void frogFreezeStand(int buttonCursor, int row) {
	byte image01[8] = {B00000, B00000, B00000, B00000, B01111, B11101, B11111, B11010};


	lcd.createChar(0, image01);


	lcd.setCursor(buttonCursor, row);
	lcd.write(byte(0));
}

void idleDownTop0(int buttonCursor, int row) {
    byte image01[8] = {B00000, B00000, B00000, B00000, B00000, B01111, B11101, B11111};
    byte imgEmpty[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};

    // Use different character slots based on which frog
    int charOffset = (row == 0) ? FROG1_CHAR_START : FROG2_CHAR_START;
    
    lcd.createChar(charOffset, imgEmpty);
    lcd.createChar(charOffset + 1, image01);

    clearRow(row);
    lcd.setCursor(buttonCursor, row);
    lcd.write(byte(charOffset + 1));
    
    if (row == 0) {
        frog1Standing = false;
    } else {
        frog2Standing = false;
    }
}

void idleUpTop0(int buttonCursor, int row) {
    byte image01[8] = {B00000, B00000, B00000, B00000, B01111, B11101, B11111, B11010};
    byte imgEmpty[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};

    int charOffset = (row == 0) ? FROG1_CHAR_START : FROG2_CHAR_START;
    
    lcd.createChar(charOffset, imgEmpty);
    lcd.createChar(charOffset + 1, image01);

    clearRow(row);
    lcd.setCursor(buttonCursor, row);
    lcd.write(byte(charOffset + 1));

    if (row == 0) {
        frog1Standing = true;
    } else {
        frog2Standing = true;
    }
}

void jump1Top0(int buttonCursor, int row) {
	byte image01[8] = {B00000, B00000, B00000, B01111, B11101, B11111, B10010, B01000};
	byte imgEmpty[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};



	lcd.createChar(0, imgEmpty);
	lcd.createChar(1, image01);

	clearRow(row);
	lcd.setCursor(buttonCursor, row);
	lcd.write(byte(1));

}

void jump2Top0(int buttonCursor, int row) {

	byte image01[8] = {B00000, B00000, B00111, B01110, B01111, B01001, B10000, B00000};
	// byte imgEmpty[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};



	// lcd.createChar(0, imgEmpty);
	lcd.createChar(1, image01);

	clearRow(row);
	lcd.setCursor(buttonCursor, row);
	lcd.write(byte(1));

}

void jump3Top0(int buttonCursor, int row) {

	byte image01[8] = {B00000, B00000, B00011, B00111, B00111, B01100, B00000, B00000};
	byte image02[8] = {B00000, B00000, B10000, B10000, B10000, B00000, B00000, B00000};
	
	byte imgEmpty[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};


	lcd.createChar(0, imgEmpty);
	lcd.createChar(1, image01);
	lcd.createChar(2, image02);

	clearRow(row);
	lcd.setCursor(buttonCursor, row);
	lcd.write(byte(1));
	lcd.setCursor(buttonCursor + 1, row);
	lcd.write(byte(2));

}

void jump4Top0(int buttonCursor, int row) {

	byte image01[8] = {B00000, B00000, B00001, B00011, B00011, B00010, B00100, B00000};
	byte image02[8] = {B00000, B00000, B11000, B01000, B11000, B10000, B00000, B00000};
	byte imgEmpty[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};


	lcd.createChar(0, imgEmpty);
	lcd.createChar(1, image01);
	lcd.createChar(2, image02);


	clearRow(row);
	lcd.setCursor(buttonCursor, row);
	lcd.write(byte(1));
	lcd.setCursor(buttonCursor + 1, row);
	lcd.write(byte(2));

}

void jump5Top0(int buttonCursor, int row) {

	byte image01[8] = {B00000, B00000, B00000, B00000, B00001, B00001, B00001, B00010};
	byte image02[8] = {B00000, B00000, B00000, B11100, B10100, B11100, B01000, B00000};
	byte imgEmpty[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};


	lcd.createChar(0, imgEmpty);
	lcd.createChar(1, image01);
	lcd.createChar(2, image02);


	clearRow(row);
	lcd.setCursor(buttonCursor, row);
	lcd.write(byte(1));
	lcd.setCursor(buttonCursor + 1, row);
	lcd.write(byte(2));


}

void jump6Top0(int buttonCursor, int row) {

	byte image01[8] = {B00000, B00000, B00000, B11110, B11010, B11110, B00100, B00000};
	byte imgEmpty[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};



	lcd.createChar(0, imgEmpty);
	lcd.createChar(1, image01);

	clearRow(row);
	lcd.setCursor(buttonCursor+1, row);
	lcd.write(byte(1));

}

void jump7Top0(int buttonCursor, int row) {

	byte image01[8] = {B00000, B00000, B00000, B01111, B11101, B11111, B10010, B01000};
	byte imgEmpty[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};



	lcd.createChar(0, imgEmpty);
	lcd.createChar(1, image01);

	clearRow(row);
	lcd.setCursor(buttonCursor+1, row);
	lcd.write(byte(1));

}

void idleUpTop1(int buttonCursor, int row) {

	byte image02[8] = {B00000, B00000, B00000, B00000, B11111, B11101, B11111, B11010};


	lcd.createChar(0, image02);

	clearRow(row);
	lcd.setCursor(buttonCursor + 1, row);
	lcd.write(byte(0));

}

// when is this ever called?
void idleDownTop1(int buttonCursor, int row) {

	byte image02[8] = {B00000, B00000, B00000, B00000, B00000, B11111, B11101, B11111};


	lcd.createChar(0, image02);

	clearRow(row);
	lcd.setCursor(1, row);
	lcd.write(byte(0));

}

void winner(int player) {
	
	byte image02[8] = {B10101, B11111, B11111, B00000, B01111, B11101, B11111, B11010};
	
	
	lcd.createChar(0, image02);
	
	
	lcd.setCursor(15, player-1);
	lcd.write(byte(0));
  
  }
