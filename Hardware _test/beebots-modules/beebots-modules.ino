#include <Arduino.h>
#include <Servo.h>

#include "src/motor.h"
#include "src/encoders.h"
#include "src/twiddle.h"
#include "src/PID.h"
#include "src/MPU6050.h"

#define RADIUS 0.036
#define LENGTH 0.325
#define UINT_MAX 4294967295
#define TICKS_PER_REV 3690.0
#define ENCODER_LEFT_PIN 19
#define ENCODER_RIGHT_PIN 21
#define POSITION_COMPUTE_INTERVAL 150
#define SEND_INTERVAL 100
#define TARGET_RPM 150

bool set_;
double set = 120;
int lpos, rpos;
double pidRightCorrection, pidLeftCorrection;
volatile unsigned int leftTicks, rightTicks;
double prevX = 0, prevY = 0;
volatile int inChar = 4;
unsigned long prevPositionComputeTime = 0, prevSendTime = 0;
int start = 0;

/*Position control*/
float posX = 0, posY = 0, orientation = 0;
float targetPosX = 0, targetPosY = 0, targetOrientation = 0, finalOrientation = 0;
float currentDistance = 0, targetDistance = 0;

Servo arm;
Motor motor;

enum State
{
  IDLE,
  TURNING_TO_FACE_TARGET,
  MOVING_TOWARDS_TARGET
} currentState = IDLE;

/*Control Parameters */
//38.58	34.48	0.00
//-.22 -.23
//float Kpl = 0.001, Kil = 0.001, Kdl = 0.001; ///left
float Kpl = 0.22, Kil = 0.001, Kdl = 0.23; ///left
//float Kpr = 0.001, Kir = 0.001, Kdr = 0.001; //right
float Kpr = 0.22, Kir = 0.001, Kdr = 0.23; //right
float p_el = 0, p_er = 0;
float sm_el = 0, sm_er = 0;
int out_max = 255, out_min = 0;

void pulseLeft();
void pulseRight();

Encoders encoders(pulseLeft, pulseRight);

PID_ pidRight(&encoders.rrpm, &pidRightCorrection, &set, Kpr, Kir, Kdr);
PID_ pidLeft(&encoders.lrpm, &pidLeftCorrection, &set, Kpl, Kil, Kdl);
void pulseLeft() 
{
  encoders.incrementleftticks();
}
void pulseRight() 
{
  encoders.incrementrightticks();
}

char states[][10] 
{
  "IDLE",
  "TURNING",
  "MOVING"
};

void configureSET() 
{
  Serial.println("Enter SET: ");
  while (!Serial.available());
  set = Serial.readString().toFloat();
}

void configureRightPID() 
{
  Serial.println("Enter KPR: ");
  while (!Serial.available());
  Kpr = Serial.readString().toFloat();

  Serial.println("Enter KDR: ");
  while (!Serial.available());
  Kdr = Serial.readString().toFloat();

  Serial.println("Enter KIR: ");
  while (!Serial.available());
  Kir = Serial.readString().toFloat();

  Serial.print("Setting values: ");
  Serial.print(Kpr);
  Serial.print(", ");
  Serial.print(Kdr);
  Serial.print(", ");
  Serial.print(Kir);
  Serial.print("\n");

  pidRight.SetTunings(Kpr, Kdr, Kir);
}

void configureLeftPID() 
{
  Serial.println("Enter KPL: ");
  while (!Serial.available());
  Kpl = Serial.readString().toFloat();

  Serial.println("Enter KDL: ");
  while (!Serial.available());
  Kdl = Serial.readString().toFloat();

  Serial.println("Enter KIL: ");
  while (!Serial.available());
  Kil = Serial.readString().toFloat();

  Serial.print("Setting values: ");
  Serial.print(Kpl);
  Serial.print(", ");
  Serial.print(Kdl);
  Serial.print(", ");
  Serial.print(Kil);
  Serial.print("\n");

  pidLeft.SetTunings(Kpl, Kdl, Kil);
}

void pick()
{
  arm.write(70);  

  // pick for bot1 - 15
  // pick for bot2 - 70
  // pick for bot3 - 
}

void place()
{
  arm.write(20);

  // place for bot1 - 45
  // place for bot2 - 20
}

void setup()
{
  Serial.begin(115200);
  float flusher = 3.14;

  // configureRightPID();

  arm.attach(7);
  //place();

  //MPU6050::init();

   	motor.setrightspeed(0);
   	motor.setleftspeed(0);
   	Serial.println("left");
   	Twiddle::autoTune(encoders.lrpm, set, pidLeft, pidLeftCorrection, &Motor::addToLeftSpeed, &motor, encoders);
   	motor.setrightspeed(0);
   	motor.setleftspeed(0);
   	Serial.println("right");
   	Twiddle::autoTune(encoders.rrpm, set, pidRight, pidRightCorrection, &Motor::addToRightSpeed, &motor, encoders);
   	motor.setrightspeed(0);
   	motor.setleftspeed(0);
    motor.forward();
    motor.setleftspeed(255);
    motor.setrightspeed(255);
}

