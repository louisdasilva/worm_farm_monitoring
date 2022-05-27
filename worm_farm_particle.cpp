int boardLED = D7; // The on-board LED
int button = D2; // push button pin
int tempLED = D3; // red LED
int soilLED = D4; // blue LED
bool button_press = false; // whether the button is depressed
bool button_latched = false; // once pressed button latched until button released
bool user_alert_latched = false; // once alerted, latch until warning cancelled

/**
* user alert latching is used in the ifttt_trigger() otherwise the program will publish an alert on every time an mqtt alert is received which will be 
* on every sample cycle of the arduino at the monitoring end. Latching means an alert is issued  once and only once, when a new alert is received, the latch 
* is released when a message is received to cancel the alert.
*/
void ifttt_trigger(String message){
    if(!user_alert_latched) {
        Particle.publish("user_alert", message);
    }
}

void tempWarn(bool temp_warning) {
    if(temp_warning) {
        digitalWrite(tempLED, HIGH);
    } else digitalWrite(tempLED, LOW);
}

void soilWarn(bool soil_warning) {
    if(soil_warning) {
        digitalWrite(soilLED, HIGH);
    } else digitalWrite(soilLED, LOW);
}

// MQTT
#include <MQTT.h>
void callback(char* topic, byte* payload, unsigned int length);
void callback(char* topic, byte* payload, unsigned int length) {
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = NULL;
    String message(p);
    Serial.write(payload, length);
    Serial.println(topic);
    if(message == "temp high" or message == "temp low") {
        Serial.println("TEMP WARNING");
        tempWarn(true);
        ifttt_trigger(message);
        user_alert_latched = true;
    }
    if(message == "temp safe") {
        Serial.println("TEMP SAFE");
        tempWarn(false);
        user_alert_latched = false;
    }
    if(message == "soil low") {
        Serial.println("SOIL MOISTURE LOW");
        soilWarn(true);
        ifttt_trigger(message);
        user_alert_latched = true;
    }
    if(message == "soil good") {
        Serial.println("SOIL MOISTURE GOOD");
        soilWarn(false);
        user_alert_latched = false;
    }    
}
MQTT client("broker.mqttdashboard.com", 1883, callback);


// PROGRAM
void setup() {
  pinMode(boardLED, OUTPUT);
  pinMode(button, INPUT);
  pinMode(tempLED, OUTPUT);
  pinMode(soilLED, OUTPUT);
  
  // connect to the MQTT server
  client.connect("worm_client");
  // publish/subscribe
  if (client.isConnected()) {
    client.publish("/wormFarm218541748/userApproach","Hello Worms, this is Particle");
    client.subscribe("/wormFarm218541748/tempWarn");
    client.subscribe("/wormFarm218541748/soilWarn");
  }
}

void loop() {
    if (client.isConnected())
        client.loop();
    if(digitalRead(button) == LOW) {
        button_press = true;
    }
    else button_press = false;
    
    /**
     * button latching is used below otherwise the main program loop will cycle through the button press logic at high frequency
     * latching controls the press instructions only once, rather than to continue firing infinitely whilst the button is pressed
     */
    if(button_press and !button_latched) {
        digitalWrite(boardLED, HIGH);
        client.publish("/wormFarm218541748/userApproach", "user_approaching");
        button_latched = true;
    }
    if(!button_press and button_latched) {
        delay(2000); // the delay allows the logic inputs time to switch states correctly before being probed again, especially with a slow release having ambiguous inputs
        digitalWrite(boardLED, LOW);
        button_latched = false;
    }
}