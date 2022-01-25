#include <REG52.H>            
#include "lcd.h"
#include <stdio.h>               
#include "inttypes.h"
#include <stdlib.h>
#include <ctype.h>


char* p;
char receivedChar = 0; 
int tmp = 0;
int sequence_length = 0;
int cancel = 0;
int uart_rr = 0;
int i;
char hardstr[] = {"abc789"};
char buf[16];

sbit EDSIM_WR = P0^7;

void uart_isr() interrupt 4 {	
	if(RI == 1){
		receivedChar = SBUF;
		//If enter was pressed during the flashing of a previous sequence
		// the flashing job should restart with the new sequence.
		if(receivedChar == 0x0A) cancel = 1;
		uart_rr = 1;
		RI = 0;
	}
	else if(TI == 1){
		TI = 0;
	}
}

void char_print(char c){
    SBUF = c;
	  while(TI==0);	
}

void my_print(char *p){
    while(*p) char_print(*p++);
}

void delay_funct(int delay_time) {
	TMOD |= 0x01;
	TL0 = delay_time;
	TL0 = 0x66;
	TR0 = 1;
	while (TF0 == 0);
	TR0 = 0;
	TF0 = 0;
}

// pulse then zero
void pulse(int value){
	P1 = value;
	P1 = 0;
}

// morse lookup table
// Chars that are not numbers should be ignored
void morse(char digit){
	if(digit > 48 && digit < 54){
		// 1 to 5
		for(i = 0; i < digit % 48; i++) pulse(100);
		for(i = 0; i < 5 - digit % 48; i++) pulse(200);
	}
	else if ((digit > 53 && digit < 58) || digit == 48){
		// 6 to 9 and 0
		for(i = 0; i < digit % 53; i++) pulse(200);
		for(i = 0; i < 5 - digit % 53; i++) pulse(100);
	}
}

void handle_uart(){
	if (uart_rr) {
		uart_rr = 0;
	  
		// if newline from serial
		if(cancel){
			cancel = 0;
			
			// set length to zero since it may cancel any time
			tmp = sequence_length;
			sequence_length = 0;
		
			// Demonstrate you are able to save sequences by printing them back when enter is pressed. 
			for(i = 0; i < tmp && !cancel; i = i + 1){
				char_print(buf[i]);
				morse(buf[i]);
			}
			// Demonstrate you can report back to serial when you cancel flashing strings; i.e. when a new sequence is entered before flashing a new one is finished.
			if (cancel) my_print(" > flashing cancelled\n");
			// Demonstrate you can report back to serial when you finish flashing strings.
			else my_print(" > flashing finished\n");
		}
		// Input chars should be buffered internally until enter (\n) is sent from serial
		else {
			// Demonstrate you are able to receive chars by echoing them back. 
			/*
			my_print("received: ");
		  char_print(receivedChar);
		  my_print("\n");
			*/
			buf[sequence_length] = receivedChar;
			sequence_length += 1;
		}

		//char_print(receivedChar);
	}
}



// Demonstrate you are able to flash certain hard-coded chars.
void demo_hardcoded_chars(){
	//morse('1');
	//morse('2');
	morse('3');
}

void flash_string(char* strp){
	for (p = strp; *p != '\0'; p++){
		morse(*p);
	}
	//while(*strp) morse(*strp++);
}

void main (void) {
	SCON  = 0x50;		        /* SCON: mode 1, 8-bit UART, enable rcvr      */
	TMOD |= 0x20;               /* TMOD: timer 1, mode 2, 8-bit reload        */
	TH1   = 0xFD;                /* TH1:  reload value for 19200 baud @ 11.059MHz   */
	TR1   = 1;                  /* TR1:  timer 1 run                          */
	TI    = 1;                  /* TI:   set TI to send first char of UART    */
		
	PCON = 0x80; // toggle smod = 1 

	EDSIM_WR = 0; // enable the DAC WR line
		
	EA = 1;
	ES = 1;

	while (1) {
			handle_uart();
		  //demo_hardcoded_chars();
		  // Demonstrate you are able to flash hard-coded strings.
			flash_string(hardstr);
		}
}

