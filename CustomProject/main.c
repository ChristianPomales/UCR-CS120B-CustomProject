#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <ucr/bit.h>
#include <ucr/timer.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "io.h"
#include "io.c"

#define PLAYERHITTERRAINSOUND 261.63
#define PLAYERHITENEMYSOUND 293.66
#define PROJECTILEFIREDSOUND 523.25
#define PROJECTILEHITENEMYSOUND 440.00

// audio functions
void set_PWM(double frequency) {
	// 0.954 hz is lowest frequency possible with this function,
	// based on settings in PWM_on()
	// Passing in 0 as the frequency will stop the speaker from generating sound
	
	static double current_frequency; // Keeps track of the currently set frequency
	// Will only update the registers when the frequency changes, otherwise allows
	// music to play uninterrupted.
	if (frequency != current_frequency) {
		if (!frequency) { TCCR3B &= 0x08; } //stops timer/counter
		else { TCCR3B |= 0x03; } // resumes/continues timer/counter
		
		// prevents OCR3A from overflowing, using prescaler 64
		// 0.954 is smallest frequency that will not result in overflow
		if (frequency < 0.954) { OCR3A = 0xFFFF; }
		
		// prevents OCR0A from underflowing, using prescaler 64					// 31250 is largest frequency that will not result in underflow
		else if (frequency > 31250) { OCR3A = 0x0000; }
		
		// set OCR3A based on desired frequency
		else { OCR3A = (short)(8000000 / (128 * frequency)) - 1; }

		TCNT0 = 0; // resets counter
		current_frequency = frequency; // Updates the current frequency
	}
}

void PWM_on() {
	TCCR3A = (1 << COM0A0);
	// COM3A0: Toggle PB3 on compare match between counter and OCR0A
	TCCR3B = (1 << WGM02) | (1 << CS01) | (1 << CS00);
	// WGM02: When counter (TCNT0) matches OCR0A, reset counter
	// CS01 & CS30: Set a prescaler of 64
	set_PWM(0);
}

void PWM_off() {
	TCCR0A = 0x00;
	TCCR3B = 0x00;
}

// keypad function
unsigned char GetKeypadKey() {

	PORTC = 0xEF; // Enable col 4 with 0, disable others with 1’s
	asm("nop"); // add a delay to allow PORTC to stabilize before checking
	if (GetBit(PINC,0)==0) { return(' '); }
	if (GetBit(PINC,1)==0) { return('4'); }
	if (GetBit(PINC,2)==0) { return('7'); }
	if (GetBit(PINC,3)==0) { return('*'); }

	// Check keys in col 2
	PORTC = 0xDF; // Enable col 5 with 0, disable others with 1’s
	asm("nop"); // add a delay to allow PORTC to stabilize before checking
	if (GetBit(PINC,0)==0) { return('2'); }
	if (GetBit(PINC,1)==0) { return('5'); }
	if (GetBit(PINC,2)==0) { return('8'); }
	if (GetBit(PINC,3)==0) { return('0'); }


	// Check keys in col 3
	PORTC = 0xBF; // Enable col 6 with 0, disable others with 1’s
	asm("nop"); // add a delay to allow PORTC to stabilize before checking
	if (GetBit(PINC,0)==0) { return('3'); }
	if (GetBit(PINC,1)==0) { return('6'); }
	if (GetBit(PINC,2)==0) { return('9'); }
	if (GetBit(PINC,3)==0) { return('#'); }

	// Check keys in col 4
	PORTC = 0x7F; // Enable col 6 with 0, disable others with 1’s
	asm("nop"); // add a delay to allow PORTC to stabilize before checking
	if (GetBit(PINC,0)==0) { return('A'); }
	if (GetBit(PINC,1)==0) { return('B'); }
	if (GetBit(PINC,2)==0) { return('C'); }
	if (GetBit(PINC,3)==0) { return('D'); }

	return('\0'); // default value

}

// movement functions
unsigned char moveUP(unsigned char location) {
	// only move up if we're on the bottom line
	if ((location >= 16) && (location <= 31)) {
		return location - 16;
	}
	return location;
}

