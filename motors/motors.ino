// Config Bluetooth
// SM,3
// SR,000666098992
// C
#include <SoftwareSerial.h>  
#include <math.h>

#include <SD.h>

char configFileName[] = "config.txt";
char dataFileName[] = "data.txt";
char printedFileName[] = "printed.txt";

boolean sdCardInitialized = false;
unsigned long dataFilePosition = 0;

File dataFile;
File configFile;
File printedFile;

String dataBuffer = "";
String configBuffer = "";

// Config bluetooth module pins
int bluetoothTx = 2;
int bluetoothRx = 3;
SoftwareSerial bluetooth(bluetoothTx, bluetoothRx);

// Config Motors pins
int dirL = 7;
int pulL = 8; 

int dirR = 5;
int pulR = 6;

// Config buttons pins
int button1 = 0;
int button2 = 1;
int button3 = 2;
int button4 = 3;

//Config buzzer pin
int buzzer = 4;

// Delay after motor pulse high and low in microseconds
int delayHigh = 1;
int delayLow = 1;

// Canvas size
long xMax = 22500; // x max
long yMax = 11000; // y max

// Current position in mm
long currentX = 1;
long currentY = 1;

// Current wires length in mm
float currentD1 = 0.0;
float currentD2 = 0.0;

// d1 and d2 modulos
float d1Modulo = 0;
float d2Modulo = 0;

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

  pinMode(10, OUTPUT);     // SD card reader SS pin


  bluetooth.begin(19200);  // Start bluetooth serial at 9600

    if (!SD.begin(4)) {
    Serial.println("SD card initialization failed!");
    sdCardInitialized = false;

  } 
  else {
    Serial.println("SD card initialization done.");
    sdCardInitialized = true;

    // Set datafile position based on printed file
    printedFile = SD.open(printedFileName);

    dataFilePosition = printedFile.size();
    printedFile.close();
    Serial.println("Setting data positiont to: " + String(dataFilePosition));

    // Load config
    configFile = SD.open(configFileName);
    if(configFile){      
      char configChar;

      //read line from file
      Serial.println("Reading config file");
      while(configFile.available() ){
        configChar = configFile.read();
        Serial.print(configChar);
        //do nat add newline to buffer
        configBuffer += configChar;
        if(configChar == '\n'){
          parseCommand(configBuffer);
          configBuffer = "";
        }
      }
      Serial.println("Closing config file");
      configFile.close();
    }  
  }

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

// X length from d1 and d2
float computeXLength(float d1, float d2){
  float y = computeYLength(d1, d2);
  float x = sqrt((d1 * d1) -(((float)yMax-y)*((float)yMax-y)));
  return x;
}

