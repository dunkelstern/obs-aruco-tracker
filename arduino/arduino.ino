#include <PWMServo.h>

PWMServo servo_x;
PWMServo servo_y;

// PWM Minimum and Maximum values
const int servo_x_pwm_min = 550;
const int servo_x_pwm_max = 2200;

const int servo_y_pwm_min = 450;
const int servo_y_pwm_max = 2000;

// Minimal and maximal angle of the axis without hitting any surroundings
const int servo_x_angle_min = 0;
const int servo_x_angle_max = 180;

const int servo_y_angle_min = 70;
const int servo_y_angle_max = 160;

// Initial position (0 left, 399 right, 200 centered)
const int max_val = 90;
int x = max_val / 2;
int y = max_val / 2;

// loop counter for servo deactivation
int loop_counter = 0;

void setup() {
  // Using Arduino Uno we only have 2 PWM pins (9 and 10)
  servo_y.attach(SERVO_PIN_A, servo_y_pwm_min, servo_y_pwm_max);
  servo_x.attach(SERVO_PIN_B, servo_x_pwm_min, servo_x_pwm_max);

  // Initialize serial port with fast baudrate
  Serial.begin(115200);
}

void loop() {
  // by default we do not move
  int move_x = 0;
  int move_y = 0;

  while (Serial.available() > 0) {

    // parse first two values separated by comma
    move_x = Serial.parseInt();
    move_y = Serial.parseInt();

    // and ignore the rest of the line
    while(Serial.available() > 0) {
      if (Serial.read() == '\n') {
        break;
      }
    }
  }

  // re-position servos
  if (move_x || move_y) {
//    Serial.print("Moving ");
//    Serial.print(move_x);
//    Serial.print(", ");
//    Serial.println(move_y);

    // re-attach detached servos
    if (move_y && (!servo_y.attached())) {
        servo_y.attach(SERVO_PIN_A, servo_y_pwm_min, servo_y_pwm_max);
    }
    if (move_x && (!servo_x.attached())) {
        servo_x.attach(SERVO_PIN_B, servo_x_pwm_min, servo_x_pwm_max);
    }

    x += move_x;
    y += move_y;

    // clamp value range to 0 to 199
    if (x > max_val) {
      x = max_val;
    }
    if (x < 0) {
      x = 0;
    }

    if (y > max_val) {
      y = max_val;
    }
    if (y < 0) {
      y = 0;
    }

    // update servo position
    servo_x.write(map(x, 0, max_val, servo_x_angle_min, servo_x_angle_max));
    servo_y.write(map(y, 0, max_val, servo_y_angle_min, servo_y_angle_max));
    loop_counter = 0;

  }

  // wait 30 ms
  delay(30);

  // detach servos to avoid jittering and whining after one second
  loop_counter++;
  if (loop_counter >= 33) {
    loop_counter = 0;
    if (servo_x.attached()) {
      servo_x.detach();
    }
    if (servo_y.attached()) {
      servo_y.detach();
    }
  }

}