unsigned char moveDown(unsigned char location) {
	// only move down if we're on the top line
	if ((location >= 0) && (location <= 15)) {
		return location + 16;
	}
	return location;
}

unsigned char moveLeft(unsigned char location) {
	// top line
	if ((location > 0) && (location <= 15)) {
		return location - 1;
	}
	
	// bottom line
	else if ((location > 16) && (location <= 31)) {
		return location - 1;
	}
	
	return location;
	
	return location;
}

unsigned char moveRight(unsigned char location) {
	// top line
	if ((location >= 0) && (location < 15)) {
		return location + 1;
	}
	
	// bottom line
	else if ((location >= 16) && (location < 31)) {
		return location + 1;
	}
	
	return location;
}

unsigned char enemyMoveLeft(unsigned char location) {
	// move off screen if enemy is at the leftmost side
	if ((location == 16)||(location == 0)) {
		return 255;
	}
	
	// top line
	else if ((location > 0) && (location <= 15)) {
		return location - 1;
	}
	
	// bottom line
	else if ((location > 16) && (location <= 31)) {
		return location - 1;
	}
	
	return location;
}

// custom character functions
void LCD_build(unsigned char location, unsigned char *ptr){
	unsigned char i;
	if(location<8){
		LCD_WriteCommand(0x40+(location*8));
		for(i=0;i<8;i++)
		LCD_WriteData(ptr[i]);
	}
}

void loadPlayerOneSprite() {
	static unsigned char playerOne[8] = {
		0b01000,
		0b01000,
		0b01100,
		0b11111,
		0b11111,
		0b01100,
		0b01000,
		0b01000
	};
	
	LCD_build(0, playerOne);
}

void loadTerrainSprite() {
	static unsigned char terrain[8] = {
		0b11111,
		0b10101,
		0b11111,
		0b10101,
		0b11111,
		0b10101,
		0b11111,
		0b11111
	};
	
	LCD_build(1, terrain);
}

void loadEnemySprite() {
	static unsigned char enemy[8] = {
		0b00010,
		0b00110,
		0b11111,
		0b00110,
		0b00110,
		0b11111,
		0b00110,
		0b00010
	};
	
	LCD_build(2, enemy);
}

void displayPlayerOneSprite() {
	LCD_WriteData(0);
}

void displayTerrainSprite() {
	LCD_WriteData(1);
}

void displayEnemySprite() {
	LCD_WriteData(2);
}

// shift register code
void transmit_data(unsigned char data) {
	for (unsigned char i = 0; i < 8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTA = 0x08 << 2;
		// set SER = next bit of data to be sent.
		PORTA |= ((data >> i) & 0x01) << 2;
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTA |= 0x02 << 2;
	}
	// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
	PORTA |= 0x04 << 2;
	// clears all lines in preparation of a new transmission
	PORTA = 0x00 << 2;
}

// 7 segment display code -> display is common annode
void displayLives(unsigned char lives) {
	static unsigned char display = 0;
	
	switch (lives) {
		case 0:
			display = 0x3F;
			break;
			
		case 1:
			display = 0x06;
			break;
			
		case 2:
			display = 0x5B;
			break;
			
		case 3:
			display = 0x4F;
			break;
			
		case 4:
			display = 0x66;
			break;
			
		case 5:
			display = 0x6D;
			break;
			
		case 6:
			display = 0x7D;
			break;
			
		case 7:
			display = 0x07;
			break;
			
		case 8:
			display = 0x7F;
			break;
			
		case 9:
			display = 0x67;
			break;
	}
	
	transmit_data(~display);
}

void clearLives() {
	transmit_data(0xFF);
}

// eeprom functions
void saveScore(unsigned char score) {
	eeprom_write_byte(0, score);
}

unsigned char loadScore() {
	return eeprom_read_byte(0);
}

//--------Find GCD function --------------------------------------------------
unsigned long int findGCD(unsigned long int a, unsigned long int b)
{
	unsigned long int c;
	while(1){
		c = a%b;
		if(c==0){return b;}
		a = b;
		b = c;
	}
	return 0;
}
//--------End find GCD function ----------------------------------------------

