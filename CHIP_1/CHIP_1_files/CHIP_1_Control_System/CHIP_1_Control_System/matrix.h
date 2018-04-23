
// Permission to copy is granted provided that this header remains intact. 
// This software is provided with no warranties.

////////////////////////////////////////////////////////////////////////////////


#ifndef MATRIX_H
#define MATRIX_H

#include "bit.h"

void matrix_print(int arr[8][8], unsigned char *tmpB, unsigned char *tmpD, unsigned char i){
	*tmpB = 0x00;
	*tmpD = 0x00;
	*tmpB = SetBit(*tmpB,i,1); //set bit i to 1 turns on column
	for(int j = 0; j < 8; ++j){
		if(arr[i][j] == 0){
			*tmpD = SetBit(*tmpD, j, 1); //if location is zero in array set to 1 to turn off
		}
	}


} 




#endif //MATRIX_H
