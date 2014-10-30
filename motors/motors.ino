// Config Bluetooth
// SM,3
// SR,000666098992
// C
#include <SoftwareSerial.h>  
#include <math.h>

int bluetoothTx = 3;  // TX-O pin of bluetooth mate, Arduino D2
int bluetoothRx = 2;  // RX-I pin of bluetooth mate, Arduino D3
SoftwareSerial bluetooth(bluetoothTx, bluetoothRx);

// Config Motors
int dirL = 4;
int pulL = 5; 

int dirR = 6;
int pulR = 7;

// Delay after motor pulse hightand low
int delayHigh = 200;
int delayLow = 1500;

// Canvas size
long xMax = 2800; // x max
long yMax = 2130; // y max

// Current position in mm
long currentX = 1;
long currentY = 1;

// Current wires length in mm
float currentD1 = 0.0;
float currentD2 = 0.0;

// Config serial buffer global
String serialBuffer = "";

void setup() {
  // Setup serial line
  Serial.begin(9600);  // Begin the serial monitor at 9600bps

  // Setup motor pins
  pinMode(dirL, OUTPUT);
  pinMode(pulL, OUTPUT);  
  pinMode(dirR, OUTPUT);
  pinMode(pulR, OUTPUT);  

  // Setup bluetooth
//  bluetooth.begin(115200);  // The Bluetooth Mate defaults to 115200bps
//  bluetooth.print("$");  // Print three times individually
//  bluetooth.print("$");
//  bluetooth.print("$");  // Enter command mode
//  delay(100);  // Short delay, wait for the Mate to send back CMD
//  bluetooth.println("U,9600,N");  // Temporarily Change the baudrate to 9600, no parity
//  // 115200 can be too fast at times for NewSoftSerial to relay the data reliably
  bluetooth.begin(19200);  // Start bluetooth serial at 9600
  
  // Set triangular D1 and D2  to cartesian X0 and Y0
  setZero();
}

// Compute step length

// Motor step length in mm based on wire roll out
float currentD1StepLength(){
  return 0.3;
}

float currentD2StepLength(){
  return 0.27;
}


// Rolling motors, assuming that left is up and right is down
 
// d1 -
void leftStepLeft() {
  digitalWrite(dirL, LOW); // Set the direction.
  digitalWrite(pulL, LOW); // Make pulse
  digitalWrite(pulL, HIGH);
  delayMicroseconds(delayHigh);
  digitalWrite(pulL, LOW);
  delayMicroseconds(delayLow);
  currentD1 = currentD1 - currentD1StepLength();
}

// d1 +
void leftStepRight() {
  digitalWrite(dirL, HIGH); // Set the direction.
  digitalWrite(pulL, LOW); // Make pulse  
  digitalWrite(pulL, HIGH);
  delayMicroseconds(delayHigh);
  digitalWrite(pulL, LOW);
  delayMicroseconds(delayLow);
  currentD1 = currentD1 + currentD1StepLength();
}

// d2 -
void rightStepLeft() {
  digitalWrite(dirR, LOW); // Set the direction.
  digitalWrite(pulR, LOW); // Make pulse   
  digitalWrite(pulR, HIGH); 
  delayMicroseconds(delayHigh);
  digitalWrite(pulR, LOW);
  delayMicroseconds(delayLow);
  currentD2 = currentD2 - currentD2StepLength();
}


// d2 +
void rightStepRight() {
  digitalWrite(dirR, HIGH); // Set the direction.
  digitalWrite(pulR, LOW); // Make pulse  
  digitalWrite(pulR, HIGH); 
  delayMicroseconds(delayHigh);
  digitalWrite(pulR, LOW);
  delayMicroseconds(delayLow);
  currentD2 = currentD2 + currentD2StepLength();
}

// Compute left wire length in mm (d1) based on abs coordinates in mm
long computeLeftWireLengthFromAbs(long x, long y){
  long b1 = yMax - (long) y;
  long c1 = (long) x;
  long base1 = (long)c1*c1 + (long)b1*b1 ;
  long d1 = sqrt(base1);
  return d1;
}

// Compute right wire length mm (d2) based on abs coordinates in mm
long computeRightWireLenghtFromAbs(long x, long y){
  long a1 = yMax - (long) y;
  long c2 = xMax - (long) x;
  long base2 = (long)a1*a1 + (long)c2*c2;
  long d2 = sqrt(base2);
  return d2;
}

