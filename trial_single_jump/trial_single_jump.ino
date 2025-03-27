#include <LiquidCrystal.h>

#define BUTTON1_PIN 3
#define BUTTON2_PIN 9
#define ROCK1_PIN 8
#define ROCK2_PIN 10
#define CLOCK_PIN A5
#define FROG1_CHAR_START 0  // Character slots 0-3 for frog 1
#define FROG2_CHAR_START 4  // Character slots 4-7 for frog 2
#define INTERRUPT_PIN 2

const int MAX_ROCKS_PER_ROUND = 3;

int tick = 0; //value written to clock pin, we alternate between zero and 1

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

int idleAnimationRate = 300;
bool frog1UpdateIdle = false;
bool frog2UpdateIdle = false;

int frog1PrevIdleTimeSlice = 0;
int frog2PrevIdleTimeSlice = 0;

volatile int rock1Throw = 0; // player 1 has thrown a rock
volatile int rock2Throw = 0; 
bool rock1Placed = 0; // rock is in front of player 1- poor naming??.. gotta change this
bool rock2Placed = 0; 
int rock1Timer = 0; // time before rock is destroyed
int rock2Timer = 0;
int rock1LastTime_Slice = 0;
int rock2LastTime_Slice = 0;

int actionFlag = 0; // if the player interacts with the game then this is incremented
int gameTimeoutLastTime_Slice = 0;
int gameTimeout = 0; // 5 second timer to track whether the game should timeout (clear display and reset)
bool displayOn = true;// if we turn off lcd during idle reset

int jumpAnimationRate = 70; // Frame rate for jump animation (faster than idle)
bool frog1UpdateJump = false;
bool frog2UpdateJump = false;

int frog1PrevJumpTimeSlice = 0;
int frog2PrevJumpTimeSlice = 0;

// Add variables to track rock throws per player
int rock1ThrowCount = 0;
int rock2ThrowCount = 0;

int frogWon = 0;

void setup() {

	lcd.begin(16,   2);
	// Serial.begin(9600); 

	attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), buttonsISR, RISING);

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
	pinMode(ROCK1_PIN, INPUT);
	pinMode(ROCK2_PIN, INPUT);
	pinMode(CLOCK_PIN, OUTPUT);
}

