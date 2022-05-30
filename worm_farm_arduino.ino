/** Code informed by or modified and adapted from
 *  DHT22 sensor:     https://create.arduino.cc/projecthub/mafzal/temperature-monitoring-with-dht22-arduino-15b013
 *  Light Sensing:    https://www.electronicshub.org/photodiode-working-characteristics-applications/
 *  Soil Moisture:    https://wiki.dfrobot.com/Moisture_Sensor__SKU_SEN0114_
 *  Distance Sensing: https://randomnerdtutorials.com/complete-guide-for-ultrasonic-sensor-hc-sr04/
 *  Serial Comms:     https://roboticsbackend.com/raspberry-pi-arduino-serial-communication/
 *                    https://www.arduino.cc/reference/en/language/functions/communication/serial/
 *  Async Timing:     https://www.arduino.cc/reference/en/libraries/asynctimer/
 *  
 *  by L.da Silva 24.05.2022
 */

// *** libraries ***

#include <DHT.h> // Adafruit Temperature Sensor Library
#include <AsyncTimer.h> // Async Timer library for non-blocking timer

// *** constants ***

const int tempPin = 2; // temperature sensor pin
const int lightPin = A0; // photo_diode pin
const int soilPin = A1; // soil_moisture pin
const int trigPin = 3; // proximity sensor trigger pin
const int echoPin = 4; // proximity sensor echo pin
const int buzzPin = 5; // buzzer pin

// *** global variables ***

float hum, temp;  // Stores humidity and temperature
int lightLevel; // Stores light level value
int soilMoisture; // Stores soil moisture level
long duration, cm; // Stores timing and distance for proximity sensor
bool user_approach = false; // Know whether the user is coming
float update_frequency = 300000; // 5 minute sampling frequency

// *** instances ***

DHT dht(tempPin, DHT22); // dht(inputPin, sensorType) Initialize DHT sensor for normal 16mhz Arduino
AsyncTimer t; // instance for asynchronous timing

// *** functions ***

// gets distance to nearest object in front of proximity sensor and stores it in the global cm variable
void get_distance(){
  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
 
  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  duration = pulseIn(echoPin, HIGH); // returned value is in microseconds (10^-6 seconds)
 
  // Convert the time into a distance
  cm = duration * 0.0343 / 2;     // speed of sound 343m/s = 34300cm/s = 0.0343cm/microsecond
}

/** buzzComms used for testing purposes as well as pest scare off 
 *  count = number of beeps
 *  timing = length of beep in milliseconds
 */
void buzzComms(int count, int timing) {
  for(int i = 0; i < count; i++) {
    digitalWrite(buzzPin, HIGH);
    delay(timing);
    digitalWrite(buzzPin, LOW);    
  }
  delay(timing);
}

void pest_check() {
  if(cm < 10){
    buzzComms(1, 3000); // buzz to scare off pests
  }
}

void check_user_approach() {
  if (Serial.available() > 0) { 
    long data_in = Serial.parseInt();
    if (data_in == 1) {
      user_approach = true;
    } else user_approach = false;
  } 
}

void reset_user_approach(){
  buzzComms(2, 500);
  user_approach = false;
}

void setup()
{
  Serial.begin(9600);
  dht.begin(); // temp sensor
  pinMode(tempPin, INPUT);
  pinMode(lightPin, INPUT);
  pinMode(soilPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzPin, OUTPUT);
}

void loop()
{   
    t.handle(); // handler for asynchronous timing
    
    // Read Sensors
    hum = dht.readHumidity();
    temp= dht.readTemperature();
    lightLevel = analogRead(lightPin);
    soilMoisture = analogRead(soilPin);
    get_distance();

    // pest control if no user approaching
    check_user_approach();
    if(user_approach) {
      t.setTimeout(reset_user_approach, 30000);
    } else pest_check();

    // Communicate Sensor Data as a JSON string
    Serial.print("{\"temperature\":");
    Serial.print(temp);
    Serial.print(",");

    Serial.print("\"humidity\":");
    Serial.print(hum);
    Serial.print(",");
    
    Serial.print("\"light\":");
    Serial.print(lightLevel);
    Serial.print(",");

    Serial.print("\"soilMoisture\":");
    Serial.print(soilMoisture);
    Serial.print(",");

    Serial.print("\"distance\":");
    Serial.print(cm);
    Serial.println("}");

    delay(update_frequency);
}