// Y length from d1 and d2
float computeYLength(float d1, float d2){
  float p = (xMax + d1 + d2) / 2;
  float h1 = (2.0 / xMax);
  float h2 = sqrt(p*(p-xMax)*(p-d1)*(p-d2));
  float h =  h1 * h2;
  float y = yMax - h; 
  return  y;
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
float computeLeftWireLengthFromAbs(long x, long y){
  float b1 = yMax -  y;
  float c1 = x;
  float base1 = c1*c1 + b1*b1 ;
  float d1 = sqrt(base1);
  return d1;
}

// Compute right wire length mm (d2) based on abs coordinates in mm
float computeRightWireLenghtFromAbs(long x, long y){
  float a1 = yMax - y;
  float c2 = xMax - x;
  float base2 = a1*a1 + c2*c2;
  float d2 = sqrt(base2);
  return d2;
}

// Print info
void printInfo(){
    Serial.println("Current position x and y:");
    Serial.println(currentX);
    Serial.println(currentY);
    Serial.println("Current computed position x and y:");
    Serial.println(computeXLength(currentD1, currentD2));
    Serial.println(computeYLength(currentD1, currentD2));
    Serial.println("Current d1 and d2:");
    Serial.println(currentD1);
    Serial.println(currentD2);
}

// Convert absolute cartesian coordinates to relative triangular
void goToAbs(int x, int y){
  float d1 = computeLeftWireLengthFromAbs(x, y);
  float d2 = computeRightWireLenghtFromAbs(x, y);
  Serial.println("Compute target d1 and d2: ");
  Serial.println(d1);
  Serial.println(d2);
  float deltaD1 = d1 - currentD1;
  float deltaD2 = d2 - currentD2;

  // add modulo from previous move
  deltaD1 = deltaD1 + d1Modulo;
  deltaD2 = deltaD2 + d2Modulo;
  
  float deltaDRatio = (float)deltaD1 / (float)deltaD2;

  // perform the move
  boolean d1Done = false;
  boolean d2Done = false;
  
  while(d1Done == false || d2Done == false){
    if(fabs(deltaD1) < currentD1StepLength() && d1Done == false){
      d1Modulo = deltaD1;
      d1Done = true;
      Serial.println("d1 modulo:");
      Serial.println(d1Modulo);

    }
    
    if(d1Done == false){
      if(deltaD1 > 0){
        deltaD1 = deltaD1 - currentD1StepLength();
        leftStepRight();
      } else {
        deltaD1 = deltaD1 + currentD1StepLength();
        leftStepLeft();
      }
    }
    
    if(fabs(deltaD2) < currentD2StepLength() && d2Done == false){
      d2Modulo = deltaD2;
      d2Done = true;
      Serial.println("d2 modulo:");
      Serial.println(d2Modulo);
    }

    if(d2Done == false){
      if(deltaD2 > 0){
        deltaD2 = deltaD2 - currentD2StepLength();
        rightStepRight();
      } else {
        deltaD2 = deltaD2 + currentD2StepLength();
        rightStepLeft();
      }
    }
  }
  // set current position after successfull move
  currentX = x;
  currentY = y;
  
  Serial.println("Wire length d1 after:");
  Serial.println(currentD1);
  Serial.println("Wire lenght d2 after:");
  Serial.println(currentD2);  
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

  // Argument 1
  int arg1End = commandString.indexOf(delimiter);
  String arg1 = commandString.substring(0, arg1End);
  if(arg1End != -1){
    commandString = commandString.substring(arg1End + 1);
  } else { 
    commandString = "";
  }
  //Serial.println(commandString);

  // Argument 2
  int arg2End = commandString.indexOf(delimiter);
  String arg2 = commandString.substring(0, arg2End);
  if(arg2End != -1){
    commandString = commandString.substring(arg2End + 1);
  } else { 
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
  
  if(command == "setZero"){
    setZero();
    printInfo();
  }
  
  if(command == "goToAbs"){
    //first pass
    goToAbs(arg1.toInt(), arg2.toInt());

    Serial.println("READY");
  }

  if(command == "goToAbsAndPaint"){
    //first pass
    goToAbs(arg1.toInt(), arg2.toInt());
    
    bluetooth.write('p');
    bluetooth.write('a');
    bluetooth.write('i');
    bluetooth.write('n');
    bluetooth.write('t');
    bluetooth.write(':');
    bluetooth.write('\n');
    delay(1500);
    Serial.println("READY");
  }


  if(command == "info"){
    printInfo();
  }

  if(command == "wait"){
    delay(arg1.toInt());
  }

  if(command == "setCanvas"){
    xMax = arg1.toInt();
    yMax = arg2.toInt();
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

    // Pass serial char to bluetooth module
    bluetooth.print(serialChar);

    // If newline incomes on serial, parse buffer
    if(serialChar == '\n'){      
      // Parse buffer to command
      parseCommand(serialBuffer);

      // Reset serial buffer
      serialBuffer = "";
    } 
    else {
      serialBuffer += serialChar;   
    }    
  }

  if(sdCardInitialized == false){
    //Read buttons
    if(analogRead(button1) > 1000) {
      leftStepLeft();
    }

    if(analogRead(button2) > 1000) {
      leftStepRight();
    }

    if(analogRead(button3) > 1000) {
      rightStepLeft();
    }

    if(analogRead(button4) > 1000) {
      rightStepRight();
    }
  } 
  else {  
    dataFile = SD.open(dataFileName);
    if(dataFile){
      dataFile.seek(dataFilePosition);

      char dataChar;

      //read line from file
      while(dataFile.available() ){
        dataChar = dataFile.read();
        Serial.print(dataChar);
        //do nat add newline to buffer
        if(dataChar == '\n'){
          break;
        }
        dataBuffer += dataChar;
      }
      dataFilePosition = dataFile.position();
      dataFile.close();

      // When card is uncleanly removed dataBuffer is empty, we are not performing any actions
      if(dataBuffer != ""){
        Serial.println("SD line caught:");
        Serial.println(dataBuffer);

        // parse and eval command
        parseCommand(dataBuffer);

        // Write performed line to the log
        Serial.println("Writing log");
        printedFile = SD.open(printedFileName, FILE_WRITE);
        printedFile.print(dataBuffer);
        printedFile.print('\n');
        printedFile.close();
      }
      //release data buffer
      dataBuffer = "";
    }
  }  
  // and loop forever and ever!
}


