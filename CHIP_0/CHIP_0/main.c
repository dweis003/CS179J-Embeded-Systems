#include <stdint.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <stdbool.h> 
#include <string.h> 
#include <math.h> 
#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <avr/eeprom.h> 
#include <avr/portpins.h> 
#include <avr/pgmspace.h> 
#include "usart_ATmega1284.h"
#include "bit.h"
 
//FreeRTOS include files 
#include "FreeRTOS.h" 
#include "task.h" 
#include "croutine.h" 


//GLOBAL VARIABLES 

unsigned char ARM_DISARM = 0; // when 0 system is disarmed, when 1 system is armed
unsigned char delay_timing = 0; //0 = 5 sec, 1 = 10 sec, 2 = 15 sec
unsigned char bark_setting = 0; //0 = minor, 1 = major
unsigned char head_movement = 0; //0 = right to left, 1 = left to right
unsigned char systems_go = 0; //0 = do not turn on dog outputs, 1 = turn on dog outputs 
int random_seed_val = 0; // global random seed to be set by main controller FSM

//receive FSM data
unsigned char received_data = 0x00;

//stepper motor FSM variable 
unsigned char phases[4] = {0x0A, 0x05, 0x06, 0x09};
unsigned char p_index = 0;
int numCounter = 0;
int numPhases = 1024; //number of phases needed to rotate 180 deg

//stepper motor controller FSM variables
int temp_random = 0;
unsigned char STEPPER_GO = 0;   //0 don't move, 1 = move
unsigned char STEPPER_DONE = 0; //stepper has moved forward and back to initial position

//servo motor FSM variables
unsigned char right = 0;	// A0
unsigned char center = 1;
unsigned char left = 0;		// A2
unsigned char max_servo = 0;
unsigned char min_servo = 0;
unsigned char servo_counter = 0;
unsigned char timeline = 1;	// left 0, center = 2, right = 4
unsigned short servo_direction = 0; //0 = center, 1 = left, 2 = right
int temp_random2 = 0;

//IR BEAM FSM variables
unsigned char beam_detected = 0; //0=no IR break detected, 1 = IR break detected

//motion FSM variables
unsigned char motion_detected = 0; //0=no motion detected, 1 = motion detected
int motion_boot_cnt = 0; //system takes approximately 15 seconds to boot so cnt to 20000 

//master control fsm varibales
int controll_counter = 0; //used for the delay in output activation
int vishal_counter = 0; //used for 2 second delay after vishal signal ends
int delay_sec = 5; //5, 10 or 15 seconds

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//random number function
int generate_random_num(){
	//srand(seed);
	int temp = rand() % 10; //gives value between 0 - 9
	return temp;
}

//FSMs Begin
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//stepper motor FSM
enum MotorState {m_wait, m_forward, m_reset} motor_state;


void Motor_Init(){
	motor_state = m_wait;
}

void Motor_Tick(){
	//Actions
	switch(motor_state){
		case m_wait:
		//do nothing
		break;

		case m_forward:
			PORTB = phases[p_index];
			if(p_index == 3){
				p_index = 0;
			}
				
			else{
				++p_index;
			}
		break;

		case m_reset:
			PORTB = phases[p_index];
			if(p_index == 0){
				p_index = 3;
			}
			
			else{
				--p_index;
			}
		break;

		default:
		break;
	}
	//Transitions
	switch(motor_state){
		case m_wait:
		if(STEPPER_GO == 0){ 
			motor_state = m_wait;
		}
		else{ //else turn on motors
			motor_state = m_forward;
			//reset to original values
			p_index = 0;
			numCounter = 0;
		}
		break;

		case m_forward:
		++numCounter; //increment counter
		if(numCounter < numPhases){
			motor_state = m_forward;
		}
		else{
			motor_state = m_reset;
			p_index = 3; //set to index to 3 because going backwards
			numCounter = 0; //reset
		}
		
		break;

		case m_reset:
		++numCounter; //increment counter
		if(numCounter < numPhases){
			motor_state = m_reset;
		}
		else{
			motor_state = m_wait; //go back to waiting
			STEPPER_DONE = 1; //set to done
			STEPPER_GO = 0;
		}
		break;
		
		default:
		motor_state = m_wait;
		break;
	}
}

void MotorSecTask()
{
	Motor_Init();
	for(;;)
	{
		Motor_Tick();
		vTaskDelay(1);
	}
}

void MotorSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(MotorSecTask, (signed portCHAR *)"MotorSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//stepper controller motor FSM
enum STEPPERCONTROLLERState {Stepper_controller_off_mode, Stepper_controller_wait, Stepper_controller_on} stepper_controller_state;

void stepper_controller_Init(){
	stepper_controller_state = Stepper_controller_off_mode;
}

void stepper_controller_Tick(){
	//Actions
	switch(stepper_controller_state){
		//no actions in state
		case Stepper_controller_off_mode:
		break;
		
		case Stepper_controller_wait:
		break;
		
		case Stepper_controller_on:
		break;
		
		default:
		break;
	}
	//Transitions
	switch(stepper_controller_state){
		case Stepper_controller_off_mode:
		if(systems_go == 0){
			stepper_controller_state = Stepper_controller_off_mode; //do not turn on stepper motors
		}
		else{
			stepper_controller_state = Stepper_controller_wait;  //prepare motors for action
		}
		break;
		
		case Stepper_controller_wait:
		if(systems_go == 0){
			stepper_controller_state = Stepper_controller_off_mode; //do not turn on stepper motors go back to waiting
		}
		else{
			temp_random = generate_random_num();
			if((temp_random == 7)){ //number generator gives number between 0-9 so 10% chance of turning on stepper
				STEPPER_GO = 1; //tell motor FSM to go
				stepper_controller_state =  Stepper_controller_on;
			}
			else{
				stepper_controller_state = Stepper_controller_wait;
			}
		}
		break;
		
		case Stepper_controller_on:
			//finish movement
			if((STEPPER_DONE == 0) && (systems_go == 0)){
				//if the stepper has finished moving go back to waiting
				stepper_controller_state = Stepper_controller_on;
			}
			else if((STEPPER_DONE == 1) && (systems_go == 1)){ //reset stepper done to 0){
				STEPPER_DONE = 0; //clear
				stepper_controller_state = Stepper_controller_wait;
			}
			else if((STEPPER_DONE == 1) && (systems_go == 0)){ //reset stepper done to 0){
				STEPPER_DONE = 0; //clear
				stepper_controller_state = Stepper_controller_off_mode;
			}
			else{ //stepper
				//stepper hasn't finish moving and system still on stay here
				stepper_controller_state =  Stepper_controller_on;
			}
		break;
		
		default:
		stepper_controller_state = Stepper_controller_off_mode;
		break;
	}
}

void  stepper_controllerSecTask()
{
	stepper_controller_Init();
	for(;;)
	{
		 stepper_controller_Tick();
		vTaskDelay(500);
	}
}

void Start_stepper_controller_Pulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(stepper_controllerSecTask, (signed portCHAR *)"stepper_controllerSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//servo motor FSM
enum SERVO_DRIVERState {servo_read} servo_driver_state;

void Servo_Driver_Init(){
	servo_driver_state = servo_read;
}

void SERVO_DRIVE_Tick(){
	//Actions
	switch(servo_driver_state){
		case servo_read:
			//move left
			if(servo_direction == 1){
					right = 0;
					center = 0;
					left = 1;
					max_servo = 1;
				
			}
		
			//move right
			else if(servo_direction == 2){				
					right = 1;
					center = 0;
					left = 0;
					max_servo = 2;
				
			}
		
			//move to center
			else{
					right = 0;
					left = 0;
					center = 1;
					max_servo = 0;
			}
		
			break;
		
		default:
			break;
	}
	//Transitions
	switch(servo_driver_state){
		case servo_read:
			servo_driver_state = servo_read;
			break;
		
		default:
			servo_driver_state = servo_read;
			break;
	}
}

void SERVODRIVESecTask()
{
	Servo_Driver_Init();
	for(;;)
	{
		SERVO_DRIVE_Tick();
		vTaskDelay(1);
	}
}

void SERVODRIVESecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(SERVODRIVESecTask, (signed portCHAR *)"SERVODRIVESecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 enum SERVOState {servo_init, drive_high} servo_state;

 void SERVO_Init(){
	 servo_state = servo_init;
 }

void SERVO_Tick(){
	//Transitions
	switch(servo_state){
		case servo_init:
			if(left || right){
				servo_state = drive_high;
				servo_counter = 0;
				//PORTC = 0x01;
				//PORTC = PORTC & 0xFD; //Set Pin C1 low
				PORTC = PORTC | 0x01; //set Pin C0 high
				//SetBit(PORTC,0,1);
			}
		  
			else{
				//PORTC = 0x00;
				//PORTC = PORTC | 0x20; //set PIN C1 to high
				PORTC = PORTC & 0xFE; //set PIN C0 to low
				
				//SetBit(PORTC,0,0);
				servo_state = servo_init;
			}
			break;
			
		case drive_high:
			if((servo_counter < max_servo) && (left || right)){
				 ++servo_counter;
				servo_state = drive_high;
			}
			
			else{
				 left = 0;
				 center = 0;
				 right = 0;
				 //PORTC = 0x00;
				//PORTC = PORTC | 0x20; //set PIN C1 to high
				PORTC = PORTC & 0xFE; //set PIN C0 to low
				 servo_state = servo_init;
			}
			break;
		
		default:
			servo_state = servo_init;
			break;
	  }
	  
	//Actions
	switch(servo_state){
		case servo_init:
			break;
		  
		case drive_high:
			if(center){
				left = 0;
				center = 0;
				right = 0;
				//PORTC= 0x00;
				PORTC = PORTC & 0xFE;
				servo_state = servo_init;
			}
			break;
/*
		case drive_low:
			break;
		  */
		default:
			break;
	}
	
	
  }


 void SERVOSecTask()
 {
	 SERVO_Init();
	 for(;;)
	 {
		 SERVO_Tick();
		 vTaskDelay(1);
	 }
 }

 void SERVOSecPulse(unsigned portBASE_TYPE Priority)
 {
	 xTaskCreate(SERVOSecTask, (signed portCHAR *)"SERVOSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
 }
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Servo controller FSM
enum SERVOCONTROLLERState {Servo_controller_off_mode, Servo_controller_wait, Servo_controller_on} servo_controller_state;

void servo_controller_Init(){
	servo_controller_state = Servo_controller_off_mode;
}

void servo_controller_Tick(){
	//Actions
	switch(servo_controller_state){
		//no actions in state
		case Servo_controller_off_mode:
		break;
		
		case Servo_controller_wait:
		break;
		
		case Servo_controller_on:
		break;
		
		default:
		break;
	}
	//Transitions
	switch(servo_controller_state){
		case Servo_controller_off_mode:
		if(systems_go == 0){
			servo_controller_state = Servo_controller_off_mode; //do not turn on servo motors
		}
		else{
			servo_controller_state = Servo_controller_wait;  //prepare motors for action
		}
		break;
		
		case Servo_controller_wait:
		if(systems_go == 0){
			servo_controller_state = Servo_controller_off_mode; //do not turn on servo motors go back to waiting
		}
		else{
			temp_random2 = generate_random_num();
			if((temp_random2 >= 8)){ //number generator gives number between 0-9 so 20% chance of turning on servo
				servo_controller_state =  Servo_controller_on;
				//0 = right to left, 1 = left to right
				if(head_movement == 1){ //left to right
					servo_direction = 1; //move left
				}
				else{                 //right to left
					servo_direction = 2; //move right
				}
			}
			else{
				servo_controller_state = Servo_controller_wait;
			}
		}
		break;
		
		case Servo_controller_on:
		temp_random2 = generate_random_num();
		//if true move head backto initial position
		if((temp_random2 >= 3) || systems_go == 0){ //number generator gives number between 0-9 so 70% chance of turning on servo or if off go back
			servo_controller_state =  Servo_controller_wait;
			//0 = right to left, 1 = left to right
			if(head_movement == 1){ //left to right
				servo_direction = 2; //move back to right
			}
			else{                 //right to left
				servo_direction = 1; //move back to left
			}
		}
		else{
			servo_controller_state =  Servo_controller_on;
		}
		break;
		
		default:
		servo_controller_state = Servo_controller_off_mode;
		break;
	}
}

void  servo_controllerSecTask()
{
	servo_controller_Init();
	for(;;)
	{
		servo_controller_Tick();
		vTaskDelay(500);
	}
}

void Start_servo_controller_Pulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(servo_controllerSecTask, (signed portCHAR *)"servo_controllerSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//audio FSM
enum AUDIOState {audio_off,audio_on} audio_state;

void AUDIO_Init(){
	audio_state = audio_off;
}

void Audio_Tick(){
	//Actions
	switch(audio_state){
		case audio_off:
			PORTC = PORTC|0x0C; //set pins C2,C3 to high so audio won't play
		break;
		
		case audio_on:
			//major bark
			if(bark_setting == 1){
				PORTC = PORTC & 0xF7; //set PIN C3 to low so audio will play
			}
			//minor bark
			else{
				PORTC = PORTC & 0xFB; //set PIN C2 to low so audio will play
			}
		break;
		
		default:
		//no action
		break;
	}
	//Transitions
	switch(audio_state){
		case audio_off:
			if(systems_go == 1){
				audio_state = audio_on;
			}
			else{
				audio_state = audio_off;
			}
		break;
		
		case audio_on:
			if(systems_go == 1){
				audio_state = audio_on;
			}
			else{
				audio_state = audio_off;
			}
		break;
		
		default:
			audio_state = audio_off;
		break;
	}
}

void AudioSecTask()
{
	AUDIO_Init();
	for(;;)
	{
		Audio_Tick();
		vTaskDelay(10);
	}
}

void StartAudioPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(AudioSecTask, (signed portCHAR *)"AudioSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}




/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//IR BEAM FSM
enum BeamState {Beam_off, Beam_on} beam_state;

void Beam_Init(){
	beam_state = Beam_off;
}

void Beam_Tick(){
	//Actions
	switch(beam_state){
		case Beam_off:
		//reset value
		beam_detected = 0;
		break;
		
		case Beam_on:
		if((GetBit(~PINA,0) == 1)){
			beam_detected = 1; //if soemthing crosses path or IR Beam set to 1
		}
		else{
			beam_detected = 0;
		}
		
		break;

		default:
		//no action
		break;
	}
	//Transitions
	switch(beam_state){
		case Beam_off:
		//if system is on read beam
		if(ARM_DISARM == 1){
			beam_state = Beam_on;
		}
		//if system is off don't read beam
		else{
			beam_state = Beam_off;
		}
		break;
		
		case Beam_on:
		//if system is on read beam
		if(ARM_DISARM == 1){
			beam_state = Beam_on;
		}
		//if system is off don't read beam
		else{
			beam_state = Beam_off;
		}
		break;

		default:
		beam_state = Beam_off;
		break;
	}
}

void BEAMSecTask()
{
	Beam_Init();
	for(;;)
	{
		Beam_Tick();
		vTaskDelay(10);
	}
}

void StartBeamPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(BEAMSecTask, (signed portCHAR *)"BEAMSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//motion FSM
//motion_detected
//motion_boot_cnt
enum MOTIONState {Motion_boot_up,Motion_off,Motion_on} motion_state;

void Motion_Init(){
	motion_state = Motion_boot_up; 
}

void Motion_Tick(){
	//Actions
	switch(motion_state){
		case Motion_boot_up:
		++motion_boot_cnt; //increment cntr
		break;
		
		case Motion_off:
		//no action
		break;
		
		case Motion_on:
		if((GetBit(PINA,2) == 1)){
			motion_detected = 1; //if motion detected set to 1
			//PORTB = 0xff;
		}
		else{
			motion_detected = 0; //no motion detected set to 1
			//PORTB = 0x00;
		}
		break;
		
		default:
		//do nothing
		break;
	}
	//Transitions
	switch(motion_state){
		case Motion_boot_up:
		//give the sensor 20 secs to boot up
		if(motion_boot_cnt < 20000){
			motion_state = Motion_boot_up;
		}
		else{
			motion_state = Motion_off;
			motion_boot_cnt = 0;
		}
		break;
		
		case Motion_off:
		//if system is on read sensor, else off
		if(ARM_DISARM == 1){
			motion_detected = 0; //clear value
			motion_state = Motion_on;
		}
		else{
			motion_state = Motion_off;
		}
		break;
		
		case Motion_on:
		//if system is on read sensor, else off
		if(ARM_DISARM == 1){
			motion_state = Motion_on;
		}
		else{
			motion_state = Motion_off;
			//motion_detected = 0;
		}
		break;
		
		default:
		motion_state = Motion_off;
		break;
	}
}

void MotionSecTask()
{
	Motion_Init();
	for(;;)
	{
		Motion_Tick();
		vTaskDelay(1);
	}
}

void StartMotionPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate( MotionSecTask, (signed portCHAR *)" MotionSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Master Controller FSM
enum ControlState {controll_off,control_on,Vishal,Vishal_wait,motion_wait_1,motion_wait_2,activate_ouptut} control_state;

void Control_Init(){
	control_state = controll_off;
}

void Control_Tick(){
	//Actions
	switch(control_state){
		case controll_off:
		PORTB = PORTB & 0x0F;
		PORTB = PORTB | 0xF0;
		systems_go = 0;
		
		break;
		
		case control_on:
			++random_seed_val;
			if(random_seed_val >= 9999){
				random_seed_val = 0;
			}
			//set delay 
			if(delay_timing == 0){
				delay_sec = 5;
			}
			else if(delay_timing == 1){
				delay_sec = 10;
			}
			else{
				delay_sec = 15;
			}
			//PORTB = 0x01//
			PORTB = PORTB & 0x0F; //set bit 7-4 low
			PORTB = PORTB | 0x10; //set bit 4 high
		break;
		
		case Vishal:
			//PORTB = 0xFF;
		break;
		
		case Vishal_wait:
			//increment counter
			++vishal_counter; 
		
		break;
		
		case motion_wait_1:
			//PORTB = 0x02;
			PORTB = PORTB & 0x0F; //set bit 7-4 low
			PORTB = PORTB | 0x20; //set bit 4 high
			++controll_counter;
		break;
		
		case motion_wait_2:
			//PORTB = 0x03;
			PORTB = PORTB & 0x0F; //set bit 7-4 low
			PORTB = PORTB | 0x30; //set bit 4,5 high
			++controll_counter;
		break;
		
		case activate_ouptut:
			//PORTB = 0x04;
			PORTB = PORTB & 0x0F; //set bit 7-4 low
			PORTB = PORTB | 0x40; //set bit 5 high
			++controll_counter;
		break;
		
		default:
		//no action
		break;
	}
	//Transitions
	switch(control_state){
		case controll_off:
		if(ARM_DISARM==1){
			control_state = control_on;
			//reset all values
			systems_go = 0;
			controll_counter = 0; 
			vishal_counter = 0;
			random_seed_val = 0;
		}
		else{
			control_state = controll_off;
		}
		break;
		
		case control_on:
			if(ARM_DISARM == 0){
				control_state = controll_off;
			}
			//vishal signal
			else if((GetBit(PINA,7) == 1)){
				control_state = Vishal; 
				vishal_counter = 0;
			}
			else if(motion_detected == 1){
				//motion has been detected 
				control_state =  motion_wait_1;
				//reset counter
				controll_counter = 0; 
				//set srand
				 srand(random_seed_val);
			}
			else{
				//else stay here
				control_state = control_on;
			}
		break;
		
		case Vishal:
			//if disabled go to off state
			if(ARM_DISARM == 0){
				control_state = controll_off;
			}
			//if vishal's signal stay here
			else if((GetBit(PINA,7) == 1)){
				control_state = Vishal; 
			}
			else{
				//now wait five seconds after signal stops
				control_state = Vishal_wait; 
			}
		break;
		
		case Vishal_wait:
			if(ARM_DISARM == 0){
				control_state = controll_off;
			}
			else if(vishal_counter < 5000){
				//wait for 5 seconds after vishal signal 
				control_state = Vishal_wait; 
			}
			else{
				//go back to control on now
				control_state = control_on;
			}
		break;
		
		case motion_wait_1:
			if((ARM_DISARM == 0)){
				control_state = controll_off;
			}
			else if((motion_detected == 1) && (beam_detected == 0)){
				control_state = motion_wait_1;
			}
			else if((motion_detected == 1) && (beam_detected == 1)){
				control_state = motion_wait_2;
			}
			else{
				control_state = controll_off;
			}
		
		break;
		
		case motion_wait_2:
			if((ARM_DISARM == 0)){
				//reset counter
				controll_counter = 0; 
				control_state = controll_off;
			}
			else if(controll_counter >= (delay_sec * 1000)){
				control_state = activate_ouptut; 
				systems_go = 1; 
			}
			else if((motion_detected == 1) || (beam_detected == 1)){
				control_state = motion_wait_2;
			}
			else{
				//reset counter
				controll_counter = 0;
				control_state = controll_off;
				controll_counter = 0;
			}
		break;
		
		case activate_ouptut:
			//if off button pressed turn off outputs
			if(ARM_DISARM == 0){
				control_state = controll_off;
			}
			//if timer is less than 20 secs or motion detected stay on 
			else if((controll_counter < 20000) || (motion_detected == 1)){
				control_state = activate_ouptut;
			}
			//if timer is over 30 seconds and no motion turn off outputs
			else if((controll_counter >= 20000) && (motion_detected == 0)){
				//go back to on state
				control_state = control_on;
				controll_counter = 0;
				systems_go = 0;
			}
		break;
		
		default:
		//no action
		break;
	}
}

void ControlSecTask()
{
	Control_Init();
	for(;;)
	{
		Control_Tick();
		vTaskDelay(1);
	}
}

void StartControlPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(ControlSecTask, (signed portCHAR *)"ControlSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//RECEIVE FSM

enum RECState {Rec_Wait, Receive_State } rec_state;

void REC_Init(){
	rec_state = Rec_Wait;
}

void Rec_Tick(){
	//Actions
	switch(rec_state){
		case Rec_Wait:
		break;


		case Receive_State:
		//ARM_DISARM = USART_Receive(0); //receive data
		 received_data = USART_Receive(0);
		 //set global variables
		 //ARM_DISARM  when 0 system is disarmed, when 1 system is armed
		 //delay_timing 0 = 5 sec, 1 = 10 sec, 2 = 15 sec
		 //bark_setting 0 = minor, 1 = major
		 //head_movement 0 = right to left, 1 = left to right
		 
		 //set arm_disarm
		 if(GetBit(received_data,0)==1){ //system is armed
			 ARM_DISARM = 1;
			 //PORTB = PORTB | 0x40; //set pin B6 high
			 //PORTB = 0xFF;
			 
		 }
		 else{                           //system is disarmed
			 ARM_DISARM = 0;
			 //PORTB = 0x00;
			 //PORTB = PORTB & 0xBF; //set pin B6 low
			 
			 //set delay timing
			 if(GetBit(received_data,1) == 1){ //delay 10 second
				 delay_timing = 1;
			 }
			 else if(GetBit(received_data,2) == 1){//delay 15 second
				 delay_timing = 2;
			 }
			 else{                           //delay 5 second
				 delay_timing = 0;
			 }
			 
			 //set bark settings
			 if(GetBit(received_data,3)==1){ //Major Bark
				 bark_setting = 1;
			 }
			 else{                           //Minor Bark
				 bark_setting = 0;
			 }
			 
			 //set dog head movement
			 if(GetBit(received_data,4)==1){ //Left to Right
				 head_movement = 1;
			 }
			 else{                           //Right to left
				 head_movement = 0;
			 }
		 }
		 
		  
	
		USART_Flush(0); //flush so flag reset
		break;
		
		default:
		break;
	}
	//Transitions
	switch(rec_state){
		case Rec_Wait:
			if(USART_HasReceived(0)){
				rec_state = Receive_State; //if ready go to next state
			}
			else{
				rec_state = Rec_Wait; //if not ready to receive data stay
			}
		break;


		case Receive_State:
			rec_state = Rec_Wait; //go back 
		break;
		
		default:
		rec_state = Rec_Wait;
		break;
	}

}


void RecSecTask()
{
	REC_Init();
	for(;;)
	{
		Rec_Tick();
		vTaskDelay(10);
	}
}

void RecSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(RecSecTask, (signed portCHAR *)"RecSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}









//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 
int main(void) 
{ 
   DDRA = 0x00; PORTA=0xFF; //set to input
   DDRB = 0xFF; PORTB = 0x00; //set to output
   DDRC = 0xFF; PORTC = 0x00;
   PORTC = 0x0C; //set pins C2,C3 to high so audio wont play
   //DDRD = 0xFF; PORTD = 0x00; //used by USART 0
   initUSART(0);//Initialize USART 0
   //Start Tasks  
   RecSecPulse(1);
   StartBeamPulse(1);
   StartMotionPulse(1);
   StartControlPulse(1);
   
   MotorSecPulse(1);
   Start_stepper_controller_Pulse(1);
   Start_servo_controller_Pulse(1);
   SERVODRIVESecPulse(1);
   SERVOSecPulse(1);
   StartAudioPulse(1);
    //RunSchedular 
   vTaskStartScheduler(); 
 
   return 0; 
}