// Convert absolute cartesian coordinates to relative triangular
void goToAbs(int x, int y){
  float d1 = computeLeftWireLengthFromAbs(x, y);
  float d2 = computeRightWireLenghtFromAbs(x, y);
  
  float deltaD1 = d1 - currentD1;
  float deltaD2 = d2 - currentD2;
  
  float deltaDRatio = (float)deltaD1 / (float)deltaD2;
  
  Serial.println("Wire length d1:");
  Serial.println(d1);
  Serial.println("Wire lenght d2:");
  Serial.println(d2);
  Serial.println("Delta d1:");
  Serial.println(deltaD1);
  Serial.println("Delta d2:");
  Serial.println(deltaD2);
  Serial.println("Delta D ratio:");
  Serial.println(deltaDRatio);

  // perform the move
  while(((int)deltaD1 != 0) || (int)deltaD2 != 0){
    if((int)deltaD1 != 0){
      if(deltaD1 > 0){
        deltaD1 = deltaD1 - currentD1StepLength();
        leftStepRight();
      } else {
        deltaD1 = deltaD1 + currentD1StepLength();
        leftStepLeft();
      }
      //Serial.println("deltaD1");
      //Serial.println(deltaD1);
    }
  
    if((int)deltaD2 != 0){
      if(deltaD2 > 0){
        deltaD2 = deltaD2 - currentD2StepLength();
        rightStepRight();
      } else {
        deltaD2 = deltaD2 + currentD2StepLength();
        rightStepLeft();
      }
      //Serial.println("deltaD2");
      //Serial.println(deltaD2);

    }
  }
  // set current position after successfull move
  currentX = x;
  currentY = y;
}

// Set position of triangular d1 and d2 to cartesian zero
void setZero(){
  int x = 1;
  int y = 1;
  long d1 = computeLeftWireLengthFromAbs(x, y);
  long d2 = computeRightWireLenghtFromAbs(x, y);
  currentD1 = d1;
  currentD2 = d2;
}

// Parsing commands
// Expected command format 'COMMAND:ARG1:ARG2:ARG3'

void parseCommand(String commandString) {
  String delimiter = ":";

  // Command
  int commandEnd = commandString.indexOf(delimiter);
  String command = commandString.substring(0, commandEnd);
  if(commandEnd != -1){
    commandString = commandString.substring(commandEnd + 1);
  } else {
    commandString = "";
  }
  //Serial.println(commandString);

  // Argument 1
  int arg1End = commandString.indexOf(delimiter);
  String arg1 = commandString.substring(0, arg1End);
  if(arg1End != -1){
    commandString = commandString.substring(arg1End + 1);
  } else{ 
    commandString = "";
  }
  //Serial.println(commandString);
  
  // Argument 2
  int arg2End = commandString.indexOf(delimiter);
  String arg2 = commandString.substring(0, arg2End);
  if(arg2End != -1){
    commandString = commandString.substring(arg2End + 1);
  } else{ 
    commandString = "";
  }
  //Serial.println(commandString);

  // Argument 3
  int arg3End = commandString.indexOf(delimiter);
  String arg3 = commandString.substring(0, arg3End);

  evalCommand(command, arg1, arg2, arg3);  
}


// Evaluating commands = mapping commands to functions
void evalCommand(String command, String arg1, String arg2, String arg3){
  if(command == "rightMotor"){
    // arg1 = direction
    // arg2 = steps
    int steps = arg2.toInt(); 
    for(int i = 0; i < steps ;i++){
      if(arg1 == "left"){
        rightStepLeft();
      }
      if(arg1 == "right"){
        rightStepRight();
      }
    }
  }

  if(command == "leftMotor"){
    // arg1 = direction
    // arg2 = steps
    int steps = arg2.toInt(); 
    for(int i=0; i < steps; i++){
      if(arg1 == "left"){
        leftStepLeft();
      }
      if(arg1 == "right"){
        leftStepRight();
      }
    }

  }

  if(command == "goToAbs"){
    goToAbs(arg1.toInt(), arg2.toInt());
    Serial.println("READY");
  }

  if(command == "goToAbsAndPaint"){
    //goToAbs(arg1.toInt(), arg2.toInt());
    bluetooth.println("paint:");
    Serial.println("READY");
  }

  
  if(command == "currentPos"){
    Serial.println("Current position:  " + String(currentX) + ", " + String(currentY));
    Serial.println("Currant d1 and d2:");
    Serial.println(currentD1);
    Serial.println(currentD2);
  }
  
  if(command == "wait"){
    delay(arg1.toInt());
  }
}

void loop(){
  // Bluetooth passthrought
  // Read bluetooh
  if(bluetooth.available()){
    // Send any characters the bluetooth prints to the serial monitor
    char bluetoothChar = bluetooth.read();
        
    // Pass bluetooth module char to serial
    Serial.print(bluetoothChar);  
  }

  // Read serial
  if(Serial.available())  // If stuff was typed in the serial monitor
  {
    // Send any characters the Serial monitor prints to the bluetooth
    char serialChar = Serial.read();

    // If newline incomes on serial, parse buffer
    if(serialChar == '\n'){      
      // Parse buffer to command
      parseCommand(serialBuffer);
      
      // Reset serial buffer
      serialBuffer = "";
    } else {
      serialBuffer += serialChar;   
    }
    
    // Pass serial char to bluetooth module
    bluetooth.print(serialChar);
  }

  // and loop forever and ever!
}