//--------Task scheduler data structure---------------------------------------
// Struct for Tasks represent a running process in our simple real-time operating system.
typedef struct _task {
	/*Tasks should have members that include: state, period,
		a measurement of elapsed time, and a function pointer.*/
	signed char state; //Task's current state
	unsigned long int period; //Task period
	unsigned long int elapsedTime; //Time elapsed since last task tick
	int (*TickFct)(int); //Task tick function
} task;

//--------Shared Variables----------------------------------------------------
unsigned char playFlag = 0;
unsigned char gameOverFlag = 0;
unsigned char score = 0;
unsigned char lives = 0;
unsigned char key = ' '; // key pressed
unsigned char terrainShift = 0; // how much the terrain should be moved by

// players and their projectiles
unsigned char player_one_location = 0;
unsigned char player_one_projectile_location [5] = {255, 255, 255, 255, 255};
	
// enemies and their projectiles
unsigned char enemy_location [3] = {255, 255, 255};
// unsigned char enemy_projectile_location [6] = {255, 255, 255, 255, 255, 255};

const unsigned char terrain[32] = {
	0xFF, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0xFF, ' ', ' ', ' ',
	' ', 0xFF, ' ', ' ', ' ', ' ', ' ', ' ', 0xFF, ' ', ' ', ' ', ' ', ' ', ' ', ' '
};

unsigned char screenBuffer[32] = {
								' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
								' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '
}; // 16 x 2 characters

//--------User defined FSMs---------------------------------------------------
enum key_tick_states {FETCHKEY};
	
int key_tick(int state) {	
	key = GetKeypadKey();
	return state;
}

enum lcd_display_tick_states {DISPLAY_START_MESSAGE, DISPLAY_GAMEOVER_MESSAGE, DISPLAY_GAME};

int lcd_display_tick(int state) {
	static unsigned char tempScore = 0;
	static unsigned char hundreds = 0;
	static unsigned char tens = 0;
	static unsigned char ones = 0;
	
	static const unsigned char startMessage[32] = {
		'P', 'r', 'e', 's', 's', ' ', 'A', ' ', 't', 'o', ' ', 'S', 't', 'a', 'r', 't',
		'P', 'r', 'e', 's', 's', ' ', 'D', ' ', 't', 'o', ' ', 'R', 'e', 's', 'e', 't'
	}; // 16 x 2 characters

	static const unsigned char pressCToBegin[16] = {
		'P', 'r', 'e', 's', 's', ' ', 'C', ' ', 't', 'o', ' ', 'B', 'e', 'g', 'i', 'n'
	}; // 16 x 1 characters
	
	static const unsigned char screenBlank[32] = {
		' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '
	}; // 16 x 2 characters
	
	switch(state) {
		case DISPLAY_START_MESSAGE:
			if (playFlag == 1) {
				state = DISPLAY_GAME;
			}
			else {
				state = state;
			}
			break;
			
		case DISPLAY_GAMEOVER_MESSAGE:
			if ((gameOverFlag == 0) && (playFlag == 0)) {
				state = DISPLAY_START_MESSAGE;
			}
			if (playFlag == 1) {
				state = DISPLAY_GAME;
			}
			else {
				state = state;
			}
			break;
			
		case DISPLAY_GAME:
			if (gameOverFlag == 1) {
				state = DISPLAY_GAMEOVER_MESSAGE;
			}
			else if (playFlag == 0) {
				state = DISPLAY_START_MESSAGE;
			}
			else {
				state = state;
			}
			break;
	}
	
	switch(state) {
		case DISPLAY_START_MESSAGE:
			for (unsigned char i = 0; i < 32; i++) {
				LCD_Cursor(i + 1);
				LCD_WriteData(startMessage[i]);
				LCD_Cursor(0);
			}
			
			clearLives();
			
			break;
			
		case DISPLAY_GAMEOVER_MESSAGE:
			LCD_ClearScreen();
			
			LCD_DisplayString(1, (const unsigned char *)"SCORE: ");
			
			tempScore = score;
			
			ones = tempScore % 10;
			tempScore = tempScore - ones;
			
			tens = (tempScore % 100) / 10;
			tempScore = tempScore - tens;
			
			hundreds = (tempScore % 1000) / 100;
			tempScore = tempScore - hundreds;
			
			LCD_Cursor(8);
			LCD_WriteData('0' + hundreds);
			
			LCD_Cursor(9);
			LCD_WriteData('0' + tens);
			
			LCD_Cursor(10);
			LCD_WriteData('0' + ones);
			
			for (unsigned char i = 0; i < 16; i++) {
				LCD_Cursor(i + 17);
				LCD_WriteData(pressCToBegin[i]);
				LCD_Cursor(0);
			}
			
			clearLives();
			
			break;
			
		case DISPLAY_GAME:
			// ELEMENTS ARE COMPOSITED FROM BACK TO FRONT
			memcpy(screenBuffer, screenBlank, sizeof(screenBlank));
			
			// displays terrain
			for (unsigned char i = 0; i < 16; i++) {
				screenBuffer[i + 16] = terrain[(i + terrainShift) % 32];
			}
			
			// displays a @ for each enemy
			for (unsigned char i = 0; i < 3; i++) {
				screenBuffer[enemy_location[i]] = '@';
			}
			
			// displays a dash for each projectile from player one
			for (unsigned char i = 0; i < 5; i++) {
				screenBuffer[player_one_projectile_location[i]] = '-';
			}
			
			// display player one
			screenBuffer[player_one_location] = '>';
			
			for (unsigned char i = 0; i < 32; i++) {				
				LCD_Cursor(i + 1);
				
				if (screenBuffer[i] == '>') {
					displayPlayerOneSprite();
				}
				else if (screenBuffer[i] == 0xFF) {
					displayTerrainSprite();
				}
				else if (screenBuffer[i] == '@') {
					displayEnemySprite();
				}
				
				LCD_WriteData(screenBuffer[i]);
				LCD_Cursor(0);
			}
			
			displayLives(lives);
			
			break;
	}
	
	return state;
}

