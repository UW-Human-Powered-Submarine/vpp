 /**
 * VPP Code
 */
#include "Arduino.h"
#include <SparkFun_TB6612.h> // https://github.com/sparkfun/SparkFun_TB6612FNG_Arduino_Library
// #define ENCODER_OPTIMIZE_INTERRUPTS // Optimizes Interrupts 
#include <Encoder.h> // https://github.com/PaulStoffregen/Encoder and https://www.pjrc.com/teensy/td_libs_Encoder.html#optimize

// Pins based off of https://easyeda.com/editor#id=815d6710d64740d7be23ae165a4ac48c|9e069235e92947119db9ace52b95412e
// Motor Pins
#define STBY 8
#define AIN1 9
#define AIN2 10
#define PWMA 11
const int offsetA = -1; // Change to 1 or -1, movement should be anti-clockwise
Motor motor1 = Motor(AIN1, AIN2, PWMA, offsetA, STBY); // Initialize motor
// Encoder Pins
// Change these two numbers to the pins connected to your encoder.
//   Best Performance: both pins have interrupt capability
//   Good Performance: only the first pin has interrupt capability
//   Low Performance:  neither pin has interrupt capability
#define CA 3
#define CB 4
#define INDEX 5
// Encoder Initialization
Encoder enc(CA, CB);
#define COUNTS 3244.188 // Countable Events Per Revolution (Output Shaft)
#define COUNTSPERANGLE COUNTS/360 // Counts per Angle
#define MOTORSPEED 81 // speed of the motor controlling the vpp
// Hall Effect Sensor Pin
#define HALL 2
#define HALLINTERRUPT HALL-2 // Pin 2 but interrupt Pin 0
// Variables to calculate RPM from HES data
#define SIZE 50 // define size of array
unsigned long rpm_array[SIZE]; // stores an array of microseconds (each element is 1 rotation’s time)
volatile unsigned int i;
unsigned long mean = 0;
unsigned long ms_start;

// States
double RPM;
// We need a map of RPM to angle (or use a function to model it)

// Takes time of each revolution in ms converts to rpm and stores into an array
void interrupt() { // one rotation has happened
    rpm_array[i] = 1.0 / (((millis() - ms_start) / (60.0 * 1000))); 		//initially has ms_start set to 0
    ms_start = millis();
    if(i == SIZE - 1) {		//sets i = 0 when it reaches the end of the array (replaces old value of array)
        i = 0;
    } else {			//increases i by 1 to parse through the array
        i++;
    }
}

// zeroes motor and returns the zeroed angle position
long zeroMotor() {
    motor1.drive(MOTORSPEED);
    while (digitalRead(INDEX)); // while index pin has not been found
    motor1.brake();
    return enc.read();
}

void setup() {
    Serial.begin(115200); // make sure this is running at high baud's to ensure fast transfers
    Serial.setTimeout(100000);
    delay(2000);
    Serial.println("VPP Starting...");

    // Hall Effect Initialization
    pinMode(HALL, INPUT_PULLUP);
    attachInterrupt(HALLINTERRUPT, interrupt, RISING);
    
    // Zero the motor to "home" (the angle that the motor SHOULD be at such that the propellor angle is 0 degrees)
    // Use index pin for this
    pinMode(INDEX, INPUT_PULLUP);
    zeroMotor();
}

// Function to check RPM
double checkRPM() 
{
//everytime i increases, the mean is updated by getting rid of the old value and replacing it with a new value
//it is not extremely intuitive but seems to work
	mean = ((mean * (double) SIZE) - rpm_array[i]) / SIZE; // prepare average calculation for later 		//subtract the current value in rpm[i]
	rpm_array[i] = 1.0 / (((millis() - ms_start) / (60.0 * 1000)));  // record ms passed to array			//caluculate the new value
	mean = ((mean * (double) SIZE) + rpm_array[i]) / SIZE; // get new average					//add the new value in rpm[i]
	
    double newRPM = mean;
    Serial.print("Current RPM: ");
    Serial.println(newRPM);
    Serial.println();
	return newRPM; // revolutions per minute, "fairly" accurate
}

// Function to shift the angle by a desired amount
// long angle - desired angle to shift the motor by
void changeAngle(long angle) {
    if (angle >= 0) { // if positive angle,
        motor1.drive(MOTORSPEED);
        long currPos = enc.read();
        long counts = COUNTSPERANGLE * angle;
        while(counts >= currPos) {
            long newPos = enc.read();
            if (newPos > currPos) {
                currPos = newPos;
                Serial.println(currPos);
            }
        }
        enc.readAndReset();
        motor1.brake();
    } else { // else if negative angle,
        motor1.drive(-MOTORSPEED);
        long currPos = enc.read();
        long counts = COUNTSPERANGLE * angle;
        while(counts <= currPos) {
            long newPos = enc.read();
            if (newPos < currPos) {
                currPos = newPos;
                Serial.println(currPos);
            }
        }
        enc.readAndReset();
        motor1.brake();
    }
}

void loop() {
    RPM = checkRPM();
    // Everytime we interrupt (or every few seconds), we want to check the RPM to see if it has changed significantly
    // TODO: Have some code to deal with motor jamming (some time limiter)

    // if(/* NEEDS ANGLE CHANGE, */) {
    //     changeAngle(a);
    // }

    // Temp stuff to test
    while(!Serial.available());
    long a = Serial.parseFloat();
    changeAngle(a);
}