void loop() {

	actionFlag = button1state + button2state + rock1Throw + rock2Throw;

	if (actionFlag > 0){
		gameTimeout = 0;
		if (!displayOn){
			lcd.display();
			displayOn = true;
			        // Reset all button states when display comes back on
			button1state = 0;
			button2state = 0;
			frog1Jumping = false;
			frog2Jumping = false;
			
			// Reset rock throw counts for new round
			rock1ThrowCount = 0;
			rock2ThrowCount = 0;
		}

	} else {
		if (time_slice != gameTimeoutLastTime_Slice) {  
            gameTimeout++;
            gameTimeoutLastTime_Slice = time_slice;
        }

		if (gameTimeout >= 500) {
     		lcd.clear();
			lcd.noDisplay();
			button1Cursor = 0;
			button2Cursor = 0;
			gameTimeout = 0;
			displayOn = false;
		}
	}


    // button 1 pressed
    if (button1state == 1 && !rock1Placed && frogWon != 1) {  // Only allow idle if no rock is placed
        if (time_slice - frog1PrevIdleTimeSlice >= idleAnimationRate) {
            frog1UpdateIdle = true;
            frog1PrevIdleTimeSlice = time_slice;
        }
        // FIXME 
        // Reset flagtimer when button is pressed
        if (frog1UpdateIdle) {
            // Serial.println(frog1Standing);

            if (frog1Standing) {
                idleDownTop0(button1Cursor, 0);
            } else {
                idleUpTop0(button1Cursor, 0);
            }
            //frog1Standing = !frog1Standing; // Flip the toggle state
            frog1UpdateIdle = false;
        }
    }

    // button 2 pressed
    if (button2state == 1 && !rock2Placed && frogWon != 2) {  // Only allow idle if no rock is placed
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
    }
  
  // button 1 released
      if (button1state == 0 && !rock1Placed && frogWon != 1) {
        
        // check current idle animation frame
        // if frog1Standing false,then jump
        if (frog1Standing == false){
			// to avoid interrupt
			frog1Jumping = true;
			
			// Check if it's time to update the jump frame
			if (time_slice - frog1PrevJumpTimeSlice >= jumpAnimationRate) {
				frog1UpdateJump = true;
				frog1PrevJumpTimeSlice = time_slice;
			}
			
			if (frog1UpdateJump) {
				frogJumpFrame(cycle1Complete, button1Cursor, frog1JumpFrame, 0);
				frog1UpdateJump = false;
			}
        }
        else {
			frogFreezeStand(button1Cursor, 0);
        }
        if (cycle1Complete){ // if we didn't release at the right time
          //button1Downflag = 0; // set to false manually so no conflict
			frog1JumpFrame = 0; 
			// Add cap to ensure cursor doesn't exceed 15
			if (button1Cursor < 15) {
				button1Cursor++;
			}
			cycle1Complete = false;
			frog1Standing = true;
			frog1Jumping = false;
        }
        
    }

	  // button 2 released
      if (button2state == 0 && !rock2Placed) {
        
        // check current idle animation frame
        // if frog1Standing false,then jump
        if (frog2Standing == false){
			// to avoid interrupt
			frog2Jumping = true;
			
			// Check if it's time to update the jump frame
			if (time_slice - frog2PrevJumpTimeSlice >= jumpAnimationRate) {
				frog2UpdateJump = true;
				frog2PrevJumpTimeSlice = time_slice;
			}
			
			if (frog2UpdateJump) {
				frogJumpFrame(cycle2Complete, button2Cursor, frog2JumpFrame, 1);
				frog2UpdateJump = false;
			}
        }
        else {
			frogFreezeStand(button2Cursor, 1);
        }
        if (cycle2Complete){ // if we didn't release at the right time
          //button1Downflag = 0; // set to false manually so no conflict
			frog2JumpFrame = 0; 
			// Add cap to ensure cursor doesn't exceed 15
			if (button2Cursor < 15) {
				button2Cursor++;
			}
			cycle2Complete = false;
			frog2Standing = true;
			frog2Jumping = false;
        }
        
    }
  // Win condition
  if (button1Cursor == 15 && button2Cursor != 15) {
    // crown the frog!
        winner(0);
  } else if (button2Cursor == 15 && button1Cursor != 15) {
        winner(1);
        }

	if (rock1Throw == 1 && rock1ThrowCount < MAX_ROCKS_PER_ROUND) { // rock has been thrown by player 1
        if (time_slice != rock1LastTime_Slice) {
            rock1Timer++;
            rock1LastTime_Slice = time_slice;
        }
        if (frog2Jumping == false) {
            rock(button2Cursor, 1); // place rock w/ player 2's cursor, on row 2 (bottom)
        } else {
            rock(button2Cursor + 1, 1); //anticipate where frog will land
        }
        rock2Placed = 1;

        if (rock1Timer >= 400 && rock2Placed == 1) { //time to remove rock (timeout to destroy)
                clearRock(button2Cursor, 1);
                rock2Placed = 0;
                rock1Throw = 0;
                rock1Timer = 0 ;
                rock1ThrowCount++; // Increment rock throw count
        }
    } else if (rock1Throw == 1 && rock1ThrowCount >= MAX_ROCKS_PER_ROUND) {
        // If player has used all rocks, just reset the throw flag
        rock1Throw = 0;
    }

    if (rock2Throw == 1 && rock2ThrowCount < MAX_ROCKS_PER_ROUND) { // rock has been thrown by player 2
        if (time_slice != rock2LastTime_Slice) {  // Ensures we only increment once per timer tick
            rock2Timer++;
            rock2LastTime_Slice = time_slice;
        }
        if (frog1Jumping == false) {
            rock(button1Cursor, 0); // place rock w/ player 2's cursor, on row 2 (bottom)
        } else {
            rock(button1Cursor + 1, 0); //anticipate where frog will land
        }
        rock1Placed = 1;

        if (rock2Timer >= 400 && rock1Placed == 1) { //time to remove rock (timeout to destroy)
            clearRock(button1Cursor, 0);
            rock1Placed = 0;
            rock2Throw = 0;
            rock2Timer = 0;
            rock2ThrowCount++; // Increment rock throw count
        }
    } else if (rock2Throw == 1 && rock2ThrowCount >= MAX_ROCKS_PER_ROUND) {
        // If player has used all rocks, just reset the throw flag
        rock2Throw = 0;
    }

}

