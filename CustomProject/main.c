#include <avr/io.h>
#include <avr/interrupt.h>
#include <ucr/bit.h>
#include <ucr/timer.h>
#include <stdio.h>
#include <string.h>
#include "io.h"
#include "io.c"


// keypad func
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

//--------End Task scheduler data structure-----------------------------------

//--------Shared Variables----------------------------------------------------
unsigned char playFlag = 0; // if we should stat the game or not

unsigned char key = ' '; // key pressed

// players and their projectiles
unsigned char player_one_location = 0;
unsigned char player_one_projectile_location [5] = {255, 255, 255, 255, 255};

const unsigned char screenBlank[32] = {
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '
}; // 16 x 2 characters

unsigned char screenBuffer[32] = {
								' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
								' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '
}; // 16 x 2 characters
//--------End Shared Variables------------------------------------------------

//--------User defined FSMs---------------------------------------------------
enum key_tick_states {FETCHKEY};
	
int key_tick(int state) {	
	key = GetKeypadKey();
	return state;
}

enum display_tick_states {DISPLAY};

int display_tick(int state) {
	LCD_DisplayString(1, screenBuffer);
	return state;
}

enum game_start_tick_states {GAME_INIT, GAME_PLAYING};
	
int game_start_tick(int state) {
	
	switch(state) {
		case GAME_INIT:
			if (key == 'A') {
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
	}
	
	switch(state) {
		case GAME_INIT:
			playFlag = 0;
			unsigned char startMessage[32] = {
				'P', 'r', 'e', 's', 's', ' ', 'A', ' ', 't', 'o', ' ', 'S', 't', 'a', 'r', 't',
				'P', 'r', 'e', 's', 's', ' ', 'D', ' ', 't', 'o', ' ', 'R', 'e', 's', 'e', 't'
			}; // 16 x 2 characters
			memcpy(screenBuffer, startMessage, sizeof(startMessage));
			break;
		case GAME_PLAYING:
			playFlag = 1;
			memcpy(screenBuffer, screenBlank, sizeof(screenBlank));
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
			
			screenBuffer[player_one_location] = '>';
			break;
	}
	
	return state;
}

enum player_one_projectile_tick_states {PLAYER_ONE_PROJECTILE_INIT, PLAYER_ONE_PROJECTILE_PLAYING};

int player_one_projectile_tick(int state) {
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
			break;
			
		case PLAYER_ONE_PROJECTILE_PLAYING:
			// create projectiles in array
			if (key == '5') {
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
			
			// displays a dash for each projectile
			for (unsigned char i = 0; i < 5; i++) {
				screenBuffer[player_one_projectile_location[i]] = '-';
			}
			break;
	}
	
	return state;
}

// --------END User defined FSMs-----------------------------------------------

// Implement scheduler code from PES.
int main()
{
// Set Data Direction Registers
// Buttons PORTA[0-7], set AVR PORTA to pull down logic
DDRA = 0xFF; PORTA = 0x00; //output
DDRB = 0xFF; PORTB = 0x00; //output
DDRC = 0xF0; PORTC = 0x0F; // PC7..4 outputs init 0s, PC3..0 inputs init 1
DDRD = 0xFF; PORTD = 0x00; //output

LCD_init();

// Period for the tasks
unsigned long int key_tick_calc = 200;
unsigned long int display_tick_calc = 200;
unsigned long int game_start_tick_calc = 200;
unsigned long int player_one_tick_calc = 200;
unsigned long int player_one_projectile_tick_calc = 200;

//Calculating GCD
unsigned long int tmpGCD = 1;
tmpGCD = findGCD(key_tick_calc, display_tick_calc);
tmpGCD = findGCD(tmpGCD, game_start_tick_calc);
tmpGCD = findGCD(tmpGCD, player_one_tick_calc);
tmpGCD = findGCD(tmpGCD, player_one_projectile_tick_calc);

//Greatest common divisor for all tasks or smallest time unit for tasks.
unsigned long int GCD = tmpGCD;

//Recalculate GCD periods for scheduler
unsigned long int key_tick_period = key_tick_calc/GCD;
unsigned long int display_tick_period = display_tick_calc/GCD;
unsigned long int game_start_tick_period = game_start_tick_calc/GCD;
unsigned long int player_one_tick_period = player_one_tick_calc/GCD;
unsigned long int player_one_projectile_tick_period = player_one_projectile_tick_calc/GCD;

//Declare an array of tasks 
static task task1, task2, task3, task4, task5;
task *tasks[] = { &task1, &task2, &task3, &task4, &task5};
const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

// Task 1
task1.state = FETCHKEY;//Task initial state.
task1.period = key_tick_period;//Task Period.
task1.elapsedTime = 0;//Task current elapsed time.
task1.TickFct = &key_tick;//Function pointer for the tick.

// Task 2
task2.state = DISPLAY;//Task initial state.
task2.period = display_tick_period;//Task Period.
task2.elapsedTime = 0;//Task current elapsed time.
task2.TickFct = &display_tick;//Function pointer for the tick.

// Task 3
task3.state = GAME_INIT;//Task initial state.
task3.period = game_start_tick_period;//Task Period.
task3.elapsedTime = 0;//Task current elapsed time.
task3.TickFct = &game_start_tick;//Function pointer for the tick.

// Task 4
task4.state = PLAYER_ONE_INIT;//Task initial state.
task4.period = player_one_tick_period;//Task Period.
task4.elapsedTime = 0;//Task current elapsed time.
task4.TickFct = &player_one_tick;//Function pointer for the tick.

// Task 5
task5.state = PLAYER_ONE_PROJECTILE_INIT;//Task initial state.
task5.period = player_one_projectile_tick_period;//Task Period.
task5.elapsedTime = 0;//Task current elapsed time.
task5.TickFct = &player_one_projectile_tick;//Function pointer for the tick.

// Set the timer and turn it on
TimerSet(GCD);
TimerOn();

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
