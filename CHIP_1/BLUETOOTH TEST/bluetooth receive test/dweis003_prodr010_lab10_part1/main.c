/* dweis003_prodr010_lab2_part1.c - [10/5/17]
* Partner(s) Name & E-mail:Donald Weiss dweis003@ucr.edu
*			     Paul Rodriguez prodr010@ucr.edu
* Lab Section: 022
* Assignment: Lab #2 Exercise #1
* Exercise Description:

*
* I acknowledge all content contained herein, excluding template or example
* code, is my own original work.
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include "bit.h"
#include "timer.h"
#include "io.c"
//#include "keypad.h"
#include "usart_ATmega1284.h"
#include <stdio.h>
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
unsigned char received_val = 0x00; //default 
//--------End Shared Variables------------------------------------------------


//--------User defined FSMs---------------------------------------------------
//Enumeration of states.
enum SM1_States { wait, receive};
//follower FSM

int SMTick1(int state) {
	// Local Variables

	//State machine transitions
	switch (state) {
		case wait: 
			if(USART_HasReceived(1)){
				state = receive;
			}
			else{
				state = wait;
			}
		break;
		
	

		case receive:
		state = wait;
		break;


		default: state = wait; // default: Initial state
		break;
	}
	//State machine actions
	switch(state) {
		case wait: 
			PORTB = received_val;
		break;
		

		case receive:
			received_val = USART_Receive(1); //store received value
			USART_Flush(1); //delete received val from register
			PORTB = received_val;
		break;
		
		default: break;
	}
	return state;
}
//Enumeration of states.


// --------END User defined FSMs-----------------------------------------------
// Implement scheduler code from PES.
int main()
{

	DDRB = 0xFF; PORTB = 0x00; // PORTB set to output, outputs init 0s
	//DDRC = 0xF0; PORTC = 0x0F; // PC7..4 outputs init 0s, PC3..0 inputs init 1s


	// . . . etc
	// Period for the tasks
	unsigned long int SMTick1_calc = 20;
	//Calculating GCD
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(SMTick1_calc, 0); //can only calculate 2 at a time. If more than 2 FSM run more that once
	//Greatest common divisor for all tasks or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;
	//Recalculate GCD periods for scheduler
	unsigned long int SMTick1_period = SMTick1_calc/GCD;

	static task task1;
	task *tasks[] = { &task1};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
	// Task 1
	task1.state = -1;//Task initial state.
	task1.period = SMTick1_period;//Task Period.
	task1.elapsedTime = SMTick1_period;//Task current elapsed time.
	task1.TickFct = &SMTick1;//Function pointer for the tick.
	// Set the timer and turn it on
	TimerSet(GCD);
	TimerOn();

	//code to initialize USART



	unsigned short i; // Scheduler for-loop iterator
	initUSART(1);     //init USART
	while(1) {
		// Scheduler code ---------------------------------------------
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
	} //---------------------------------------------------------------
	// Error: Program should not exit!
	return 0;
}