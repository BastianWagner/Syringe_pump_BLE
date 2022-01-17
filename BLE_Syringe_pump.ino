#include <AccelStepper.h>
#include <ArduinoBLE.h>

/// Define stepper motor connections and motor interface type. Motor interface type must be set to 1 when using a driver:
// Stepper pump
#define enPin_pump_1 12
#define ms1Pin_pump_1 11 
#define ms2Pin_pump_1 10    
#define stepPin_pump_1 9   
#define dirPin_pump_1 8     
#define motorInterfaceType_pump_1 1

/// Define stepper motor connections and motor interface type. Motor interface type must be set to 1 when using a driver:
// Stepper pump
#define enPin_pump_2 7
#define ms1Pin_pump_2 6 
#define ms2Pin_pump_2 5    
#define stepPin_pump_2 19    //Analog 5   
#define dirPin_pump_2 20     //Analog 6     
#define motorInterfaceType_pump_2 1

//spreadPin controls modi between 0=stealthChop and 1=spreadCycle of all connected stepper drivers (TMC220x)
#define spreadPin 2  

/// Create instances of the AccelStepper class:
AccelStepper stepper_1 = AccelStepper(motorInterfaceType_pump_1, stepPin_pump_1, dirPin_pump_1);
AccelStepper stepper_2 = AccelStepper(motorInterfaceType_pump_2, stepPin_pump_2, dirPin_pump_2);

/// Define global variables
float steps_per_second_pump_1 = 0;
float steps_per_second_pump_2 = 0;
float total_steps_pump_1 = 0;
float total_steps_pump_2 = 0;


//BLEPeripheral blePeripheral;  // BLE Peripheral Device (the board you're programming)
BLEService pumpService("920a501d-8c39-45c4-bd0e-a1aa0d6cdf2b"); // BLE LED Service

// BLE Stepper Steps Characteristic - custom 128-bit UUID, read and writable by central
BLEUnsignedCharCharacteristic stepsCharacteristic_1("45845fe7-633a-463f-b4ab-5f0ed8403544", BLERead | BLEWrite);

// BLE Stepper Speed Characteristic - custom 128-bit UUID, read and writable by central
BLEUnsignedCharCharacteristic speedCharacteristic_1("039ee0a6-6db7-46de-aad3-0eaa483918a1", BLERead | BLEWrite);

// BLE Stepper Speed Characteristic - custom 128-bit UUID, read and writable by central
BLEUnsignedCharCharacteristic stepsCharacteristic_2("7853b997-bbda-4d6f-842d-d00f97643699", BLERead | BLEWrite);

// BLE Stepper Speed Characteristic - custom 128-bit UUID, read and writable by central
BLEUnsignedCharCharacteristic speedCharacteristic_2("bcae9061-246b-451f-88bb-68ac8af74317", BLERead | BLEWrite);


