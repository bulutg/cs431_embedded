#include "mbed.h"

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
		    printf("p5 pressed at %dms\n", timebtn);
            timebtn = 0;
        }
        CriticalSectionLock::disable();
        wait_ms(100);
    }
}
