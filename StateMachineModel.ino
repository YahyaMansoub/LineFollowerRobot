// == Motor Pins ==
int IN1 = 9;
int IN2 = 10;
int EN1 = 5;   // left motor

int IN3 = 11;
int IN4 = 3;
int EN2 = 6;   // right motor

// == Line Sensor Pins (DIGITAL) ==
int LEFT_SENSOR   = A0;  // digital output from left sensor
int MIDDLE_SENSOR = A1;  // digital output from middle sensor
int RIGHT_SENSOR  = A2;  // digital output from right sensor

// speeds
int FWD_SPEED   = 150;  // forward speed
int SPIN_SPEED  = 130;  // spin speed for corrections
int SEARCH_SPIN = 110;  // slower spin when searching

// middle-detected counter
int middleSeenCount = 0;
const int MIN_SEEN_FOR_RECOVERY = 3;

// turn state (finite state machine)
bool turningLeft  = false;
bool turningRight = false;

void setup() {
  pinMode(EN1, OUTPUT);
  pinMode(EN2, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(LEFT_SENSOR, INPUT);
  pinMode(MIDDLE_SENSOR, INPUT);
  pinMode(RIGHT_SENSOR, INPUT);

  Serial.begin(9600);
}

// ===== MOTOR HELPERS (signed speeds) =====
void driveSigned(int leftPWM, int rightPWM) {
  leftPWM  = constrain(leftPWM, -255, 255);
  rightPWM = constrain(rightPWM, -255, 255);

  // LEFT motor
  if (leftPWM >= 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    analogWrite(EN1, leftPWM);
  } else {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(EN1, -leftPWM);
  }

  // RIGHT motor
  if (rightPWM >= 0) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    analogWrite(EN2, rightPWM);
  } else {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(EN2, -rightPWM);
  }
}

void moveForward() {
  driveSigned(FWD_SPEED, FWD_SPEED);
}

void spinLeft() {
  // left backward, right forward
  driveSigned(-SPIN_SPEED, SPIN_SPEED);
}

void spinRight() {
  // left forward, right backward
  driveSigned(SPIN_SPEED, -SPIN_SPEED);
}

void searchLeft() {
  driveSigned(-SEARCH_SPIN, SEARCH_SPIN);
}

void stopMotors() {
  driveSigned(0, 0);
}

// ===== SENSOR HELPER =====
void readSensors(bool &leftGreenOn, bool &midGreenOn, bool &rightGreenOn) {
  int L = digitalRead(LEFT_SENSOR);
  int M = digitalRead(MIDDLE_SENSOR);
  int R = digitalRead(RIGHT_SENSOR);

  // assume: GREEN LED ON => output LOW
  leftGreenOn  = (L == LOW);
  midGreenOn   = (M == LOW);
  rightGreenOn = (R == LOW);

  // debug
  Serial.print("L/M/R: ");
  Serial.print(leftGreenOn ? 1 : 0); Serial.print("/");
  Serial.print(midGreenOn  ? 1 : 0); Serial.print("/");
  Serial.print(rightGreenOn? 1 : 0);
  Serial.print("  seenCount: ");
  Serial.print(middleSeenCount);
  Serial.print("  state: ");
  if (turningLeft)  Serial.print("TL");
  else if (turningRight) Serial.print("TR");
  else Serial.print("FWD/IDLE");
  Serial.println();
}

// ===== MAIN LOOP =====
void loop() {
  bool leftGreenOn, midGreenOn, rightGreenOn;
  readSensors(leftGreenOn, midGreenOn, rightGreenOn);

  // --- highest priority: middle on line -> follow forward, exit turn states ---
  if (midGreenOn) {
    if (middleSeenCount < 1000) middleSeenCount++;
    turningLeft  = false;
    turningRight = false;
    moveForward();
  }
  // --- if we are currently turning right, KEEP turning right until middle is on ---
  else if (turningRight) {
    spinRight();
  }
  // --- if we are currently turning left, KEEP turning left until middle is on ---
  else if (turningLeft) {
    spinLeft();
  }
  // --- not turning yet: decide what to do based on side sensors ---
  else if (rightGreenOn && !leftGreenOn &&
           middleSeenCount >= MIN_SEEN_FOR_RECOVERY) {
    // only right sees line: start turn-right sequence
    stopMotors();
    delay(300);           // pause once before correcting
    turningRight = true;  // from now on, keep spinning right until middle is on
    turningLeft  = false;
    spinRight();          // start spinning this loop
  }
  else if (leftGreenOn && !rightGreenOn &&
           middleSeenCount >= MIN_SEEN_FOR_RECOVERY) {
    // only left sees line: start turn-left sequence
    stopMotors();
    delay(300);
    turningLeft  = true;  // from now on, keep spinning left until middle is on
    turningRight = false;
    spinLeft();
  }
  else if (!leftGreenOn && !midGreenOn && !rightGreenOn &&
           middleSeenCount >= MIN_SEEN_FOR_RECOVERY) {
    // nothing seen, but we were on the line before AND not currently in a turn
    searchLeft();
  }
  else {
    // before first proper detection or weird state
    turningLeft  = false;
    turningRight = false;
    stopMotors();
  }

  delay(10);  // small loop delay
}

