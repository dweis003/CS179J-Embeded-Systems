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
#include "keypad.h"
#include "io.c" //contains lcd.h file

//FreeRTOS include files
#include "FreeRTOS.h"
#include "task.h"
#include "croutine.h"

//BUTTON Global VARIABLES
unsigned char ARM_DISARM = 0; // 0 = off, 1 = on
unsigned char Button_A = 0; //select button
unsigned char Button_B = 0; //cancel button
unsigned char Button_L = 0; //left button
unsigned char Button_R = 0; //right button

//RECIEVE FSM Local VARIABLES
unsigned char received_value = 0x00;
//TRANSMIT FSM Local VARIABLES
unsigned char data_to_send = 0x00; //0x00 = tell chip 0 disarm, 0xFF tell chip 1 arm

///////////////////////////////////////////////////////////////
//menu Global data
unsigned short counter = 0; //if 5,000 reset to system off menu
unsigned char bark_setting = 0; //0 = minor, 1 = major
unsigned char bark_delay = 0; //0 = 5 sec, 1 = 10 sec, 2 = 15 sec
unsigned char peripheral_on_off = 0; //0 = do not use outside signals to disarm dog, 1 = use outside signals to disarm system

//bluetooth USART FSM
unsigned char bluetooth_arm_disarm = 2; // 0 = disarm, 1 = arm, 2 = no valid input
unsigned char bluetooth_temp = 0x00;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//functions

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FSM to read on/off button
enum BUTTONState {Button_Wait, Button_Press_Wait} button_state;

void BUTTON_Init(){
	button_state = Button_Wait;
}

void BUTTON_Tick(){
	//Actions
	switch(button_state){
		case Button_Wait:
		//no action, actions performed on transition
		break;
		
		case Button_Press_Wait:
		//no action, actions performed on transition
		break;
		
		default:
		//no action, actions performed on transition
		break;
	}
	//Transitions
	switch(button_state){
		case Button_Wait:
		//if one button is pressed, move to next state
		//on off button
		if((GetBit(~PINA,0) == 1) && (GetBit(~PINA,1) == 0) && (GetBit(~PINA,2) == 0) && (GetBit(~PINA,3) == 0) && (GetBit(~PINA,4) == 0)){
			//change device status
			PORTB = PORTB | 0x02; //turn on PIN A1 for beep
			if(ARM_DISARM == 1){
				ARM_DISARM = 0;
			}
			else{
				ARM_DISARM = 1;
			}
			button_state = Button_Press_Wait;
		}
		//Button A
		else if((GetBit(~PINA,0) == 0) && (GetBit(~PINA,1) == 1) && (GetBit(~PINA,2) == 0) && (GetBit(~PINA,3) == 0) && (GetBit(~PINA,4) == 0)){
			//change device status
			PORTB = PORTB | 0x02; //turn on PIN A1 for beep
			Button_A = 1;
			button_state = Button_Press_Wait;
		}
		//Button B
		else if((GetBit(~PINA,0) == 0) && (GetBit(~PINA,1) == 0) && (GetBit(~PINA,2) == 1) && (GetBit(~PINA,3) == 0) && (GetBit(~PINA,4) == 0)){
			//change device status
			PORTB = PORTB | 0x02; //turn on PIN A1 for beep
			Button_B = 1;
			button_state = Button_Press_Wait;
		}
		//Button L
		else if((GetBit(~PINA,0) == 0) && (GetBit(~PINA,1) == 0) && (GetBit(~PINA,2) == 0) && (GetBit(~PINA,3) == 1) && (GetBit(~PINA,4) == 0)){
			//change device status
			PORTB = PORTB | 0x02; //turn on PIN A1 for beep
			Button_L = 1;
			button_state = Button_Press_Wait;
		}
		//Button R
		else if((GetBit(~PINA,0) == 0) && (GetBit(~PINA,1) == 0) && (GetBit(~PINA,2) == 0) && (GetBit(~PINA,3) == 0) && (GetBit(~PINA,4) == 1)){
			PORTB = PORTB | 0x02; //turn on PIN A1 for beep
			//change device status
			Button_R = 1;
			button_state = Button_Press_Wait;
		}
		//else just stay in current state
		else{
			button_state = Button_Wait;
		}
		break;
		
		case Button_Press_Wait:
		PORTB = PORTB & 0xFD; //set PIN A1 low to end beep
		//if any button is being pressed stay here
		if((GetBit(~PINA,0) == 1) || (GetBit(~PINA,1) == 1) || (GetBit(~PINA,2) == 1) || (GetBit(~PINA,3) == 1) || (GetBit(~PINA,4) == 1)){
			button_state = Button_Press_Wait;
		}
		else{
			button_state = Button_Wait;
		}
		break;
		
		default:
		button_state = Button_Wait;
		break;
	}
}

