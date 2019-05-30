#include <PWMServo.h>

PWMServo servo_x;
PWMServo servo_y;

const int servo_x_pwm_min = 550;
const int servo_x_pwm_max = 2200;
const int servo_x_angle_min = 0;
const int servo_x_angle_max = 180;

const int servo_y_pwm_min = 450;
const int servo_y_pwm_max = 2000;
const int servo_y_angle_min = 70;
const int servo_y_angle_max = 160;

int x = 100;
int y = 100;


void setup() {
  servo_y.attach(SERVO_PIN_A, servo_y_pwm_min, servo_y_pwm_max);
  servo_x.attach(SERVO_PIN_B, servo_x_pwm_min, servo_x_pwm_max);
  pinMode(13, OUTPUT);

  Serial.begin(115200);
}

void loop() {
  int move_x = 0;
  int move_y = 0;

  while (Serial.available() > 0) {
    move_x = Serial.parseInt();
    move_y = Serial.parseInt();

    while(Serial.available() > 0) {
      if (Serial.read() == '\n') {
        break;
      }
    }

//    Serial.print("Moving ");
//    Serial.print(move_x);
//    Serial.print(", ");
//    Serial.println(move_y);
  }

  if (move_y && (!servo_y.attached())) {
      servo_y.attach(SERVO_PIN_A, servo_y_pwm_min, servo_y_pwm_max);
  }
  if (move_x && (!servo_x.attached())) {
      servo_x.attach(SERVO_PIN_B, servo_x_pwm_min, servo_x_pwm_max);
  }

  x += move_x;
  y += move_y;

  if (x > 199) {
    x = 199;
  }
  if (x < 0) {
    x = 0;
  }

  if (y > 199) {
    y = 199;
  }
  if (y < 0) {
    y = 0;
  }

//  Serial.print("Position ");
//  Serial.print(x);
//  Serial.print(", ");
//  Serial.println(y);
    
  servo_x.write(map(x, 0, 199, servo_x_angle_min, servo_x_angle_max));
  servo_y.write(map(y, 0, 199, servo_y_angle_min, servo_y_angle_max));

  delay(100);
  if (servo_x.attached()) {
    servo_x.detach();
  }
  if (servo_y.attached()) {
    servo_y.detach();
  }
}
