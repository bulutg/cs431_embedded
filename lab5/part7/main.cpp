#include "mbed.h"
#include <string>
#include <sstream>
 
#define BLINKING_RATE 100ms
#define MAXIMUM_BUFFER_SIZE 32

using namespace std;
using namespace std::chrono;

Mutex mutex;
ConditionVariable cond(mutex);

PwmOut l4(LED4);
PwmOut l3(LED3);
PwmOut l2(LED2);
PwmOut l1(LED1);

Semaphore ledsem(1);
Semaphore serial_sem(1);
Semaphore heartbeat_sem(1);

Thread t1;
Thread t2;
Thread t3;
Thread t4;
Thread t5;
Timer t;

string my_left = "left";

static BufferedSerial serial_port(USBTX, USBRX);

double ledbright[4] = {0.0, 0.0, 0.0, 0.0};

string str_buf = "";

void safe_print(const char *text)
{
    serial_sem.acquire();
    printf("%s\n\r", text);
    serial_sem.release();
}

void heartbeat_timer_task()
{
    while(true)
    {
        heartbeat_sem.acquire();
        ThisThread::sleep_for(5s);
        heartbeat_sem.release();
    }
}

void heartbeat_task()
{
    while(true)
    {
        heartbeat_sem.acquire();
        string s = "Alive for " + std::to_string(t.elapsed_time().count()/1000000) +" seconds";
        safe_print(s.c_str());
        heartbeat_sem.release();
    }
}


void serial_receiver_task()
{
    char buf[MAXIMUM_BUFFER_SIZE] = {0};
    while(true)
    {
        if (uint32_t num = serial_port.read(buf, sizeof(buf))) {
            str_buf += buf;
            if (*buf == '\n') 
            {
                safe_print(str_buf.c_str());
                mutex.lock();
                cond.notify_all();
                mutex.unlock();
            }
        }
    }
}

void led_br_thread()
{
    while (true) 
    {
        mutex.lock();
        cond.wait();
        ledsem.acquire();

 
        string token;

        stringstream streamData(str_buf.c_str());

        while (getline(streamData, token, ' ')) {  
            if (token == my_left){
                ledbright[0] += 0.1;
                ledbright[1] += 0.1;
                ledbright[2] += 0.1;
                ledbright[3] += 0.1;
            }
        } 



        ledsem.release();
        str_buf = "";
        mutex.unlock();
    }
}

void led_thread()
{
    while (true) 
    {

        ledsem.acquire();
        l1 = ledbright[0];
        l2 = ledbright[1];
        l3 = ledbright[2];
        l4 = ledbright[3];
        ThisThread::sleep_for(100ms);
        ledsem.release();
    }
}

int main()
{
    serial_port.set_baud(9600);
    serial_port.set_format(
         8,
         BufferedSerial::None,
         1
    );
    
    t.start();

    t1.start(callback(serial_receiver_task));
    t2.start(callback(led_br_thread));
    t3.start(callback(led_thread));
    t4.start(callback(heartbeat_task));
    t5.start(callback(heartbeat_timer_task));
}