void ButtonTask()
{
	BUTTON_Init();
	for(;;)
	{
		BUTTON_Tick();
		vTaskDelay(10);
	}
}

void StartButtonPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(ButtonTask, (signed portCHAR *)"ButtonTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//menu fsm
enum MENUState {Menu_Init, Menu_On, Menu_Off, Delay_Menu, Delay_5, Delay_10, Delay_15, Bark_Menu, Bark_Minor, Bark_Major, System_Settings, System_Settings_2, Peripheral_Menu, Peripheral_On, Peripheral_Off} menu_state;

void MenuInit(){
	menu_state = Menu_Init;
}

void Menu_Tick(){
	//Actions
	switch( menu_state){
		case Menu_Init:
		
		break;
		
		case Menu_On:
		
		break;
		
		case Menu_Off:
		
		break;
		
		case Delay_Menu:
		//once counter hits 500 reset menu back to off screen
		++counter;
		break;
		
		case Delay_5:
		//once counter hits 500 reset menu back to off screen
		++counter;
		break;
		
		case Delay_10:
		//once counter hits 500 reset menu back to off screen
		++counter;
		break;
		
		case Delay_15:
		//once counter hits 500 reset menu back to off screen
		++counter;
		break;
		
		case Bark_Menu:
		//once counter hits 500 reset menu back to off screen
		++counter;
		break;
		
		case  Bark_Minor:
		//once counter hits 500 reset menu back to off screen
		++counter;
		break;
		
		case  Bark_Major:
		//once counter hits 500 reset menu back to off screen
		++counter;
		break;
		
		case  System_Settings:
		//once counter hits 500 reset menu back to off screen
		++counter;
		break;
		
		case  System_Settings_2:
		//once counter hits 500 reset menu back to off screen
		++counter;
		break;
		
		case  Peripheral_Menu:
		//once counter hits 500 reset menu back to off screen
		++counter;
		break;
		
		case  Peripheral_On:
		//once counter hits 500 reset menu back to off screen
		++counter;
		break;
		
		case  Peripheral_Off:
		//once counter hits 500 reset menu back to off screen
		++counter;
		break;
		
		default:
		//do nothing
		break;
	}
	//Transitions///////////////////////////////////////////////////////
	switch( menu_state){
		case Menu_Init:
		if(ARM_DISARM == 0){
			counter = 0; //clear counter
			LCD_DisplayString(1, "Dog System is   OFF WOOF WOOF");
			LCD_Cursor(33);
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			PORTB = 0x00;
			menu_state = Menu_Off;
		}
		else{
			LCD_DisplayString(1, "Dog System is ONWOOF WOOF");
			LCD_Cursor(33);
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			PORTB = 0x01;
			menu_state = Menu_On;
		}
		break;
		
		case Menu_On:
		//if bluetooth command sent
		if(bluetooth_arm_disarm == 0){
			bluetooth_arm_disarm = 2; //set to default
			ARM_DISARM = 0; //set to 0 aka off
			menu_state = Menu_Init;
		}
		//if kill command given from Paul's/Bauldo's project and peripherals are enabled
		else if((GetBit(PINA,5) == 1) && peripheral_on_off == 1){
			ARM_DISARM = 0; //set to 0 aka off
			menu_state = Menu_Init;
		}
		//if on/off button pressed
		else if(ARM_DISARM == 0){
			menu_state = Menu_Init;
		}
		//else stay here system is on
		else{
			menu_state = Menu_On;
		}
		
		break;
		
		case Menu_Off:
		//if bluetooth command sent
		if(bluetooth_arm_disarm == 1){
			bluetooth_arm_disarm = 2; //set to default
			ARM_DISARM = 1; //set to 0 aka off
			menu_state = Menu_Init;
		}
		//if on/off button pressed
		else if(ARM_DISARM == 1){
			menu_state = Menu_Init;
			bluetooth_arm_disarm = 2;
		}
		else if(Button_R == 1){
			counter = 0; //clear counter
			LCD_DisplayString(1, "Dog timing      setting");
			LCD_Cursor(33);
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			menu_state = Delay_Menu;
		}
		//else stay here system is on
		else{
			menu_state = Menu_Off;
		}
		
		break;
		
		case Delay_Menu:
		//if sitting for 5 seconds go back to off menu
		if(counter >= 500){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog System is   OFF WOOF WOOF");
			LCD_Cursor(33);
			menu_state = Menu_Off;
		}
		//if bluetooth command sent
		else if(bluetooth_arm_disarm == 1){
			bluetooth_arm_disarm = 2; //set to default
			ARM_DISARM = 1; //set to 1 aka on
			menu_state = Menu_Init;
		}
		//if on/off button pressed
		else if(ARM_DISARM == 1){
			bluetooth_arm_disarm = 2;
			menu_state = Menu_Init;
		}
		else if(Button_R == 1){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog bark        setting");
			LCD_Cursor(33);
			menu_state = Bark_Menu;
		}
		else if(Button_L == 1){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog System is   OFF WOOF WOOF");
			LCD_Cursor(33);
			menu_state = Menu_Off;
		}
		else if(Button_A == 1){
			//enter delay menu
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Delay activationby ->5,10,15 sec");
			LCD_Cursor(33);
			menu_state = Delay_5;
		}
		//else stay here
		else{
			menu_state = Delay_Menu;
		}
		
		break;
		
		case Delay_5:
		//if sitting for 5 seconds go back to off menu
		if(counter >= 500){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog System is   OFF WOOF WOOF");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Menu_Off;
		}
		else if(Button_R == 1){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Delay activationby 5,->10,15 sec");
			LCD_Cursor(33);
			menu_state = Delay_10;
		}
		else if(Button_A == 1){
			//selected delay of 5 seconds store and go back to off
			bark_delay = 0; //0 = 5 sec, 1 = 10 sec, 2 = 15 sec
			eeprom_write_byte(1,0);
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog System is   OFF WOOF WOOF");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Menu_Off;
		}
		else if(Button_B == 1){
			//go back to previous menu
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog timing      setting");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Delay_Menu;
		}
		//else stay here
		else{
			menu_state = Delay_5;
		}
		
		break;
		
		case Delay_10:
		//if sitting for 5 seconds go back to off menu
		if(counter >= 500){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog System is   OFF WOOF WOOF");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Menu_Off;
		}
		else if(Button_R == 1){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Delay activationby 5,10,->15 sec");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Delay_15;
		}
		else if(Button_L == 1){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Delay activationby ->5,10,15 sec");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Delay_5;
		}
		else if(Button_A == 1){
			//selected delay of 10 seconds store and go back to off
			bark_delay = 1; //0 = 5 sec, 1 = 10 sec, 2 = 15 sec
			eeprom_write_byte(1,1);
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog System is   OFF WOOF WOOF");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Menu_Off;
		}
		else if(Button_B == 1){
			//go back to previous menu
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog timing      setting");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Delay_Menu;
		}
		//else stay here
		else{
			menu_state = Delay_10;
		}
		
		break;
		
		case Delay_15:
		//if sitting for 5 seconds go back to off menu
		if(counter >= 500){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog System is   OFF WOOF WOOF");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Menu_Off;
		}
		else if(Button_L == 1){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Delay activationby 5,->10,15 sec");
			LCD_Cursor(33);
			menu_state = Delay_10;
		}
		else if(Button_A == 1){
			//selected delay of 15 seconds store and go back to off
			bark_delay = 2; //0 = 5 sec, 1 = 10 sec, 2 = 15 sec
			eeprom_write_byte(1,2);
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog System is   OFF WOOF WOOF");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Menu_Off;
		}
		else if(Button_B == 1){
			//go back to previous menu
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog timing      setting");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Delay_Menu;
		}
		//else stay here
		else{
			menu_state = Delay_15;
		}
		break;
		
		case Bark_Menu:
		//if sitting for 5 seconds go back to off menu
		if(counter >= 500){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog System is   OFF WOOF WOOF");
			LCD_Cursor(33);
			menu_state = Menu_Off;
		}
		//if bluetooth command sent
		else if(bluetooth_arm_disarm == 1){
			bluetooth_arm_disarm = 2; //set to default
			ARM_DISARM = 1; //set to 1 aka on
			menu_state = Menu_Init;
		}
		//if on/off button pressed
		else if(ARM_DISARM == 1){
			bluetooth_arm_disarm = 2;
			menu_state = Menu_Init;
		}
		else if(Button_R == 1){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Peripheral      Settings");
			LCD_Cursor(33);
			menu_state =  Peripheral_Menu;
			
		}
		else if(Button_L == 1){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog timing      setting");
			LCD_Cursor(33);
			menu_state = Delay_Menu;
		}
		else if(Button_A == 1){
			//enter bark menu
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "->Minor Bark      Major Bark");
			LCD_Cursor(33);
			menu_state = Bark_Minor;
		}
		//else stay here
		else{
			menu_state = Bark_Menu;
		}
		
		break;
		
		case  Bark_Minor:
		//if sitting for 5 seconds go back to off menu
		if(counter >= 500){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog System is   OFF WOOF WOOF");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Menu_Off;
		}
		else if(Button_A == 1){
			//selected minor bark
			bark_setting = 0; //0 = minor, 1 = major
			eeprom_write_byte(0,0);
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog System is   OFF WOOF WOOF");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Menu_Off;
		}
		else if(Button_B == 1){
			//go back to previous menu
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog bark        setting");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Bark_Menu;
		}
		else if(Button_R == 1){
			//enter bark menu
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "  Minor Bark    ->Major Bark");
			LCD_Cursor(33);
			menu_state = Bark_Major;
		}
		//else stay here
		else{
			menu_state = Bark_Minor;
		}
		break;
		
		case  Bark_Major:
		//if sitting for 5 seconds go back to off menu
		if(counter >= 500){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog System is   OFF WOOF WOOF");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Menu_Off;
		}
		else if(Button_A == 1){
			//selected major bark
			bark_setting = 1; //0 = minor, 1 = major
			eeprom_write_byte(0,1);
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog System is   OFF WOOF WOOF");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Menu_Off;
		}
		else if(Button_B == 1){
			//go back to previous menu
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog bark        setting");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Bark_Menu;
		}
		else if(Button_L == 1){
			//enter bark menu
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			//Button_R = 0; //right button
			LCD_DisplayString(1, "->Minor Bark      Major Bark");
			LCD_Cursor(33);
			menu_state = Bark_Minor;
		}
		//else stay here
		else{
			menu_state = Bark_Major;
		}
		break;
		
		case  System_Settings:
		if(counter >= 500){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog System is   OFF WOOF WOOF");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Menu_Off;
		}
		//if bluetooth command sent
		else if(bluetooth_arm_disarm == 1){
			bluetooth_arm_disarm = 2; //set to default
			ARM_DISARM = 1; //set to 1 aka on
			menu_state = Menu_Init;
		}
		//if on/off button pressed
		else if(ARM_DISARM == 1){
			bluetooth_arm_disarm = 2;
			menu_state = Menu_Init;
		}
		else if(Button_L == 1){
			//enter bark menu
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Peripheral      Settings");
			LCD_Cursor(33);
			menu_state =  Peripheral_Menu;
		}
		else if(Button_R == 1){
			//enter system settings 2
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			//right to left
			if(peripheral_on_off == 1){
				LCD_DisplayString(1, "Peripheral      are used");
				LCD_Cursor(33);
			}
			//left to right
			else{
				LCD_DisplayString(1, "Peripheral      are not used");
				LCD_Cursor(33);
			}
			menu_state =  System_Settings_2;
		}
		else{
			menu_state = System_Settings;
		}
		break;
		/////////////////////////////////////////////////////////////////////////////////////////
		case  System_Settings_2:
		if(counter >= 500){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog System is   OFF WOOF WOOF");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Menu_Off;
		}
		//if bluetooth command sent
		else if(bluetooth_arm_disarm == 1){
			bluetooth_arm_disarm = 2; //set to default
			ARM_DISARM = 1; //set to 1 aka on
			menu_state = Menu_Init;
		}
		//if on/off button pressed
		else if(ARM_DISARM == 1){
			bluetooth_arm_disarm = 2;
			menu_state = Menu_Init;
		}
		else if(Button_L == 1){
			//enter bark menu
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			/////////////////////////////////////////////////////////////////////////////////////////
			if(bark_setting == 0){
				if(bark_delay == 0){
					LCD_DisplayString(1, "Bark Delay 5 secBark = Minor");
					LCD_Cursor(33);
				}
				else if(bark_delay == 1){
					LCD_DisplayString(1, "Bark Delay 10secBark = Minor");
					LCD_Cursor(33);
				}
				else{
					LCD_DisplayString(1, "Bark Delay 15secBark = Minor");
					LCD_Cursor(33);
				}
			}
			//bark setting = 1
			else{
				if(bark_delay == 0){
					LCD_DisplayString(1, "Bark Delay 5 secBark = Major");
					LCD_Cursor(33);
				}
				else if(bark_delay == 1){
					LCD_DisplayString(1, "Bark Delay 10secBark = Major");
					LCD_Cursor(33);
				}
				else{
					LCD_DisplayString(1, "Bark Delay 15secBark = Major");
					LCD_Cursor(33);
				}
			}
			menu_state =  System_Settings;
		}
		else{
			menu_state = System_Settings_2;
		}
		break;
		/////////////////////////////////////////////////////////////////////////////////////////
		case  Peripheral_Menu:
		//if sitting for 5 seconds go back to off menu
		if(counter >= 500){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog System is   OFF WOOF WOOF");
			LCD_Cursor(33);
			menu_state = Menu_Off;
		}
		//if bluetooth command sent
		else if(bluetooth_arm_disarm == 1){
			bluetooth_arm_disarm = 2; //set to default
			ARM_DISARM = 1; //set to 1 aka on
			menu_state = Menu_Init;
		}
		//if on/off button pressed
		else if(ARM_DISARM == 1){
			bluetooth_arm_disarm = 2;
			menu_state = Menu_Init;
		}
		else if(Button_R == 1){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//print out delay and bark setting
			//bark_setting 0 = minor, 1 = major
			// bark_delay 0 = 5 sec, 1 = 10 sec, 2 = 15 sec
			if(bark_setting == 0){
				if(bark_delay == 0){
					LCD_DisplayString(1, "Bark Delay 5 secBark = Minor");
					LCD_Cursor(33);
				}
				else if(bark_delay == 1){
					LCD_DisplayString(1, "Bark Delay 10secBark = Minor");
					LCD_Cursor(33);
				}
				else{
					LCD_DisplayString(1, "Bark Delay 15secBark = Minor");
					LCD_Cursor(33);
				}
			}
			//bark setting = 1
			else{
				if(bark_delay == 0){
					LCD_DisplayString(1, "Bark Delay 5 secBark = Major");
					LCD_Cursor(33);
				}
				else if(bark_delay == 1){
					LCD_DisplayString(1, "Bark Delay 10secBark = Major");
					LCD_Cursor(33);
				}
				else{
					LCD_DisplayString(1, "Bark Delay 15secBark = Major");
					LCD_Cursor(33);
				}
			}
			menu_state = System_Settings;
		}
		else if(Button_L == 1){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog bark        setting");
			LCD_Cursor(33);
			menu_state = Bark_Menu;
		}
		else if(Button_A == 1){
			//enter bark menu
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "->Peripheral On   Peripheral Off");
			LCD_Cursor(33);
			menu_state = Peripheral_On;
		}
		//else stay here
		else{
			menu_state = Peripheral_Menu;
		}
		break;
		
		case  Peripheral_On:
		//if sitting for 5 seconds go back to off menu
		if(counter >= 500){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog System is   OFF WOOF WOOF");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Menu_Off;
		}
		else if(Button_A == 1){
			//selected right to left
			peripheral_on_off = 1; 
			eeprom_write_byte(2,1);  //address peripheral setting
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog System is   OFF WOOF WOOF");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Menu_Off;
		}
		else if(Button_B == 1){
			//go back to previous menu
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Peripheral      Settings");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state =  Peripheral_Menu;
		}
		else if(Button_R == 1){
			//enter bark menu
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			//Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "  Peripheral On ->Peripheral Off");
			LCD_Cursor(33);
			menu_state = Peripheral_Off;
		}
		//else stay here
		else{
			menu_state = Peripheral_On;
		}
		break;
		
		case  Peripheral_Off:
		//if sitting for 5 seconds go back to off menu
		if(counter >= 500){
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog System is   OFF WOOF WOOF");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Menu_Off;
		}
		else if(Button_A == 1){
			//selected right to left to right
			peripheral_on_off = 0; 
			eeprom_write_byte(2,0);  //address 2 peripheral setting
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Dog System is   OFF WOOF WOOF");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state = Menu_Off;
		}
		else if(Button_B == 1){
			//go back to previous menu
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "Peripheral      Settings");
			LCD_Cursor(33);
			bluetooth_arm_disarm = 2; //clear bluetooth
			menu_state =  Peripheral_Menu;
		}
		else if(Button_L == 1){
			//enter bark menu
			counter = 0; //clear counter
			//clear all buttons
			Button_A = 0; //select button
			Button_B = 0; //cancel button
			Button_L = 0; //left button
			Button_R = 0; //right button
			LCD_DisplayString(1, "->Peripheral On   Peripheral Off");
			LCD_Cursor(33);
			menu_state = Peripheral_On;
		}
		//else stay here
		else{
			menu_state = Peripheral_Off;
		}
		break;
		
		default:
		menu_state = Menu_Init;
		break;
	}
}

