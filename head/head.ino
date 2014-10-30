// leftMotor:left:400
// rightMotor:left:400
// paint:
#include <Servo.h> 
#include <SoftwareSerial.h>  

//Config bluetooth module
int bluetoothTx = 2;  // TX-O pin of bluetooth mate, Arduino D2
int bluetoothRx = 3;  // RX-I pin of bluetooth mate, Arduino D3
SoftwareSerial bluetooth(bluetoothTx, bluetoothRx); 

//Config servo
Servo myservo; 

// Config serial buffer global
String bluetoothBuffer = "";
String serialBuffer = "";

//Config servo
int servoDefault = 45;
int servoTarget = 1;
int servoTargetDelay = 500;

void setup() { 
  Serial.begin(9600);  // Begin the serial monitor at 9600bps

  myservo.attach(9);  // attaches the servo on pin 9 to the servo object
  myservo.write(servoDefault);

  // Setup bluetooth
//  bluetooth.begin(115200);  // The Bluetooth Mate defaults to 115200bps
//  bluetooth.print("$");  // Print three times individually
//  bluetooth.print("$");
//  bluetooth.print("$");  // Enter command mode
//  delay(100);  // Short delay, wait for the Mate to send back CMD
//  bluetooth.println("U,9600,N");  // Temporarily Change the baudrate to 9600, no parity
//  // 115200 can be too fast at times for NewSoftSerial to relay the data reliably
  bluetooth.begin(19200);  // Start bluetooth serial at 9600

} 

void sweepServo(){
  myservo.write(servoTarget);
  delay(servoTargetDelay);
  myservo.write(servoDefault);
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
  } else{ 
    commandString = "";
  }
  
  // Argument 2
  int arg2End = commandString.indexOf(delimiter);
  String arg2 = commandString.substring(0, arg2End);
  if(arg2End != -1){
    commandString = commandString.substring(arg2End + 1);
  } else{ 
    commandString = "";
  }

  // Argument 3
  int arg3End = commandString.indexOf(delimiter);
  String arg3 = commandString.substring(0, arg3End);

  evalCommand(command, arg1, arg2, arg3);  
}

// Evaluating commands = mapping commands to functions
void evalCommand(String command, String arg1, String arg2, String arg3){
  if(command == "paint"){
    bluetooth.println("head: painting");
    sweepServo();
  }

  if(command == "setServo"){
    //setServo:35:1:300
    bluetooth.println("head: setting servo");
    servoDefault = arg1.toInt();
    servoTarget = arg2.toInt();
    servoTargetDelay = arg3.toInt();
    sweepServo();

  }
}

void loop() {
  // Bluetooth passthrought
  // Read bluetooh
  if(bluetooth.available()){
    // Send any characters the bluetooth prints to the serial monitor
    char bluetoothChar = bluetooth.read();
    if(bluetoothChar == '\n'){
      // Reset buffer
      parseCommand(bluetoothBuffer);
      bluetoothBuffer = "";
    } else {
      bluetoothBuffer += bluetoothChar;
    }
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
