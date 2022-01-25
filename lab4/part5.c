#include "mbed.h"

Timer t;
PwmOut led(p5);

float foo(Timer* clockbuffer) {
    return (float)((t.read_ms() / 100) % 11)/10;
}

int main() {
    t.start();
    while(1){
        led = foo(&t);
        wait_ms(100);
    }
}