void setup() {
  //Setup Serial
  Serial.begin(230400);

  //Set LED pin to output mode
  pinMode(LED_BUILTIN, OUTPUT);
  
  //Start BLE 
  if (!BLE.begin()) 
  {
  Serial.println("starting BLE failed!");
  while (1);
  }
  
  //Set advertised local name
  BLE.setLocalName("BLE-SyringePump");

  //Add service and characteristic:
  BLE.setAdvertisedService(pumpService);
  pumpService.addCharacteristic(stepsCharacteristic_1);
  pumpService.addCharacteristic(speedCharacteristic_1);
  pumpService.addCharacteristic(stepsCharacteristic_2);
  pumpService.addCharacteristic(speedCharacteristic_2);
  BLE.addService(pumpService);

  //Begin advertising BLE service:
  BLE.advertise();
  Serial.println("Bluetooth device active, waiting for connections...");
  
  //Deactivate driver (HIGH deactive)
  pinMode(enPin_pump_1, OUTPUT);
  pinMode(enPin_pump_2, OUTPUT);
  digitalWrite(enPin_pump_1, HIGH); 
  digitalWrite(enPin_pump_2, HIGH);
  
  //Set dirPin to LOW
  pinMode(dirPin_pump_1, OUTPUT);
  pinMode(dirPin_pump_2, OUTPUT);
  digitalWrite(dirPin_pump_1, LOW); 
  digitalWrite(dirPin_pump_2, LOW); 
  
  // set stepPin to LOW
  pinMode(stepPin_pump_1, OUTPUT);
  pinMode(stepPin_pump_2, OUTPUT);
  digitalWrite(stepPin_pump_1, LOW);
  digitalWrite(stepPin_pump_2, LOW);
  
  //Set modi between 0=stealthChop and 1=spreadCycle of all connected stepper drivers (TMC220x)
  pinMode(spreadPin, OUTPUT);
  digitalWrite(spreadPin, LOW);
  
  //Set pump stepper to 1/16 microstepping
  pinMode(ms1Pin_pump_1, OUTPUT);
  digitalWrite(ms1Pin_pump_1, HIGH);
  pinMode(ms2Pin_pump_1, OUTPUT);
  digitalWrite(ms2Pin_pump_1, HIGH);
  
  //Set fraction stepper to 1/16 microstepping
  pinMode(ms1Pin_pump_2, OUTPUT);
  digitalWrite(ms1Pin_pump_2, HIGH);
  pinMode(ms2Pin_pump_2, OUTPUT);
  digitalWrite(ms2Pin_pump_2, HIGH);

  //Activate Driver
  digitalWrite(enPin_pump_1, LOW); 
  digitalWrite(enPin_pump_2, LOW); 

  //Set the maximum speed in steps per second:
  stepper_1.setMaxSpeed(4000);
  stepper_2.setMaxSpeed(4000);

  //Set Current Position
  stepper_1.setCurrentPosition(0);
  stepper_2.setCurrentPosition(0);
}
void loop() {

  //Listen for BLE peripherals to connect:
  BLEDevice central = BLE.central();

  //If a central is connected to peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());

    String data_TotalSteps_1 = "";
    int char_counter_TotalSteps_1 = 0;

    String data_Speed_1 = "";
    int char_counter_Speed_1 = 0;

    String data_TotalSteps_2 = "";
    int char_counter_TotalSteps_2 = 0;

    String data_Speed_2 = "";
    int char_counter_Speed_2 = 0;

    int direction_1 = 1;
    int direction_2 = 1;
    
    //While the central is still connected to peripheral, collcet new characteristic values
    digitalWrite(LED_BUILTIN, HIGH); 
    while (central.connected()) {
      //Collect 10 bytes in total, transfrom each byte to single number and concat each to from a long number
      if (stepsCharacteristic_1.written()) {
        char byte_char = (char) stepsCharacteristic_1.value();
        if (byte_char == '-') {
          direction_1 = -1;
          char_counter_TotalSteps_1++;
        }
        else {
          data_TotalSteps_1.concat(byte_char);
          char_counter_TotalSteps_1++;
        }
     
        if (char_counter_TotalSteps_1 == 10) {
          total_steps_pump_1 = data_TotalSteps_1.toFloat();
          total_steps_pump_1 = total_steps_pump_1*direction_1;
          Serial.println("Total steps pump_1: " + String(total_steps_pump_1));
          data_TotalSteps_1 = "";  
          char_counter_TotalSteps_1 = 0;     
        }
      }
      
      //Collect 10 bytes in total, transfrom each byte to single number and concat each to from a long number
      if (stepsCharacteristic_2.written()) {
        char byte_char = (char) stepsCharacteristic_2.value();
        if (byte_char == '-') {
          direction_2 = -1;
          char_counter_TotalSteps_2++;
        }
        else {
          data_TotalSteps_2.concat(byte_char);
          char_counter_TotalSteps_2++;
        }
     
        if (char_counter_TotalSteps_2 == 10) {
          total_steps_pump_2 = data_TotalSteps_2.toFloat();
          total_steps_pump_2 = total_steps_pump_2*direction_2;
          Serial.println("Total steps pump_2: " + String(total_steps_pump_2));
          data_TotalSteps_2 = "";  
          char_counter_TotalSteps_2 = 0;     
        }
      }
      
      //Collect 10 bytes in total, transfrom each byte to single number and concat each to from a long number
      if (speedCharacteristic_1.written()) {
        char byte_char = (char) speedCharacteristic_1.value();
        if (byte_char == '-') {
          direction_1 = -1;
          char_counter_Speed_1++;
        }
        else {
          data_Speed_1.concat(byte_char);
          char_counter_Speed_1++;
        }
     
        if (char_counter_Speed_1 == 10) {
          steps_per_second_pump_1 = data_Speed_1.toFloat();
          steps_per_second_pump_1 = steps_per_second_pump_1*direction_1;
          Serial.println("Speed pump_1: " + String(steps_per_second_pump_1));
          data_Speed_1 = "";  
          char_counter_Speed_1 = 0;     
        }
      }

      //Collect 10 bytes in total, transfrom each byte to single number and concat each to from a long number
      if (speedCharacteristic_2.written()) {
        char byte_char = (char) speedCharacteristic_2.value();
        if (byte_char == '-') {
          direction_2 = -1;
          char_counter_Speed_2++;
        }
        else {
          data_Speed_2.concat(byte_char);
          char_counter_Speed_2++;
        }
     
        if (char_counter_Speed_2 == 10) {
          steps_per_second_pump_2 = data_Speed_2.toFloat();
          steps_per_second_pump_2 = steps_per_second_pump_2*direction_2;
          Serial.println("Speed pump_2: " + String(steps_per_second_pump_2));
          data_Speed_2 = "";  
          char_counter_Speed_2 = 0;     
        }
      }
    }

    //Reset stepper position to zero
    stepper_1.setCurrentPosition(0);
    stepper_2.setCurrentPosition(0);
    //When the central disconnects, print it out:
    Serial.print(F("Disconnected from central: "));
    Serial.println(central.address());
    digitalWrite(LED_BUILTIN, LOW);     
  }

  //Move Stepper 1
  if (stepper_1.currentPosition() != total_steps_pump_1) {
     stepper_1.setSpeed(steps_per_second_pump_1);
     stepper_1.runSpeed(); 
  }
  //Move Stepper 2
  if (stepper_2.currentPosition() != total_steps_pump_2) {
     stepper_2.setSpeed(steps_per_second_pump_2);
     stepper_2.runSpeed();      
  }
}  
