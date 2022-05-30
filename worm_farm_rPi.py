# Code informed by or modified and adapted from:
# json in python: https://www.w3schools.com/python/python_json.asp
# paho and mqtt:  https://www.hivemq.com/blog/mqtt-client-library-paho-python/
#                 https://au.mathworks.com/help/thingspeak/mqtt-api.html?s_tid=CRUX_lftnav
# serial comms:   https://pyserial.readthedocs.io/en/latest/pyserial_api.html
#                
# by L.da Silva 24.05.2022

# *** libraries ***
import json # json to python -> json.loads() - python to json -> json.dumps()
import paho.mqtt.publish as publish
import paho.mqtt.client as paho
import serial
import time

# serial communication initialisation
# ser = serial.Serial('COM6', 9600, timeout = 1) # uncomment on windows
ser = serial.Serial('/dev/ttyACM0', 9600, timeout = 1) # uncomment on raspberry pi
ser.reset_input_buffer()

def get_sensor_data():
    try:
        line = ser.readline().decode('utf-8').rstrip()
        sensor_data = json.loads(line)
        return sensor_data
    except Exception as e:
        print(e)
        return "error"

# mqtt for thingspeak
channel_ID_thingSpeak = "1686039"
host_thingSpeak = "mqtt3.thingspeak.com"
client_ID_thingSpeak = "ESE6HzwsDD0XNxwSBxw8Kgs"
username_thingSpeak = "ESE6HzwsDD0XNxwSBxw8Kgs"
password_thingSpeak = "QpQ7A1tiBWMevrdSAdgQUxEw"
t_transport_thingSpeak = "websockets" # mqtt transport type
t_port_thingSpeak = 80 # mqtt port
topic_thingSpeak = "channels/" + channel_ID_thingSpeak + "/publish"

def updateThingSpeak(payload_thingSpeak):
    print("Update to ThingSpeak: ", payload_thingSpeak)
    try:
        publish.single(topic_thingSpeak, 
                       payload_thingSpeak, 
                       hostname = host_thingSpeak, 
                       transport = t_transport_thingSpeak, 
                       port = t_port_thingSpeak, 
                       client_id = client_ID_thingSpeak, 
                       auth={"username":username_thingSpeak, "password":password_thingSpeak})
    except Exception as e:
        print(e)
    
#mqtt for Particle Argon
channel_Argon = "/wormFarm218541748/"
host_Argon = "broker.mqttdashboard.com"
t_transport_Argon = "websockets" # mqtt transport type
t_port_Argon = 8000 # mqtt port
topic_Argon_tempWarning = channel_Argon + "tempWarn"
topic_Argon_soilWarning = channel_Argon + "soilWarn"
topic_Argon_userApproach = channel_Argon + "userApproach"
payload_Argon_tempWarningHigh = "temp high"
payload_Argon_tempWarningLow = "temp low"
payload_Argon_tempWarningSafe = "temp safe"
payload_Argon_soilWarningLow = "soil low"
payload_Argon_soilWarningGood = "soil good"

def on_subscribe(client, userdata, mid, granted_qos):
    print("Subscribed: ", topic_Argon_userApproach)
def on_message(client, userdata, msg):
    message = str(msg.payload.decode('utf-8').rstrip())
    print(msg.topic + ": " + message)
    if(message == "user_approaching"):
        print("Pi: have recieved user approach message from Particle")
        ser.write(b'1') ###################################################################################################
client = paho.Client()
client.on_subscribe = on_subscribe
client.on_message = on_message
client.connect(host_Argon, 1883)
client.subscribe(topic_Argon_userApproach, qos=1)
client.loop_start()

def messageArgon(topic, payload):
    print("Message to Particle: ", payload)
    try:
        publish.single(topic,
                       payload,
                       hostname = host_Argon,
                       transport = t_transport_Argon,
                       port = t_port_Argon)
    except Exception as e:
        print(e)

def processWarnings(temp, soilM):
    ### WARNINGS TO PARTICLE ###
    # Temperature Monitoring
    if(temp > 28):
        messageArgon(topic_Argon_tempWarning, payload_Argon_tempWarningHigh)
    if(temp < 12):
        messageArgon(topic_Argon_tempWarning, payload_Argon_tempWarningLow)
    if(temp >= 12 and temp <= 28):
        messageArgon(topic_Argon_tempWarning, payload_Argon_tempWarningSafe)
    # Soil Monitoring
    if(soilM < 500):
        messageArgon(topic_Argon_soilWarning, payload_Argon_soilWarningLow)
    if(soilM >= 500):
        messageArgon(topic_Argon_soilWarning, payload_Argon_soilWarningGood)    

def main():
    message_id = 0
    while True:

        time.sleep(300) # data is sampled every 5 minutes on the Arduino
        print()
        message_id += 1
        print("Message ID: ", message_id)

        # if data incoming from arduino
        if ser.in_waiting >= 83: # expected arduino string 83 characters, anything else will produce an error
            sensor_data = get_sensor_data()
            if(type(sensor_data) == dict):
                temp = sensor_data["temperature"]
                hum = sensor_data["humidity"]
                light = sensor_data["light"]
                soilM = sensor_data["soilMoisture"]
                distance = sensor_data["distance"]
        
                print("temperature: ", temp,
                      "humidity: ", hum,
                      "light_level: ", light,
                      "soil moisture: ", soilM,
                      "distance: ", distance)
            
                ### WARNINGS TO ARGON ###
                processWarnings(temp, soilM)

                ### UPDATES TO THINGSPEAK ###
                payload_thingSpeak = "field1=" + str(temp) + "&field2=" + str(hum) + "&field3=" + str(soilM) + "&field4=" + str(light)
                updateThingSpeak(payload_thingSpeak)

if __name__ == "__main__":
    main()