enum game_start_tick_states {GAME_INIT, GAME_PLAYING, GAME_OVER};
	
int game_start_tick(int state) {
	
	if (gameOverFlag == 1) {
		saveScore(score);
		state = GAME_OVER;
	}
	
	switch(state) {
		case GAME_INIT:
			if (key == 'A') {
				unsigned char tempScore = loadScore();
				if (tempScore != 0) {
					score = tempScore;
				}
				lives = 5;
				state = GAME_PLAYING;
			} else {
				state = state;
			}
			break;
		case GAME_PLAYING:
			if (key == 'D') {
				state = GAME_INIT;
				} else {
				state = state;
			}
			break;
		case GAME_OVER:
			if (key == 'C') {
				state = GAME_INIT;
				gameOverFlag = 0;
				} else {
				state = state;
			}
			break;
	}
	
	switch(state) {
		case GAME_INIT:
			playFlag = 0;
			break;
		case GAME_PLAYING:
			playFlag = 1;
			break;
		case GAME_OVER:
			playFlag = 0;
			break;
	}
	
	return state;
}

enum player_one_tick_states {PLAYER_ONE_INIT, PLAYER_ONE_PLAYING};

int player_one_tick(int state) {	
	switch(state) {
		case PLAYER_ONE_INIT:
			if (playFlag == 1)
			{
				state = PLAYER_ONE_PLAYING;
			} 
			else
			{
				state = state;
			}
			break;
		case PLAYER_ONE_PLAYING:
			if (playFlag == 0)
			{
				state = PLAYER_ONE_INIT;
			}
			else
			{
				state = state;
			}
			break;
	}
	
	switch(state) {
		case PLAYER_ONE_INIT:
			player_one_location = 0;
			break;
		case PLAYER_ONE_PLAYING:
			if (key == '2')
			{
				player_one_location = moveUP(player_one_location);
			} else if (key == '8') {
				player_one_location = moveDown(player_one_location);
			} else if (key == '4') {
				player_one_location = moveLeft(player_one_location);
			} else if (key == '6') {
				player_one_location = moveRight(player_one_location);
			}
			
			break;
	}
	
	return state;
}

enum player_one_projectile_tick_states {PLAYER_ONE_PROJECTILE_INIT, PLAYER_ONE_PROJECTILE_PLAYING};