void buttonsISR() {
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

	if (digitalRead(ROCK1_PIN) == HIGH) {
        rock1Throw = 1;
    }
    if (digitalRead(ROCK2_PIN) == HIGH) {
        rock2Throw = 1;
    }
}

// Timer ISR (1ms per tick)
ISR(TIMER0_COMPA_vect) {
    time_slice++;  // Lightweight ISR, only updates a single variable
	digitalWrite(CLOCK_PIN, tick);
	tick = !tick;
}

// New function to handle jump animation frames
void frogJumpFrame(bool &cycleComplete, int &buttonCursor, int &frogJumpFrame, int row) {
    switch (frogJumpFrame) {
        case 0: jump1Top0(buttonCursor, row); break;
        case 1: jump2Top0(buttonCursor, row); break;
        case 2: jump3Top0(buttonCursor, row); break;
        case 3: jump4Top0(buttonCursor, row); break;
        case 4: jump5Top0(buttonCursor, row); break;
        case 5: jump6Top0(buttonCursor, row); break;
        case 6: jump7Top0(buttonCursor, row); break;
        case 7: 
            idleUpTop1(buttonCursor, row); 
            cycleComplete = true; 
            break;
    }
    
    frogJumpFrame++; // Increment the frame state
}

void clearRow(int row) {
	//byte imgEmpty[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};


  //lcd.createChar(0, imgEmpty);


  lcd.setCursor(0, row);
  lcd.write("               ");

	
}

void frogFreezeStand(int buttonCursor, int row) {
    byte image01[8] = {B00000, B00000, B00000, B00000, B01111, B11101, B11111, B11010};

    int charOffset = (row == 0) ? FROG1_CHAR_START : FROG2_CHAR_START;

    lcd.createChar(charOffset, image01);

    lcd.setCursor(buttonCursor, row);
    lcd.write(byte(charOffset));
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

    int charOffset = (row == 0) ? FROG1_CHAR_START : FROG2_CHAR_START;

    lcd.createChar(charOffset, imgEmpty);
    lcd.createChar(charOffset + 1, image01);

    clearRow(row);
    lcd.setCursor(buttonCursor, row);
    lcd.write(byte(charOffset + 1));
}

void jump2Top0(int buttonCursor, int row) {
    byte image01[8] = {B00000, B00000, B00111, B01110, B01111, B01001, B10000, B00000};
    byte imgEmpty[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};

    int charOffset = (row == 0) ? FROG1_CHAR_START : FROG2_CHAR_START;

    lcd.createChar(charOffset, imgEmpty);
    lcd.createChar(charOffset + 1, image01);

    clearRow(row);
    lcd.setCursor(buttonCursor, row);
    lcd.write(byte(charOffset + 1));
}

void jump3Top0(int buttonCursor, int row) {
    byte image01[8] = {B00000, B00000, B00011, B00111, B00111, B01100, B00000, B00000};
    byte image02[8] = {B00000, B00000, B10000, B10000, B10000, B00000, B00000, B00000};
    byte imgEmpty[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};

    int charOffset = (row == 0) ? FROG1_CHAR_START : FROG2_CHAR_START;

    lcd.createChar(charOffset, imgEmpty);
    lcd.createChar(charOffset + 1, image01);
    lcd.createChar(charOffset + 2, image02);

    clearRow(row);
    lcd.setCursor(buttonCursor, row);
    lcd.write(byte(charOffset + 1));
    lcd.setCursor(buttonCursor + 1, row);
    lcd.write(byte(charOffset + 2));
}

