/*
// www.lebois-racing.com
4 actuators SRT80
competition control box (3 buttons)
Leonardo only
v1.0
uploaded on 23/08/03
v1.1 : added support for SRT100 actuator as the 5th actuator
v1.2 : added extra safety for the calibration
v1.3 : security feature can be disabled
v1.4 : optimized speed
v1.5 : optimized speed
*/

//#define SECURITYMOD
//#define BUTTONMOD

// WIRING : IT CAN'T BE CHANGED. SAME AS OPENSFX100
#define StepPin1 8
#define StepPin2 9

#define DirPin1 4
#define DirPin2 5

#define RelayPin A0  //used to control the PSU thougth a relay

#define pulseDelay 0  // mostly set the speed of the actuator : the lower the faster. Don't go below 2 or it will fail.
#define directionDelay 2
int calibrationSpeed = 10;  //la vitesse de rotation : de 0 (rapide) à 1000 (lent)

bool calibrated = true;
bool invertCalibrationDirection = true;

unsigned m1Target = 65535, m2Target = 65535;
unsigned m1Position = 65535, m2Position = 65535;
byte pulseM1 = B00000000, pulseM2 = B00000000;
int dir1 = -1, dir2 = -1;  //Will be used to set the motors direction
bool connected = false;
bool servoEnabled = false;


/////
unsigned long homeStartTime = 0;
bool homeStarted = false;

unsigned int calibrationCounter = 0;
int calibrationSpeedDivider = 5;


void setup() {

  Serial.begin(921600);  //To communicate with FlyPT. In FlyPT, put that number in "Serial speed".

  pinMode(RelayPin, OUTPUT);
  digitalWrite(RelayPin, LOW);  //LOW disable the driver

  pinMode(StepPin1, OUTPUT);
  pinMode(DirPin1, OUTPUT);
  digitalWrite(DirPin1, HIGH);


  pinMode(StepPin2, OUTPUT);
  pinMode(DirPin2, OUTPUT);
  digitalWrite(DirPin2, HIGH);

  //disableServo();
  enableServo();
}

void loop() {
  SerialReaderP();  //Get the datas from Simtools
  if (!calibrated) { 
    Calibrate(); 
  };
  moveMotor();
}

void SerialReaderP() {
  byte commandbufferE[10] = { 0 };  // variables pour stocker les valeurs de l'accélérateur et des freins

  while (Serial.available()) {  //To communicate with FlyPT. In FlyPT, put that number in "Serial speed".
    char incomingChar = Serial.read();

    if (incomingChar == 'b') {
      enableServo();
      m1Target = 65535;
      m2Target = 65535;
      m1Position = 65535;
      m2Position = 65535; 
      homeStarted = false;
      homeStartTime = 0;
      calibrated = false; 
    }

    if (incomingChar == 'A') {
      enableServo();
      calibrated = false;
    }

    else if (incomingChar == 'T') {
      if (Serial.available() >= 9) {
        for (int i = 0; i < 10; i++) { commandbufferE[i] = Serial.read(); }
        if (calibrated) {
          m1Target = commandbufferE[0] << 8 | commandbufferE[1];  //because the communication is 8bits, and the data are 16 bits, their is two bytes to combine
          m2Target = commandbufferE[2] << 8 | commandbufferE[3];
        } else {
          m1Target = commandbufferE[0] << 8 | commandbufferE[1];  //because the communication is 8bits, and the data are 16 bits, their is two bytes to combine
          m2Target = commandbufferE[2] << 8 | commandbufferE[3];
          //Calibrate();
        }
      }
    } else if (incomingChar == 'P') {
      if (Serial.available() >= 7) {
        for (int i = 0; i < 8; i++) { commandbufferE[i] = Serial.read(); }
        m1Target = commandbufferE[0] << 8 | commandbufferE[1];  //because the communication is 8bits, and the data are 16 bits, their is two bytes to combine
        m2Target = commandbufferE[2] << 8 | commandbufferE[3];
      }
    } else if (incomingChar == 'C') {
      connected = false;
      disableServo();
      calibrated = false;
      calibrationSpeed = 10;
    }
  }
}

void moveMotor() {
  directionManagerLeonardo();  //set the directions of each motor
  singleStepLeonardo();        //make the motor move one step
}

void directionManagerLeonardo() {
  byte dirChange = 0;

  if ((m1Target > m1Position) && (dir1 == -1)) {  //pin4 /PD4 // on monte
    PORTD &= B11101111;                           // dir = low
    dir1 = 1;
    dirChange = 1;
  }

  if ((m1Target < m1Position) && (dir1 == 1)) {
    PORTD |= B00010000;
    dir1 = -1;
    dirChange = 1;
  }

  if ((m2Target > m2Position) && (dir2 == -1)) {  //pin5
    PORTC &= B10111111;
    dir2 = 1;
    dirChange = 1;
  }

  if ((m2Target < m2Position) && (dir2 == 1)) {
    PORTC |= B01000000;
    dir2 = -1;
    dirChange = 1;
  }

  if (dirChange == 1) {
    delayMicroseconds(directionDelay);
  }
}

void singleStepLeonardo() {
  //pulse is 8, 9, 10, 11
  if (m1Target != m1Position) {  //pin8 /PB4
    pulseM1 = B00010000;
    m1Position += dir1;
  } else {
    pulseM1 = B00000000;
  }

  if (m2Target != m2Position) {  //pin9 /PB5
    pulseM2 = B00100000;
    m2Position += dir2;
  } else {
    pulseM2 = B00000000;
  }

  if ((pulseM1 == B00000000) && (pulseM2 == B00000000))  { return; }
  PORTB |= pulseM1 | pulseM2 ;
   delayMicroseconds(2);
  PORTB &= B00001111;
}
 

void Calibrate() {
  if(!calibrated) {

    if (!homeStarted) {
        homeStarted = true;
        homeStartTime = millis();
    }
 
    m1Target = m1Position - 1; 
    m2Target = m2Position - 1; 
    if (millis() - homeStartTime >= 2000)
    { 
        m1Position = 0;
        m2Position = 0; 
        m1Target = 0; 
        m2Target = 0; 

        calibrated = true;

        disableServo();
        delay(100);
        enableServo();

        Serial.println("HOME COMPLETE");
    }
  }
}
 
void enableServo() {
  connected = true;
  digitalWrite(RelayPin, HIGH);
  servoEnabled = true;
}
void disableServo() {
  digitalWrite(RelayPin, LOW);
  servoEnabled = false;
}

void MoveOneStep() {
  delay(20 + calibrationSpeed);
  delay(20 + calibrationSpeed);
}