int player_one_projectile_tick(int state) {
	set_PWM(0);
	
	switch(state) {
		case PLAYER_ONE_PROJECTILE_INIT:
			if (playFlag == 1)
			{
				state = PLAYER_ONE_PROJECTILE_PLAYING;
			}
			else
			{
				state = state;
			}
			break;
		case PLAYER_ONE_PROJECTILE_PLAYING:
			if (playFlag == 0)
			{
				state = PLAYER_ONE_PROJECTILE_INIT;
			}
			else
			{
				state = state;
			}
			break;
	}
	
	switch(state) {
		case PLAYER_ONE_PROJECTILE_INIT:
			// resets projectiles
			for (unsigned char i = 0; i < 5; i++) {
				player_one_projectile_location[i] = 255;
			}
			PORTB = 0;
			break;
			
		case PLAYER_ONE_PROJECTILE_PLAYING:
			// create projectiles in array
			if (key == '5') {
				set_PWM(PROJECTILEFIREDSOUND);
				for (unsigned char i = 0; i < 5; i++) {
					if (player_one_projectile_location[i] == 255) {
						player_one_projectile_location[i] = player_one_location;
						break;
					}
				}
			}
						
			// move right one spot every tick
			// might have collision detection issues at the last spot
			for (unsigned char i = 0; i < 5; i++) {
				// don't bother with unused projectiles
				if (player_one_projectile_location[i] != 255) {
					// if its at the end of the screen, make it go off screen
					if ((player_one_projectile_location[i] == 15) || (player_one_projectile_location[i] == 31)) {
						player_one_projectile_location[i] = 255;
					}
					// if its on the screen but not at the end, move right one space
					else {
						player_one_projectile_location[i] = moveRight(player_one_projectile_location[i]);
					}
				}
			}
			
			// sets the shotLED to the number of leftover projectiles
			unsigned char projectilesRemaining = 0;
			for (unsigned char i = 0; i < 5; i++) {
				if (player_one_projectile_location[i] == 255)
				{
					projectilesRemaining = projectilesRemaining << 1;
					projectilesRemaining = projectilesRemaining + 1;
				}
			}
			PORTB = projectilesRemaining;
			
			break;
	}
	
	return state;
}

enum terrain_tick_states {TERRAIN_INIT, TERRAIN_PLAYING};

int terrain_tick(int state) {
	switch(state) {
		case TERRAIN_INIT:
			if (playFlag == 1)
			{
				state = TERRAIN_PLAYING;
			}
			else
			{
				state = state;
			}
			break;
		case TERRAIN_PLAYING:
			if (playFlag == 0)
			{
				state = TERRAIN_INIT;
			}
			else
			{
				state = state;
			}
			break;
	}
	
	switch(state) {
		case TERRAIN_INIT:
			terrainShift = 0;
			break;
		case TERRAIN_PLAYING:			
			terrainShift = terrainShift + 1;
			break;
	}
	
	return state;
}

enum enemy_tick_states {ENEMY_INIT, ENEMY_PLAYING};
	
int enemy_tick(int state) {
	unsigned char enemyGenerator = rand() % 2; // 50% chance to spawn an enemy every tick
	
	switch(state) {
		case ENEMY_INIT:
			if (playFlag == 1) {
				state = ENEMY_PLAYING;
			}
			else {
				state = state;
			}
			break;
		case ENEMY_PLAYING:
			if (playFlag == 0) {
				state = ENEMY_INIT;
			}
			else {
				state = state;
			}
			break;
	}
	
	switch(state) {
		case ENEMY_INIT:
			for (unsigned char i = 0; i < 3; i++) {
				enemy_location[i] = 255;
			}
			break;
		case ENEMY_PLAYING:			
			// determine if we should add an enemy or not
			if (enemyGenerator == 1) {
				for (unsigned char i = 0; i < 3; i++) {
					// determine if we can fit another enemy on screen
					if (enemy_location[i] == 255) {
						// determine if we should spawn on top or bottom
						unsigned char startingPosition = rand() % 2;
						switch(startingPosition) {
							case 1:
								enemy_location[i] = 15;
								break;
							case 2:
								enemy_location[i] = 31;
								break;
						}
					}
				}
			}
			
			// move enemies in random direction - up, down, or froward
			for (unsigned char i = 0; i < 3; i++) {
				// determine if enemy exists or not
				if (enemy_location[i] != 255) {
					// determine movement direction
					unsigned char movementDirection = rand() % 3;
					switch(movementDirection) {
						case 0:
							enemy_location[i] = moveUP(enemy_location[i]);
							break;
						case 1:
							enemy_location[i] = moveDown(enemy_location[i]);
							break;
						case 2:
							enemy_location[i] = enemyMoveLeft(enemy_location[i]);
							break;
					}
				}
			}
			
			break;
	}
	
	return state;
}