void jump4Top0(int buttonCursor, int row) {
    byte image01[8] = {B00000, B00000, B00001, B00011, B00011, B00010, B00100, B00000};
    byte image02[8] = {B00000, B00000, B11000, B01000, B11000, B10000, B00000, B00000};
    byte imgEmpty[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};

    int charOffset = (row == 0) ? FROG1_CHAR_START : FROG2_CHAR_START;

    lcd.createChar(charOffset, imgEmpty);
    lcd.createChar(charOffset + 1, image01);
    lcd.createChar(charOffset + 2, image02);

    clearRow(row);
    lcd.setCursor(buttonCursor, row);
    lcd.write(byte(charOffset + 1));
    lcd.setCursor(buttonCursor + 1, row);
    lcd.write(byte(charOffset + 2));
}

void jump5Top0(int buttonCursor, int row) {
    byte image01[8] = {B00000, B00000, B00000, B00000, B00001, B00001, B00001, B00010};
    byte image02[8] = {B00000, B00000, B00000, B11100, B10100, B11100, B01000, B00000};
    byte imgEmpty[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};

    int charOffset = (row == 0) ? FROG1_CHAR_START : FROG2_CHAR_START;

    lcd.createChar(charOffset, imgEmpty);
    lcd.createChar(charOffset + 1, image01);
    lcd.createChar(charOffset + 2, image02);

    clearRow(row);
    lcd.setCursor(buttonCursor, row);
    lcd.write(byte(charOffset + 1));
    lcd.setCursor(buttonCursor + 1, row);
    lcd.write(byte(charOffset + 2));
}

void jump6Top0(int buttonCursor, int row) {
    byte image01[8] = {B00000, B00000, B00000, B11110, B11010, B11110, B00100, B00000};
    byte imgEmpty[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};

    int charOffset = (row == 0) ? FROG1_CHAR_START : FROG2_CHAR_START;

    lcd.createChar(charOffset, imgEmpty);
    lcd.createChar(charOffset + 1, image01);

    clearRow(row);
    lcd.setCursor(buttonCursor+1, row);
    lcd.write(byte(charOffset + 1));
}

void jump7Top0(int buttonCursor, int row) {
    byte image01[8] = {B00000, B00000, B00000, B01111, B11101, B11111, B10010, B01000};
    byte imgEmpty[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};

    int charOffset = (row == 0) ? FROG1_CHAR_START : FROG2_CHAR_START;

    lcd.createChar(charOffset, imgEmpty);
    lcd.createChar(charOffset + 1, image01);

    clearRow(row);
    lcd.setCursor(buttonCursor+1, row);
    lcd.write(byte(charOffset + 1));
}

void idleUpTop1(int buttonCursor, int row) {
    byte image02[8] = {B00000, B00000, B00000, B00000, B11111, B11101, B11111, B11010};

    int charOffset = (row == 0) ? FROG1_CHAR_START : FROG2_CHAR_START;

    lcd.createChar(charOffset, image02);

    clearRow(row);
    lcd.setCursor(buttonCursor + 1, row);
    lcd.write(byte(charOffset));
}

void idleDownTop1(int buttonCursor, int row) {
    byte image02[8] = {B00000, B00000, B00000, B00000, B00000, B11111, B11101, B11111};

    int charOffset = (row == 0) ? FROG1_CHAR_START : FROG2_CHAR_START;

    lcd.createChar(charOffset, image02);

    clearRow(row);
    lcd.setCursor(1, row);
    lcd.write(byte(charOffset));
}

void winner(int row) {
	
	byte image02[8] = {B10101, B11111, B11111, B00000, B01111, B11101, B11111, B11010};

    int charOffset = (row == 0) ? FROG1_CHAR_START : FROG2_CHAR_START;

    lcd.createChar(charOffset, image02);
        
	
	lcd.setCursor(15, row);
	lcd.write(byte(charOffset));
    frogWon = row + 1;
  
  }

void rock(int button1Cursor, int row) { // LCD row for rock placement
	byte image08[8] = {B00000, B00000, B01110, B10101, B10111, B10001, B01110, B00000};
 	lcd.createChar(7, image08);
	lcd.setCursor(button1Cursor + 1, row); //rock at next available segment
	lcd.write(byte(7)); 
}

void clearRock(int button1Cursor, int row){ //clear segment with rock position
//   byte image01[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000};
//   lcd.createChar(8, image01);
	lcd.setCursor(button1Cursor + 1, row); // Use stored row and column variables 
	lcd.write(" "); 
}

