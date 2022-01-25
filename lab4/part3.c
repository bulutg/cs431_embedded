#include "mbed.h"

#define ARRAY_SIZE 10

int tempVal;
int i; // iteration i

// a global struct to hold the array and current size.
struct mflist { // multi function list
	int current_size; // 3. keep current size in a global variable.
	int my_array[ARRAY_SIZE];
};

struct mflist myList;

int pop_left(void){
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

int append_right(int element){
    if(myList.current_size == ARRAY_SIZE) pop_left();
	if(myList.current_size < ARRAY_SIZE){
		myList.my_array[myList.current_size] = element;
		myList.current_size = myList.current_size + 1;
		return element;
	}
	return -1;
}

InterruptIn btn(p5);
Timer t;

int timebtn = 0;

void btn_p5() {
    timebtn = t.read_ms();
}

int main() {
    t.start();
    btn.fall(callback(&btn_p5));
    while(1){
        CriticalSectionLock::enable();
        if(timebtn) {
            int loop;
            for(loop = 0; loop < ARRAY_SIZE; loop++)
            printf("%d ", myList.my_array[loop]);
		    printf("p5 pressed at %dms\n", timebtn);
		    append_right(timebtn);
            timebtn = 0;
        }
        CriticalSectionLock::disable();
        wait_ms(100);
    }
}
