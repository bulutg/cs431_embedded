#include <REG52.H>
#include <stdio.h>              
#include "inttypes.h"

#define MAX_ARRAY_SIZE 12


// a global struct to hold the array and current size.
struct mflist { // multi function list
	short current_size; // 3. keep current size in a global variable.
	short my_array[ MAX_ARRAY_SIZE ];
};

struct mflist myList;

short tempVal;
short i; // iteration i
short result;
sbit P2_0 = P2^0;

// 2. write values to left/right (reject writing if full)
// put current size in a global after success (i.e. return)
// else return -1

short append_left(short element){
	if(myList.current_size < MAX_ARRAY_SIZE){
		for (i = myList.current_size; i >= 0; i--){
			myList.my_array[i] = myList.my_array[i - 1];
		}
		myList.my_array[0] = element;
		myList.current_size = myList.current_size + 1;
		return element;
	}
	return -1;
}
short append_right(short element){
	if(myList.current_size < MAX_ARRAY_SIZE){
		myList.my_array[myList.current_size] = element;
		myList.current_size = myList.current_size + 1;
		return element;
	}
	return -1;
}
short pop_left(void){
	if (myList.current_size > 0) {
		tempVal = myList.my_array[0];
		for(i = 1; i < myList.current_size; i++) {
			myList.my_array[i-1] = myList.my_array[i];
		}		
		myList.current_size = myList.current_size - 1;
		return tempVal;
	}
	return -1;
}
	
short pop_right(void){
	if (myList.current_size > 0) {
		myList.current_size = myList.current_size - 1;
		return myList.my_array[myList.current_size];
	}
	return -1;
}

// 1. read values from left/right (and reject reading when empty) 
// put current size in a global variable after success (i.e return)
// else return -1

short seek_left(void){
	if(myList.current_size > 0){
		return myList.my_array[0];
	}
	return -1;
}
short seek_right(void){
	if(myList.current_size > 0){
		return myList.my_array[myList.current_size -1];
	}
	return -1;
}
short length(void){
	return myList.current_size;
}

void main (void)  {	/* execution starts here after stack init */
  SCON  = 0x50;	/* SCON: mode 1, 8bit UART, enable rcvr */
  TMOD |= 0x20;	/* TMOD: timer 1, mode 2, 8bit reload */
  TH1   = 0xf3;	/* TH1:  reload value for 2400 baud */
  TR1   = 1;	/* TR1:  timer 1 run */
  TI    = 1;	
	
	P2_0 = 1;      /* Configure P2.0 as an input */   

	myList.current_size = 0;
	
	// uart debug
	// printf ("cs :%d\n", myList.current_size);
	
	append_left(0x11);
	append_left(0x22);
	append_left(0x33);
	append_left(0x44);
	append_left(0x55);
	append_left(0x66);
	append_left(0x77);
	append_left(0x88);
	append_right(0xDD);
	append_right(0xCC);
	append_right(0xBB);
	append_right(0xAA);
	
	// test overflow
	append_right(0xEE);
	append_right(0x01);
	
	// UART DEBUG
	//printf ("cs2 :%d\n", myList.current_size); 

  while (1) {
		while(!P2_0); 
		while(P2_0);
		
		result = length();
		//result = pop_right();
		//result = pop_left();
		//result = seek_left();
		//result = seek_right();
	}	
}	

