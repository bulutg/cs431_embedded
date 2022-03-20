#define MBED_CONF_MBED_TRACE_ENABLE 1
#define MBED_TRACE_MAX_LEVEL TRACE_LEVEL_INFO
#include "mbed.h"
#include "mbed-trace/mbed_trace.h"
#define TRACE_GROUP  "cs431"
#include "stm32f413h_discovery_lcd.h"

typedef void (*func_t)(void);

// motor control pins
PwmOut in1(p21);
PwmOut in2(p22);
PwmOut in3(p23);
PwmOut in4(p24);
// end motor control pins

// simulation variables, do not modify
Ticker lcd_ticker;
// end simulation variables

// ultrasonic sensor emulator class, do not modify
class Ultrasonic
{
  protected:
    AnalogIn* ain;
    Timeout* delayer;
    func_t echo_isr;
  public:
    static const int SPEED_OF_SOUND = 3350;
    Ultrasonic(PinName pin, func_t echo_cb)
    {
      this->ain = new AnalogIn(pin);
      this->delayer = new Timeout();
      this->echo_isr = echo_cb;
    }
    ~Ultrasonic()
    {
      delete ain;
      delete delayer;
    }
    void trigger()
    {
      if(this->echo_isr != NULL)
      {
        const float delay = (20.0+this->ain->read()*400.0)/SPEED_OF_SOUND;
        tr_debug("Ultrasonic: emulating %.0fus delay\n", delay*1e6);
        this->delayer->attach_us(this->echo_isr, delay*1e6);
      }
    }
};

// lcd drawing task that emulates the robot, do not modify
#define MOTOR_ACTIVATION_POWER 0.01
#define ROBOT_HEAD_TO_TAIL_PX  8
#define ROBOT_SPEED_MULTIPLIER 2
#define ROBOT_TURN_MULTIPLIER  0.1
#define MAP MOTOR_ACTIVATION_POWER
void lcd_draw_task()
{
  
  // simulation variables, do not modify
  static float tail_x=BSP_LCD_GetXSize()/2;
  static float tail_y=BSP_LCD_GetYSize()/2;
  static float head_x=BSP_LCD_GetXSize()/2;
  static float head_y=BSP_LCD_GetYSize()/2;
  static float dir = 0;
  static float mleft = 0.0f;
  static float mright = 0.0f;
  
  // clear LCD
  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  BSP_LCD_FillCircle(round(tail_x), round(tail_y), 3);
  BSP_LCD_FillCircle(round(head_x), round(head_y), 3);
  
  // error checking
  if(in1 >= MAP && in2 >= MAP)
  {
    tr_err("Invalid power feed to motors: +in1 & +in2");
    return;
  }
  if(in3 >= MAP && in4 >= MAP)
  {
    tr_err("Invalid power feed to motors: +in3 & +in4");
    return;
  }
  
  // combining inputs into direction
  if(in1 >= MAP)
  {
    mleft = in1;
  }
  else if(in2 >= MAP)
  {
    mleft = -in2;
  }
  else
  {
    mleft = 0.0;
  }

  if(in3 >= MAP)
  {
    mright = in3;
  }
  else if(in4 >= MAP)
  {
    mright = -in4;
  }
  else
  {
    mright = 0.0;
  }
  
  const float finalpow = mleft + mright;
  const float powdiff = mleft - mright;
  // step
  tail_x += sin(dir)*finalpow*2;
  tail_y += cos(dir)*finalpow*2;     
  head_x = tail_x + sin(dir)*ROBOT_HEAD_TO_TAIL_PX;
  head_y = tail_y + cos(dir)*ROBOT_HEAD_TO_TAIL_PX;
  // turn
  dir += powdiff*ROBOT_TURN_MULTIPLIER;
  tr_debug("mright, mleft = [%.2f, %.2f]; finalpow, powdiff = [%.2f, %.2f]; dir = %.2f", mright, mleft, finalpow, powdiff, dir*180/3.14);

  // drawing
  BSP_LCD_SetTextColor(LCD_COLOR_RED);
  BSP_LCD_FillCircle(round(tail_x), round(tail_y), 3);
  BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
  BSP_LCD_FillCircle(round(head_x), round(head_y), 3);
}


// splitted wait function which allows for lcd drawing simulation and ultrasonic sensor emulation to work
// do not use bare wait_ms and instead use this function wherever you may need wait.
// Hint: you should only need this function in your main loop to avoid browser crash. 
// You should not need any busy waits like this!!
void splitted_wait_ms(int delay_ms)
{
  static Timer internalTimer;
  internalTimer.start();
  internalTimer.reset();
  while(internalTimer.read_ms()<delay_ms)
  {
    wait_ms(1);
  }
}

