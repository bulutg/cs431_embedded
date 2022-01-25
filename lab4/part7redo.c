#include "mbed.h"

#define ARRAY_SIZE 10
#define PAIR_SIZE 3
#define BUTTON_DEBOUNCE_TIME_MS 500
#define LAST_X_MS 5000

int tempVal;
int i; // iteration i

// a global struct to hold the array and current size.
struct mflist { // multi function list
    //int timebtn = 0;
    int timebtn;
	int current_size; // 3. keep current size in a global variable.
	int my_array[ARRAY_SIZE];
	PwmOut* ledptr;
	InterruptIn* btnptr;
	Timer debounce;
	int lastcount;
};

struct mflist myList;

int pop_left(struct mflist* myList){
	if ((myList->current_size) > 0) {
		tempVal = (myList->my_array)[0];
		for(i = 1; i < (myList->current_size); i++) {
			(myList->my_array)[i-1] = (myList->my_array)[i];
		}		
		(myList->current_size) = (myList->current_size) - 1;
		return tempVal;
	}
	return -1;
}

int append_right(struct mflist* myList, int element){
    if((myList->current_size) == ARRAY_SIZE) pop_left(myList);
	if((myList->current_size) < ARRAY_SIZE){
		(myList->my_array[myList->current_size]) = element;
		(myList->current_size) = (myList->current_size)+ 1;
		return element;
	}
	return -1;
}

PwmOut led1(p10);
PwmOut led2(p11);
PwmOut led3(p12);
InterruptIn btn1(p5);
InterruptIn btn2(p6);
InterruptIn btn3(p7);
Timer t;

void buton(struct mflist* myList) {
    if (myList->debounce.read_ms() > BUTTON_DEBOUNCE_TIME_MS) {
        myList->timebtn = t.read_ms();
        myList->debounce.reset();
    }
}

// redo
float foo(struct mflist* myList) {
    float sum = -0.1;
    float oldval = (*(myList->ledptr)).read();
    int current_time = t.read_ms();
    for (int j = 0; j<ARRAY_SIZE; j++){
        if ( current_time <= (myList->my_array)[j] + LAST_X_MS){
            sum += 0.1;
        }
    }
    if (sum == -0.1) return oldval;
    return sum;
}

struct mflist parr[PAIR_SIZE];

void btn1fall(void) { buton(parr); }
void btn2fall(void) { buton(parr+1); }
void btn3fall(void) { buton(parr+2); }


int main() {
    t.start();
    parr[0].debounce.start();
    parr[1].debounce.start();
    parr[2].debounce.start();
    parr[0].ledptr = &led1;
    parr[1].ledptr = &led2;
    parr[2].ledptr = &led3;
    parr[0].btnptr = &btn1;
    parr[1].btnptr = &btn2;
    parr[2].btnptr = &btn3;
    btn1.fall(callback(&btn1fall));
    btn2.fall(callback(&btn2fall));
    btn3.fall(callback(&btn3fall));
    while(1){
        CriticalSectionLock::enable();
        struct mflist* ptr = parr;
        for (int i=0; i<PAIR_SIZE; i++, ptr++ ) {
           if (ptr->timebtn){
                int loop;
                for(loop = 0; loop < ARRAY_SIZE; loop++)
                    printf("%d ", ptr->my_array[loop]);
                printf("%d pressed at %dms\n", i,ptr->timebtn);
                *(ptr->ledptr) = foo(ptr);
                append_right(ptr, ptr->timebtn);
                ptr->timebtn = 0;
            }
        }
        CriticalSectionLock::disable();
        wait_ms(100);
    }
}
