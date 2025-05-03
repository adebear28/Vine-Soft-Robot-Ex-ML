#include <Wire.h>
#define DAC_1 0x61
#define DAC_2 0x60

// Define pins for solenoids and buttons
const int switchSideOne = 48;
const int switchSideTwo = 49;
const int solenoidPinOne = 44;
const int solenoidPinTwo = 45;

const int BlueCCWPin = 24;
const int YellowCWPin = 25;
const int WhitePin = 50;

const int motorPin1 = 33;     // Motor driver input 1
const int motorPin2 = 31;     // Motor driver input 2
const int pwmPin =  4;        // PWM pin to control motor speed

//pressure regulator for regulator
float qimax = 15.0;
float qi = 0.0;     // Initial pressure
float step = 0.005; 


// Define pins for rotary encoder
#define inputCLK 8
#define inputDT 9

// Variables for rotary encoder
int counter = 0;
int currentStateCLK;
int previousStateCLK;
String encdir = "";

uint16_t b;
uint8_t b1;
uint8_t b2;

uint16_t bv;
uint8_t bv1;
uint8_t bv2;

float qiv = 0.0;  
float qivSet = 1.125;

int onOffCounter = 0;

#define bounceDelay 20    //Minimum delay before regarding a button as being pressed and debounced
#define minButtonPress 3  //Number of times the button has to be detected as pressed before the press is considered to be valid

uint32_t previousMillis;       // Timers to time out bounce duration for each button
uint8_t pressCount;


void setup() {
  // Set up solenoid and button pins
  pinMode(switchSideOne, INPUT_PULLUP);
  pinMode(switchSideTwo, INPUT_PULLUP);
  
  pinMode(solenoidPinOne, OUTPUT);
  pinMode(solenoidPinTwo, OUTPUT);

  digitalWrite(solenoidPinOne, LOW);
  digitalWrite(solenoidPinTwo, LOW);

  // Set encoder pins as inputs
  pinMode(inputCLK, INPUT);
  pinMode(inputDT, INPUT);

    // Set up the button pins as inputs with pull-up resistors
  pinMode(BlueCCWPin, INPUT);
  pinMode(YellowCWPin, INPUT);
  pinMode(WhitePin, INPUT);

  // Set up the motor control pins as outputs
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(pwmPin, OUTPUT);

  // Set up Serial Monitor
  Serial.begin(9600);
  Serial.println("<Arduino is ready>");

  Wire.begin();

  // Initialize previous state for rotary encoder
  previousStateCLK = digitalRead(inputCLK);

  // Initialize motor to stop
  stopMotor();

}

void loop() {
  // Toggle and solenoid logic
  int buttonState = digitalRead(switchSideOne);
  int buttonState2 = digitalRead(switchSideTwo);

  // Motor button logic
  int CCWButtonState = digitalRead(BlueCCWPin);
  int CWButtonState = digitalRead(YellowCWPin);

  // Rotary encoder logic
  currentStateCLK = digitalRead(inputCLK);


// //Solenoid + toggle switch code

  if (buttonState == HIGH && buttonState2 == LOW){
    digitalWrite(solenoidPinOne, HIGH);
    digitalWrite(solenoidPinTwo, LOW);
    qiv = 0;
  }
  else if (buttonState == LOW && buttonState2 == HIGH){
    digitalWrite(solenoidPinOne, LOW);
    digitalWrite(solenoidPinTwo, HIGH);
    qiv = 0;
  }
  else{
    digitalWrite(solenoidPinOne, LOW);
    digitalWrite(solenoidPinTwo, LOW);
    if (onOffCounter % 2 == 1){
    qiv = qivSet;
    }
    else if (onOffCounter % 2 == 0){
    qiv = 0;
   }
  }
//Motor control
   // If left button is pressed, decrease the target position
  if (CCWButtonState == HIGH) {
    moveMotorClockwise();
    delay(200);  // Debounce delay
  }
  else {
    stopMotor();
  }

  // If right button is pressed, increase the target position
  if (CWButtonState == HIGH) {
    moveMotorCounterClockwise();
    delay(200);  // Debounce delay
  }
  else {
    stopMotor();
  }

  rotaryEncoder();
  if (qi > 0 && qi < qimax){
//regulator pressure regulator DAC
  b = qi / 72.5 * 5 * (pow(2.0, 12) - 1);
  b1 = b >> 4;
  b2 = (b & 15) << 4;
  // Send the DAC value to control the pressure regulator
  Wire.beginTransmission(DAC_1);
  Wire.write(64);  // Command to set DAC
  Wire.write(b1);  // Higher byte of DAC value
  Wire.write(b2);  // Lower byte of DAC value
  Wire.endTransmission();
 }
  previousStateCLK = currentStateCLK;

  debounce();
//onOff button pressure regulator DAC
  bv = qiv / 50 * (pow(2.0, 12) - 1); 
  bv1 = bv >> 4;
  bv2 = (bv & 15) << 4;
  // Send the DAC value to control the pressure regulator
  Wire.beginTransmission(DAC_2);
  Wire.write(64);  // Command to set DAC
  Wire.write(bv1);  // Higher byte of DAC value
  Wire.write(bv2);  // Lower byte of DAC value
  Wire.endTransmission();
 
}

// Function to move the motor clockwise
void moveMotorClockwise() {
  digitalWrite(motorPin1, HIGH);
  digitalWrite(motorPin2, LOW);
  analogWrite(pwmPin, 55);  // Adjust speed as needed
}

// Function to move the motor counterclockwise
void moveMotorCounterClockwise() {
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, HIGH);
  analogWrite(pwmPin, 55);  // Adjust speed as needed
}

// Function to stop the motor
void stopMotor() {
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  analogWrite(pwmPin, 0);
}

 void rotaryEncoder(){
       if (currentStateCLK != previousStateCLK) {
    // If DT state differs from CLK, the encoder is rotating counterclockwise
    if (digitalRead(inputDT) != currentStateCLK) {
      counter --;    // Increase the counter (rotate clockwise)
      encdir = "CCW";
      qi -= step;
   } else {
      counter ++;    // Decrease the counter (rotate counterclockwise)
      encdir = "CW";
      qi += step;    
    }

    // //Print the direction and current counter (optional)
    // Serial.print("Direction: ");
    // Serial.print(encdir);
    // Serial.print(" -- Counter: ");
    // Serial.println(counter);
    // // Print the current pressure
    // Serial.print("Current Pressure (psi): ");
    // Serial.println(qi);
  }
 }

void onOffButton(){
  // // OnOff button logic
  int onOffButtonState = digitalRead(WhitePin);

  if (onOffButtonState == HIGH){
    onOffCounter++;
  }

  if (onOffCounter % 2 == 1){
    qiv = qivSet;
  }
  else if (onOffCounter % 2 == 0){
    qiv = 0;
  }
  Serial.print("Counter: ");
  Serial.println(onOffCounter);
}

void debounce() {
  uint32_t currentMillis = millis();
    if (digitalRead(WhitePin) == LOW) {             //Input is high, button not pressed or in the middle of bouncing and happens to be high
        previousMillis = currentMillis;        //Set previousMillis to millis to reset timeout
        pressCount = 0;                        //Set the number of times the button has been detected as pressed to 0
      } else {
      if (currentMillis - previousMillis > bounceDelay) {
        previousMillis = currentMillis;        //Set previousMillis to millis to reset timeout
        ++pressCount;
        if (pressCount == minButtonPress) {
          onOffButton();                             //Button has been debounced. Call function to do whatever you want done.
        }
      }
    }
}