void updatePosition()
{
  float wheelRadius = RADIUS;
  float axleLength = LENGTH;

  static float prevTime = micros();
  float deltaTime = (micros() - prevTime) / 1000000.0f;
  float rpm = (encoders.lrpm + encoders.rrpm) / 2;

  float distanceTravelled = (motor.lpos + motor.rpos) / 2 * deltaTime * rpm * wheelRadius * 2 * PI;
  float angleTurned = (motor.rpos - motor.lpos) / 2 * deltaTime * rpm * 2 * PI / 60;
  float angle = angleTurned * wheelRadius * 2 / axleLength;
  currentDistance += distanceTravelled;

  orientation += angle;
  if (orientation > 2 * PI)
    orientation -= 2 * PI;
  else if (orientation < 0)
    orientation += 2 * PI;
  posX += cos(orientation) * distanceTravelled;
  posY += sin(orientation) * distanceTravelled;

  prevTime = micros();
}

void printPosition()
{
  Serial.print("lrpm- ");
  Serial.print(encoders.lrpm);
  Serial.print(" Left correction- ");
  Serial.print(pidLeftCorrection);
  Serial.print(" left voltage- ");
  Serial.print(motor.getLeftVoltage());
  Serial.print("\t rrpm- ");
  Serial.print(encoders.rrpm);
  Serial.print(" Right correction ");
  Serial.print(pidRightCorrection);
  Serial.print(" right voltage ");
  Serial.println(motor.getLeftVoltage());
}


void findAngle()
{
  targetOrientation = atan((targetPosY - posY) / (targetPosX - posX));

  if ((targetPosY - posY) == 0) //X-Axis
  {
    if (targetPosX - posX > 0)
      targetOrientation = 0;
    else
      targetOrientation = PI;
  }
  else if ((targetPosX - posX) == 0) //Y-Axis
  {
    if (targetPosY - posY > 0)
      targetOrientation = PI / 2;
    else
      targetOrientation = 0.75 * PI;
  }
  else if (((targetPosY - posY) > 0) && ((targetPosX - posX) > 0)) // 1st QUADRANT
    targetOrientation = targetOrientation;
  else if (((targetPosY - posY) > 0) && ((targetPosX - posX) < 0)) // 2nd QUADRANT
    targetOrientation += PI;
  else if (((targetPosY - posY) < 0) && ((targetPosX - posX) < 0)) // 3rd QUADRANT
    targetOrientation += PI;
  else if (((targetPosY - posY) < 0) && ((targetPosX - posX) > 0)) // 4th QUADRANT
    targetOrientation += 2 * PI;

  if (targetOrientation > 2 * PI)
    targetOrientation -= 2 * PI;

  if (targetOrientation < 0)
    targetOrientation += 2 * PI;
}

void setTarget(float x, float y, float o)
{
  targetPosX = x;
  targetPosY = y;
  finalOrientation = o;
  findAngle();
  targetDistance = distanceToTarget();
  currentDistance = 0;

  currentState = TURNING_TO_FACE_TARGET;
}

float distanceToTarget() 
{
  return sqrt(square(posY - targetPosY) - square(posX - targetPosX));
}

void loop()
{
  //MPU6050::update();
  static unsigned long t = millis();
  static bool b = false;
  
//   if  (currentState != IDLE)
  {
    pidRight.Compute();
    pidLeft.Compute();

    pidRightCorrection = -round(pidRightCorrection);
    pidLeftCorrection = -round(pidLeftCorrection);

    motor.addToLeftSpeed(pidLeftCorrection);
    motor.addToRightSpeed(pidRightCorrection);
  }
  if (millis() - prevPositionComputeTime > POSITION_COMPUTE_INTERVAL)
  {
    encoders.computeRPM();
    prevPositionComputeTime = millis();
  }

  updatePosition();
  printPosition();

  switch (currentState)
  {
    case TURNING_TO_FACE_TARGET:

      if (abs(orientation - targetOrientation) > 0.1)
      {
        motor.leftturn();
      }
      else 
      {
        motor.brake();
        currentState = MOVING_TOWARDS_TARGET;
      }
      break;
    case MOVING_TOWARDS_TARGET:
      if (currentDistance < targetDistance) 
      {
        motor.forward();
      }
      else 
      {
        motor.brake();
        Serial.print("brake");
        currentState = IDLE;
      }
      break;
  }

  static int angle = 0;

  if (Serial.available()) 
  {
    char c = Serial.read();
    switch (c) 
    {
      case '1':
        setTarget(0, 100, 0);
        break;
      case '0':
        angle = max(angle - 10, 0);
        pick();
        break;
      case '2':
        angle = min(angle + 10, 180);
        place():
        break;
    }
  }
  // Serial.print("\t");
  // Serial.print(leftTicks);
  // Serial.print("\t");
  // Serial.println(pidRightCorrection);
  // Serial.println(angle);

  //Serial.print(states[currentState]);
  //Serial.print("\t");
  //Serial.print(orientation);
  //Serial.print("/");
  //Serial.print(targetOrientation);
  //Serial.print("\t");
  //Serial.print(currentDistance);
  //Serial.print("/");
  //Serial.print(targetDistance);
  //Serial.print("rrpm:");
  //Serial.println(encoders.rrpm );
  //Serial.print("lrpm");
  //Serial.println(encoders.lrpm );
  //Serial.print(MPU6050::kalAngleX);
  //Serial.print("\t");
  //Serial.print(MPU6050::kalAngleY);
  //Serial.print("\n");
  delay(10);
}
