
// Permission to copy is granted provided that this header remains intact. 
// This software is provided with no warranties.

////////////////////////////////////////////////////////////////////////////////


#ifndef BIT_H
#define BIT_H


// x: 8-bit value.   k: bit position to set, range is 0-7.   b: set bit to this, either 1 or 0
unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b) {
	if(b){ //b ==1
		return x | (0x01 << k); //   Set bit to 1
	}
	else{ //b ==0
		return x & ~(0x01 << k);   //Set bit to 0
	}
}

//x: 8-bit value.   K: bit position, range is 0-7 return value in location k
unsigned char GetBit(unsigned char x, unsigned char k) {
	return ((x & (0x01 << k)) != 0);
}


#endif //BIT_H