void robot_emulator_init()
{
  mbed_trace_init();     // initialize the trace library
  BSP_LCD_Init();
  /* Clear the LCD */
  BSP_LCD_Clear(LCD_COLOR_WHITE);
  lcd_ticker.attach(lcd_draw_task, 0.2);
  printf("Speed of sound is %d m/s due to JS engine limitations\n", Ultrasonic::SPEED_OF_SOUND);  
}

// USER GLOBAL CODE SPACE BEGIN
struct DirectionAndSpeed {
  float forward;
  float backward;
  float left;
  float right;
  float speed;
};

DirectionAndSpeed dir = {0, 0, 0, 0, 0.5};

EventQueue queue;

Serial bt_serial(USBTX, USBRX); // tx, rx
CircularBuffer<char, 32> serialQ;

void ultrasonic_echo_isr();
Ultrasonic usonic(p19, &ultrasonic_echo_isr);
Timer usonic_timer;
int usonic_distance = 20;

InterruptIn back_button(p5), front_button(p6), left_button(p7), right_button(p8);

InterruptIn stateSwitch(p20);
bool bt_switch = false;

DirectionAndSpeed noiseDir = {0, 0, 0, 0, 1.0};
Timeout noise_button_timeout;
bool noise_received = false;


void motor_controller() {
  CriticalSectionLock::enable();
  if (bt_switch) {
    queue.call_in(200, &motor_controller);
  }
  DirectionAndSpeed tmpDir = dir;
  CriticalSectionLock::disable();
  
  float t1 = tmpDir.forward + tmpDir.left;
  float t2 = tmpDir.backward + tmpDir.right;
  float t3 = tmpDir.forward + tmpDir.right;
  float t4 = tmpDir.backward + tmpDir.left;
  // if the time elapsed from the triggering of the usonic sensor
  // to the usonic isr being run is less than 30, only allow backward motion
  CriticalSectionLock::enable();
  bool obstacle = usonic_distance < 30 ? true : false;
  CriticalSectionLock::disable();
  if (obstacle) {
    CriticalSectionLock::enable();
    in1 = 0;
    in2 = tmpDir.backward;
    in3 = 0;
    in4 = tmpDir.backward;
    CriticalSectionLock::disable();
  }
  else {
      if (t1 > t2) {
          t1 -= t2;
          t2 = 0;
      }
      else {
          t2 -= t1;
          t1 = 0;
      }
      
      if (t3 > t4) {
          t3 -= t4;
          t4 = 0;
      }
      else {
          t4 -= t3;
          t3 = 0;
      }
      
      // set motor powers
      CriticalSectionLock::enable();
      in1 = t1;
      in2 = t2;
      in3 = t3;
      in4 = t4;
      CriticalSectionLock::disable();
  }
  // stop moving if the user hasn't pressed any keys since the last
  // run of the motor_controller task
  CriticalSectionLock::enable();
  dir = (DirectionAndSpeed) {0,0,0,0, dir.speed};
  CriticalSectionLock::disable();
}

void noise_button_timeout_isr() {
    // set direction to the directions received during the timeout is active
    // with a more proper implementation we can 
    dir = noiseDir;
    noiseDir = (DirectionAndSpeed) {0, 0, 0, 0, 1.0};
    noise_received = false;
    noise_button_timeout.detach();
    queue.call(&motor_controller);
    queue.call_in(3000, &motor_controller);
}

const float NOISE_BUTTON_TIME_WINDOW = 1.0f;

void back_noise_isr() {
    // start a timeout if a noise button press is received
    // and set a flag to true, any button presses until the timeout isr runs
    // is included in the noise direction
    if (!noise_received) {
        noise_received = true;
        noise_button_timeout.attach(&noise_button_timeout_isr, NOISE_BUTTON_TIME_WINDOW);
    }
    noiseDir.backward = 1.0f;
}

void front_noise_isr() {
    if (!noise_received) {
        noise_received = true;
        noise_button_timeout.attach(&noise_button_timeout_isr, NOISE_BUTTON_TIME_WINDOW);
    }
    noiseDir.forward = 1.0f;
}

void left_noise_isr() {
    if (!noise_received) {
        noise_received = true;
        noise_button_timeout.attach(&noise_button_timeout_isr, NOISE_BUTTON_TIME_WINDOW);
    }
    noiseDir.left = 1.0f;
    noiseDir.forward = 1.0f;
}

