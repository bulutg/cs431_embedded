#include "mbed.h"

PwmOut led(p5);
const float brightness=1.0;

int main() {
    led = brightness;
}