void MENUSecTask()
{
	MenuInit();
	for(;;)
	{
		Menu_Tick();
		vTaskDelay(10);
	}
}

void StartMENUPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(MENUSecTask, (signed portCHAR *)"MENUSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FSM to transmit data over USART
enum TRANSState {Trans_Wait, Transmit_State } trans_state;

void TRANS_Init(){
	trans_state = Trans_Wait;
}

void Trans_Tick(){
	//Actions
	switch(trans_state){
		case Trans_Wait:
		//calculate data_to_send
		//bark_setting 0 = minor, 1 = major
		//bark_delay  0 = 5 sec, 1 = 10 sec, 2 = 15 sec
		
		//_ _ _ _ _ _ _ _
		//7 6 5 4 3 2 1 0
		//bit 0 (0 = off, 1 = on)
		//bit 1-2 (00 = 5 sec, 01 = 10 sec, 10 = 15 sec delay)
		//bit 3 (0 = minor bark, 1 = major bark)
		//bit 4 (0 = don't use peripheral, 1 = use peripheral)
		//calculate data_to_send
		
		//system is disarmed
		if(ARM_DISARM == 0){
			//minor bark 0
			if(bark_setting == 0){
				//5 sec delay
				if(bark_delay == 0){
					
					//Peripheral settings
					//0000 0000
					if(peripheral_on_off == 0){
						data_to_send = 0x00;
					}
					//0001 0000
					else{
						data_to_send = 0x10;
					}
				}
				//10 sec delay
				else if(bark_delay == 1){
					if(peripheral_on_off == 0){
						data_to_send = 0x02;
					}
					else{
						data_to_send = 0x12;
					}
				}
				//15 sec delay
				else{
					if(peripheral_on_off == 0){
						data_to_send = 0x04;
					}
					else{
						data_to_send = 0x14;
					}
				}
			}
			//major bark
			else{
				//5 sec delay
				if(bark_delay == 0){
					if(peripheral_on_off == 0){
						data_to_send = 0x08;
					}
					else{
						data_to_send = 0x18;
					}
					
				}
				//10 sec delay
				else if(bark_delay == 1){
					if(peripheral_on_off == 0){
						data_to_send = 0x0A;
					}
					else{
						data_to_send = 0x1A;
					}
				}
				//15 sec delay
				else{
					if(peripheral_on_off == 0){
						data_to_send = 0x0C;
					}
					else{
						data_to_send = 0x1C;
					}
				}
			}
		}
		
		//system is armed
		else{
			//minor bark 0/////////////////////////////////////////////////////////
			if(bark_setting == 0){
				//5 sec delay
				if(bark_delay == 0){
					if(peripheral_on_off== 0){
						data_to_send = 0x01;
					}
					else{
						data_to_send = 0x81;
					}
					
				}
				//10 sec delay
				else if(bark_delay == 1){
					if(peripheral_on_off == 0){
						data_to_send = 0x03;
					}
					else{
						data_to_send = 0x83;
					}
				}
				//15 sec delay
				else{
					if(peripheral_on_off == 0){
						data_to_send = 0x05;
					}
					else{
						data_to_send = 0x85;
					}
				}
			}
			//major bark 1/////////////////////////////////////////////////////////
			else{
				//5 sec delay
				if(bark_delay == 0){
					if(peripheral_on_off == 0){
						data_to_send = 0x09;
					}
					else{
						data_to_send = 0x89;
					}
					
				}
				//10 sec delay
				else if(bark_delay == 1){
					if(peripheral_on_off == 0){
						data_to_send = 0x0B;
					}
					else{
						data_to_send = 0x8B;
					}
				}
				//15 sec delay
				else{
					if(peripheral_on_off == 0){
						data_to_send = 0x0D;
					}
					else{
						data_to_send = 0x8D;
					}
				}
			}
		}
		
		break;


		case Transmit_State:
		//PORTB = data_to_send;
		USART_Send(data_to_send, 0);
		break;
		
		default:
		break;
	}
	//Transitions
	switch(trans_state){
		case Trans_Wait:
		if(USART_IsSendReady(0)){ //if usart is ready proceed to next state
			trans_state = Transmit_State;
		}
		else{
			trans_state = Trans_Wait;
		}
		break;


		case Transmit_State:
		if(USART_HasTransmitted(0)){ //if data transmitted go back to wait
			trans_state = Trans_Wait;
		}
		else{						//else stay till completion
			trans_state = Transmit_State;
		}
		break;
		
		default:
		trans_state = Trans_Wait;
		break;

	}

}


void TransSecTask()
{
	TRANS_Init();
	for(;;)
	{
		Trans_Tick();
		vTaskDelay(10);
	}
}

void TransSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(TransSecTask, (signed portCHAR *)"TransSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
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
		//get data from CHIP 0
		received_value = USART_Receive(0);
		
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





/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bluetooth RECEIVE FSM
enum BLUERECState {Blue_Rec_Wait, Blue_Receive_State } blue_rec_state;

void BLUE_REC_Init(){
	blue_rec_state = Blue_Rec_Wait;
}

void Blue_Rec_Tick(){
	//Actions
	switch(blue_rec_state){
		case Blue_Rec_Wait:
		break;


		case Blue_Receive_State:
		//get data from bluetooth
		//bluetooth_arm_disarm = USART_Receive(1);
		bluetooth_temp = USART_Receive(1);
		if(bluetooth_temp == 0x0F){	//received arm command
			bluetooth_arm_disarm = 1;
		}
		else if(bluetooth_temp == 0xF0){ //received disarm command
			bluetooth_arm_disarm = 0;
		}
		else{
			bluetooth_arm_disarm = 2;  //random or invalid data set to 2
		}
		
		USART_Flush(1); //flush so flag reset
		break;
		
		default:
		break;
	}
	//Transitions
	switch(blue_rec_state){
		case Blue_Rec_Wait:
		if(USART_HasReceived(1)){
			blue_rec_state = Blue_Receive_State; //if ready go to next state
		}
		else{
			blue_rec_state = Blue_Rec_Wait; //if not ready to receive data stay
		}
		break;


		case Blue_Receive_State:
		blue_rec_state = Blue_Rec_Wait; //go back
		break;
		
		default:
		blue_rec_state = Blue_Rec_Wait;
		break;
	}

}


void BlueRecSecTask()
{
	BLUE_REC_Init();
	for(;;)
	{
		Blue_Rec_Tick();
		vTaskDelay(10);
	}
}

void BlueRecSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(BlueRecSecTask, (signed portCHAR *)"BlueRecSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}




///////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(void)
{
	DDRA = 0x00; PORTA = 0xFF; // PA0-4 inputs for control/power buttons
	DDRC = 0xFF; PORTC = 0x00; // set as output for lcd
	DDRD = 0xFF; PORTD = 0x00; // LCD control lines
	DDRB = 0xFF; PORTB = 0x00; //set as output
	
	//(GetBit(~PINA, 0) == 1)
	
	
	//EEPROM
	//eeprom_write_byte(0,0);  //address 0 bark settings
	//eeprom_write_byte(1,0);  //address 1 timing setting
	//eeprom_write_byte(2,0);  //address 1 periperal setting
	

	bark_setting = eeprom_read_byte(0);
	bark_delay = eeprom_read_byte(1);
	peripheral_on_off = eeprom_read_byte(2);


	//LCD init
	LCD_init();
	
	LCD_DisplayString(1, "TEST");
	LCD_Cursor(33);
	//output initial message that dog is off
	//LCD_DisplayString(1, LCD_data[1]);
	
	//init USART
	initUSART(0); //used to communicate to CHIP 0 AKA "dog"
	initUSART(1); //used to communicate via bluetooth
	
	
	//Start Tasks
	StartButtonPulse(1);
	StartMENUPulse(1);
	TransSecPulse(1);
	RecSecPulse(1);

	BlueRecSecPulse(1);
	
	//RunSchedular
	vTaskStartScheduler();
	
	return 0;
}