void right_noise_isr() {
    if (!noise_received) {
        noise_received = true;
        noise_button_timeout.attach(&noise_button_timeout_isr, NOISE_BUTTON_TIME_WINDOW);
    }
    noiseDir.right = 1.0f;
    noiseDir.forward = 1.0f;
}

void ultrasonic_echo_isr() {
  usonic_timer.stop();
  usonic_distance = usonic_timer.read_ms();
}

void usonic_task() {
    usonic_timer.reset();
    usonic_timer.start();
    usonic.trigger();
}

void serial_task() {
    char c;
    if (serialQ.pop(c)) {
        // When you hold down multiple keys, the serial terminal sends each of them once
        // and continues sending the last key while you continue holding it down
        // Because of this behaviour we have opted to implement intermediary directions
        // with extra keys, otherwise the user would have to send the 2 keypresses for
        // the intermediary direction after each run of the motor controller task.
        // If we were to implement the serial communications with our own client application,
        // rather than the builtin serial terminal, we could have easily implemented
        // this with just 4 direction keys.
        CriticalSectionLock::enable();
        switch(c) {
            case 'w':
                dir.backward = 0;
                dir.forward = dir.speed;
                dir.left = 0;
                dir.right = 0;
                break;
            case 'q':
                dir.backward = 0;
                dir.forward = dir.speed;
                dir.left = dir.speed;
                dir.right = 0;
                break;
            case 'a':
                dir.backward = 0;
                dir.forward = 0;
                dir.left = dir.speed;
                dir.right = 0;
                break;
            case 'z':
                dir.backward = dir.speed;
                dir.forward = 0;
                dir.left = dir.speed;
                dir.right = 0;
                break;
            case 's':
                dir.backward = dir.speed;
                dir.forward = 0;
                dir.left = 0;
                dir.right = 0;
                break;
            case 'x':
                dir.backward = dir.speed;
                dir.forward = 0;
                dir.left = 0;
                dir.right = dir.speed;
                break;
            case 'd':
                dir.backward = 0;
                dir.forward = 0;
                dir.left = 0;
                dir.right = dir.speed;
                break;
            case 'e':
                dir.backward = 0;
                dir.forward = dir.speed;
                dir.left = 0;
                dir.right = dir.speed;
                break;
            case '1':
                dir.speed = 0.5f;
                bt_serial.printf("Speed set to 1\n");
                break;
            case '2':
                dir.speed = 1.0f;
                bt_serial.printf("Speed set to 2\n");
                break;
        }
        CriticalSectionLock::disable();
        bt_serial.printf("Received: %c\n", c);
    }
}

void serial_isr() {
    char c = bt_serial.getc();
    // if bluetooth mode is off, ignore the character read from serial
    //if (bt_switch && (c == 'w' || c == 'a' || c == 's' || c == 'd' || c == '1' || c == '2')) {
    if (bt_switch) {
        // push the character that was read to the serial queue
        serialQ.push(c);
        // push the serial task to the event queue
        queue.call(serial_task);
    }
}


void switch_bluetooth_isr() {
    bt_serial.printf("Switched to bluetooth\n");
    // disable noise button isrs
    back_button.fall(NULL);
    front_button.fall(NULL);
    right_button.fall(NULL);
    left_button.fall(NULL);
    bt_switch = true;
    queue.call(&motor_controller);
}

void switch_noise_isr() {
    bt_serial.printf("Switched to noise sensors\n");
    // re-enable noise button isrs
    back_button.fall(&back_noise_isr);
    front_button.fall(&front_noise_isr);
    right_button.fall(&right_noise_isr);
    left_button.fall(&left_noise_isr);
    bt_switch = false;
}

// USER GLOBAL CODE SPACE END

int main() 
{
  // DO NOT REMOVE THIS CALL FROM MAIN!
  robot_emulator_init();

  // USER MAIN CODE SPACE BEGIN
  // call the ultrasonic sensor task every second
  queue.call_every(1000, &usonic_task);
  
  // attach ISRs to serial, bluetooth switch and noise buttons
  bt_serial.attach(&serial_isr);
  stateSwitch.rise(&switch_bluetooth_isr);
  stateSwitch.fall(&switch_noise_isr);
  back_button.fall(&back_noise_isr);
  front_button.fall(&front_noise_isr);
  right_button.fall(&right_noise_isr);
  left_button.fall(&left_noise_isr);

  // dispatch events from the event queue forever
  queue.dispatch_forever();
  
  // USER MAIN CODE SPACE END
}