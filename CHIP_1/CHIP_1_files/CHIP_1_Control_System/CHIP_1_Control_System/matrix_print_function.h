
// Permission to copy is granted provided that this header remains intact. 
// This software is provided with no warranties.

////////////////////////////////////////////////////////////////////////////////


#ifndef MATRIX_PRINT_FUNCTION_H
#define MATRIX_PRINT_FUNCTION_H

#include "bit.h"
#include "shift.h"
#include "matrix.h"

unsigned char tmpB = 0x00;
unsigned char tmpD = 0x00;

void print_matrix(int matrix_array[8][8]) {
	for (unsigned char j = 0; j < 1; ++j) {
		for (unsigned char i = 0; i < 8; ++i) {
			matrix_print(matrix_array, &tmpB, &tmpD, i);
			unsigned short t;
			//mask the values to 16bit short
			t = tmpD;
			t = t | (tmpB << 8);
			transmit_data(t);
		}
	}
}



#endif //MATRIX_PRINT_FUNCTION_H