enum collision_detection_states {COLLISION_INIT, COLLISION_PLAYING};
	
int collision_tick(int state) {
	switch(state) {
		case COLLISION_INIT:
			if (playFlag == 1) {
				//score = 0;
				state = COLLISION_PLAYING;
			}
			else {
				state = state;
			}
			break;
		case COLLISION_PLAYING:
			if (playFlag == 0) {
				state = COLLISION_INIT;
			}
			else {
				state = state;
			}
			break;
	}
	
	switch(state) {
		case COLLISION_INIT:
			break;
 		case COLLISION_PLAYING:
 			// check if player shot an enemy, inc score, remove projectile, remove enemy
			for (unsigned char i = 0; i < 5; i++) {
				if (player_one_projectile_location[i] != 255) {
					for (unsigned char j = 0; j < 3; j++) {
						if (enemy_location[j] != 255) {
							if (player_one_projectile_location[i] == enemy_location[j]) {
								set_PWM(PROJECTILEHITENEMYSOUND);
								score++;
								player_one_projectile_location[i] = 255;
								enemy_location[j] = 255;
							}
							
						}
					}
				}
			}
			
			// terrain collision detection
			static unsigned char terrainCollisionCounter = 0;
			for (unsigned char i = 0; i < 16; i++) {
				unsigned char terrain_location = i + 16;
				unsigned char terrain_sprite = terrain[(i + terrainShift) % 32];
				
				if ((terrain_sprite != ' ') && (terrain_location == player_one_location)) {
					terrainCollisionCounter++;
					// make sure we only count lives lost once
					if (terrainCollisionCounter < 2) {
						if (lives > 1) {
							set_PWM(PLAYERHITTERRAINSOUND);
							lives--;
						} else {
						gameOverFlag = 1;
						}
					} else {
						terrainCollisionCounter = 0;
					}
				}
				
			}
			
			// enemy collision detection
			static unsigned char enemyCollisionCounter = 0;
			for (unsigned char i = 0; i < 3; i++) {
				if (player_one_location == enemy_location[i]) {
					// make sure we only count lives lost once
					if (enemyCollisionCounter < 2) {
						if (lives > 1) {
							set_PWM(PLAYERHITENEMYSOUND);
							lives--;
							} else {
							gameOverFlag = 1;
						}
					} else {
						enemyCollisionCounter = 0;
					}
				}
			}
			break;
 	}
	return state;
}

