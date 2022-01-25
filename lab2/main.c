#include <REG52.H>            
#include "lcd.h"
#include <stdio.h>               
#include "inttypes.h"
#include <stdlib.h>
#include <ctype.h>

int val;
int tmp;

char keypad_hex[3];
char uart_input;

// keypad 4x2

sbit R1 = P0^3;
sbit R2 = P0^2;
sbit R3 = P0^1;
sbit R4 = P0^0;
sbit C1 = P0^6;
sbit C2 = P0^5;
sbit C3 = P0^4;

char keypad_input;

// https://www.edsim51.com/examples.html#prog4
// lcd connections 
sbit RS = P1^3;
sbit E = P1^2;
sbit DB4 = P1^4;
sbit DB5 = P1^5;
sbit DB6 = P1^6;
sbit DB7 = P1^7;

sbit clear = P2^4;
sbit ret = P2^5;				  
sbit left = P2^6;
sbit right = P2^7;

void returnHome(void);
void entryModeSet(bit id, bit s);
void displayOnOffControl(bit display, bit cursor, bit blinking);
void cursorOrDisplayShift(bit sc, bit rl);
void functionSet(void);
void setDdRamAddress(char address);

void sendChar(char c);
void sendString(char* str);
bit getBit(char c, char bitNumber);
void delay(void);

void delay_funct(int delay_time) {
	TMOD |= 0x01;
	TH0= delay_time;
	TL0 = 0x66;
	TR0 = 1;
	while (TF0 == 0);
	TR0 = 0;
	TF0 = 0;
}

// keypad func

char read_keypad(){
  C1=1; C2=1; C3=1;
  R1=0; R2=1; R3=1; R4=1;
  if(C1==0){delay_funct(100); while(C1==0); return '1';}
  if(C2==0){delay_funct(100); while(C2==0); return '2';}
  if(C3==0){delay_funct(100); while(C3==0); return '3';}
  R1=1; R2=0; R3=1; R4=1;
  if(C1==0){delay_funct(100); while(C1==0); return '4';}
  if(C2==0){delay_funct(100); while(C2==0); return '5';}
  if(C3==0){delay_funct(100); while(C3==0); return '6';}
  R1=1; R2=1; R3=0; R4=1;
  if(C1==0){delay_funct(100); while(C1==0); return '7';}
  if(C2==0){delay_funct(100); while(C2==0); return '8';}
  if(C3==0){delay_funct(100); while(C3==0); return '9';}
  R1=1; R2=1; R3=1; R4=0;
  if(C1==0){delay_funct(100); while(C1==0); return '*';}
  if(C2==0){delay_funct(100); while(C2==0); return '0';}
  if(C3==0){delay_funct(100); while(C3==0); return '#';}
  return 0;
}


void main (void) {
	
functionSet();
entryModeSet(1, 0); // increment and no shift
displayOnOffControl(1, 1, 1); // display on, cursor on and blinking on


SCON  = 0x50;		        /* SCON: mode 1, 8-bit UART, enable rcvr      */
TMOD |= 0x20;               /* TMOD: timer 1, mode 2, 8-bit reload        */
TH1   = 0xFD;                /* TH1:  reload value for 19200 baud @ 11.059MHz   */
TR1   = 1;                  /* TR1:  timer 1 run                          */
TI    = 1;                  /* TI:   set TI to send first char of UART    */
	
PCON = 0x80; // toggle smod = 1 


  while (1) {		
		uart_input = _getkey();
		printf(&uart_input);  
		sprintf(keypad_hex, "%02X", uart_input);
		// print uart as hex
		sendChar(keypad_hex[0]);
		sendChar(keypad_hex[1]);
		tmp = uart_input;
		// wait according to received byte
		delay_funct(tmp);
		while(!(keypad_input = read_keypad()));
		setDdRamAddress(0x40); // set address to start of second line
		sendString("    ");
		setDdRamAddress(0x40); // set address to start of second line
		val = (keypad_input - '0') * 100;
		while(!(keypad_input = read_keypad()));
		val += (keypad_input - '0') * 10;
		while(!(keypad_input = read_keypad()));
		val += (keypad_input - '0');
		sprintf(keypad_hex, "%X", val);
		// print keypad value as hex
		sendString(keypad_hex);
		// reset
		setDdRamAddress(0x0); // set address to start of second line
		sendString("    ");
		setDdRamAddress(0x0); // first line
		keypad_input = '\0';
		uart_input = '\0';
  }
}

// LCD Module instructions -------------------------------------------
// To understand why the pins are being set to the particular values in the functions
// below, see the instruction set.
// A full explanation of the LCD Module: HD44780.pdf

void returnHome(void) {
	RS = 0;
	DB7 = 0;
	DB6 = 0;
	DB5 = 0;
	DB4 = 0;
	E = 1;
	E = 0;
	DB5 = 1;
	E = 1;
	E = 0;
	delay();
}	

void entryModeSet(bit id, bit s) {
	RS = 0;
	DB7 = 0;
	DB6 = 0;
	DB5 = 0;
	DB4 = 0;
	E = 1;
	E = 0;
	DB6 = 1;
	DB5 = id;
	DB4 = s;
	E = 1;
	E = 0;
	delay();
}

void displayOnOffControl(bit display, bit cursor, bit blinking) {
	DB7 = 0;
	DB6 = 0;
	DB5 = 0;
	DB4 = 0;
	E = 1;
	E = 0;
	DB7 = 1;
	DB6 = display;
	DB5 = cursor;
	DB4 = blinking;
	E = 1;
	E = 0;
	delay();
}

void cursorOrDisplayShift(bit sc, bit rl) {
	RS = 0;
	DB7 = 0;
	DB6 = 0;
	DB5 = 0;
	DB4 = 1;
	E = 1;
	E = 0;
	DB7 = sc;
	DB6 = rl;
	E = 1;
	E = 0;
	delay();
}

void functionSet(void) {
	// The high nibble for the function set is actually sent twice. Why? See 4-bit operation
	// on pages 39 and 42 of HD44780.pdf.
	DB7 = 0;
	DB6 = 0;
	DB5 = 1;
	DB4 = 0;
	RS = 0;
	E = 1;
	E = 0;
	delay();
	E = 1;
	E = 0;
	DB7 = 1;
	E = 1;
	E = 0;
	delay();
}

void setDdRamAddress(char address) {
	RS = 0;
	DB7 = 1;
	DB6 = getBit(address, 6);
	DB5 = getBit(address, 5);
	DB4 = getBit(address, 4);
	E = 1;
	E = 0;
	DB7 = getBit(address, 3);
	DB6 = getBit(address, 2);
	DB5 = getBit(address, 1);
	DB4 = getBit(address, 0);
	E = 1;
	E = 0;
	delay();
}

void sendChar(char c) {
	DB7 = getBit(c, 7);
	DB6 = getBit(c, 6);
	DB5 = getBit(c, 5);
	DB4 = getBit(c, 4);
	RS = 1;
	E = 1;
	E = 0;
	DB7 = getBit(c, 3);
	DB6 = getBit(c, 2);
	DB5 = getBit(c, 1);
	DB4 = getBit(c, 0);
	E = 1;
	E = 0;
	delay();
}

// -- End of LCD Module instructions
// --------------------------------------------------------------------

void sendString(char* str) {
	int index = 0;
	while (str[index] != 0) {
		sendChar(str[index]);
		index++;
	}
}

bit getBit(char c, char bitNumber) {
	return (c >> bitNumber) & 1;
}

void delay(void) {
	char c;
	for (c = 0; c < 50; c++);
}


