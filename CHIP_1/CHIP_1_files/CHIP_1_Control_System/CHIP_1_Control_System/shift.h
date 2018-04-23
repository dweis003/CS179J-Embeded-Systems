
// Permission to copy is granted provided that this header remains intact. 
// This software is provided with no warranties.

////////////////////////////////////////////////////////////////////////////////


#ifndef SHIFT_H
#define SHIFT_H

#include "bit.h"

//-------------------------------------------------------------------------------------------------------
//shift_function used for driving matrix USE PORT B PINS B0-B3
void transmit_data(unsigned short data) {
	int i;
	for (i = 0; i < 16; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTB = 0x08;
		// set SER = next bit of data to be sent.
		PORTB |= ((data >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTB |= 0x02;
	}// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
	PORTB |= 0x04;
	// clears all lines in preparation of a new transmission
	PORTB = 0x00;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////



#endif //SHIFT_H