int main() {
	// Set Data Direction Registers
	DDRA = 0xFF; PORTA = 0x00; //output
	DDRB = 0xFF; PORTB = 0x00; //output
	DDRC = 0xF0; PORTC = 0x0F; // PC7..4 outputs init 0s, PC3..0 inputs init 1
	DDRD = 0xFF; PORTD = 0x00; //output

	// Period for the tasks
	unsigned long int key_tick_calc = 200;
	unsigned long int display_tick_calc = 100;
	unsigned long int game_start_tick_calc = 100;
	unsigned long int player_one_tick_calc = 200;
	unsigned long int player_one_projectile_tick_calc = 200;
	unsigned long int terrain_tick_calc = 400;
	unsigned long int enemy_tick_calc = 400;
	unsigned long int collision_tick_calc = 200;

	//Calculating GCD
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(key_tick_calc, display_tick_calc);
	tmpGCD = findGCD(tmpGCD, game_start_tick_calc);
	tmpGCD = findGCD(tmpGCD, player_one_tick_calc);
	tmpGCD = findGCD(tmpGCD, player_one_projectile_tick_calc);
	tmpGCD = findGCD(tmpGCD, terrain_tick_calc);
	tmpGCD = findGCD(tmpGCD, enemy_tick_calc);
	tmpGCD = findGCD(tmpGCD, collision_tick_calc);

	//Greatest common divisor for all tasks or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;

	//Recalculate GCD periods for scheduler
	unsigned long int key_tick_period = key_tick_calc/GCD;
	unsigned long int display_tick_period = display_tick_calc/GCD;
	unsigned long int game_start_tick_period = game_start_tick_calc/GCD;
	unsigned long int player_one_tick_period = player_one_tick_calc/GCD;
	unsigned long int player_one_projectile_tick_period = player_one_projectile_tick_calc/GCD;
	unsigned long int terrain_tick_period = terrain_tick_calc/GCD;
	unsigned long int enemy_tick_period = enemy_tick_calc/GCD;
	unsigned long int collision_tick_period = collision_tick_calc/GCD;

	//Declare an array of tasks 
	static task task1, task2, task3, task4, task5, task6, task7, task8;
	task *tasks[] = { &task1, &task2, &task3, &task4, &task5, &task6, &task7, &task8};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	// Task 1
	task1.state = FETCHKEY;//Task initial state.
	task1.period = key_tick_period;//Task Period.
	task1.elapsedTime = 0;//Task current elapsed time.
	task1.TickFct = &key_tick;//Function pointer for the tick.

	// Task 2
	task2.state = DISPLAY_START_MESSAGE;//Task initial state.
	task2.period = display_tick_period;//Task Period.
	task2.elapsedTime = 0;//Task current elapsed time.
	task2.TickFct = &lcd_display_tick;//Function pointer for the tick.

	// Task 3
	task3.state = GAME_INIT;//Task initial state.
	task3.period = game_start_tick_period;//Task Period.
	task3.elapsedTime = 0;//Task current elapsed time.
	task3.TickFct = &game_start_tick;//Function pointer for the tick.

	// Task 4
	task4.state = TERRAIN_INIT;//Task initial state.
	task4.period = terrain_tick_period;//Task Period.
	task4.elapsedTime = 0;//Task current elapsed time.
	task4.TickFct = &terrain_tick;//Function pointer for the tick.

	// Task 5
	task5.state = PLAYER_ONE_INIT;//Task initial state.
	task5.period = player_one_tick_period;//Task Period.
	task5.elapsedTime = 0;//Task current elapsed time.
	task5.TickFct = &player_one_tick;//Function pointer for the tick.

	// Task 6
	task6.state = PLAYER_ONE_PROJECTILE_INIT;//Task initial state.
	task6.period = player_one_projectile_tick_period;//Task Period.
	task6.elapsedTime = 0;//Task current elapsed time.
	task6.TickFct = &player_one_projectile_tick;//Function pointer for the tick.
	
	// Task 7
	task7.state = ENEMY_INIT;//Task initial state.
	task7.period = enemy_tick_period;//Task Period.
	task7.elapsedTime = 0;//Task current elapsed time.
	task7.TickFct = &enemy_tick;//Function pointer for the tick.
		
	// Task 8
	task8.state = COLLISION_INIT;//Task initial state.
	task8.period = collision_tick_period;//Task Period.
	task8.elapsedTime = 0;//Task current elapsed time.
	task8.TickFct = &collision_tick;//Function pointer for the tick.

	// Set the timer and turn it on
	TimerSet(GCD);
	TimerOn();

	// initialize LCD and sprites
	LCD_init();
 	loadPlayerOneSprite();
	loadTerrainSprite();
	loadEnemySprite();

	// initialize RNG
	srand(time(NULL));
	
	// initialize PWM
	PWM_on();

	unsigned short i; // Scheduler for-loop iterator
	while(1) {
		// Scheduler code
		for ( i = 0; i < numTasks; i++ ) {
			// Task is ready to tick
			if ( tasks[i]->elapsedTime == tasks[i]->period ) {
				// Setting next state for task
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				// Reset the elapsed time for next tick.
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 1;
		}
		while(!TimerFlag);
		TimerFlag = 0;
	}

	// Error: Program should not exit!
	return 0;
}
