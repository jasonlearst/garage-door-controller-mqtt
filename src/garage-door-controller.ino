/*
 * Project garage-door-controller
 * Description: MQTT client for my garage door.  Implementation based on 
 * https://www.instructables.com/DIY-Smart-Garage-Door-Opener-Using-ESP8266-Wemos-D/
 * Author: Jason Learst <jason.learst@gmail.com>
 * Date: 2022-01-22
 */

#include "MQTT.h"
#include "secrets.h"

// MQTT Parameter
#define MQTT_BROKER SECRET_MQTT_BROKER
#define MQTT_CLIENTID "garage-cover"
#define MQTT_USERNAME SECRET_MQTT_USERNAME
#define MQTT_PASSWORD SECRET_MQTT_PASSWORD

// Control PINs
#define DOOR_OPEN_PIN D1
#define DOOR_CLOSE_PIN D1
#define DOOR_STATUS_PIN D6
#define DOOR_STATUS_LED D7

// Relay Parameters
#define ACTIVE_HIGH_RELAY true

// Door  Parameters
#define DOOR_ALIAS "Door"
#define MQTT_DOOR_ACTION_TOPIC "garage-cover/door/action"
#define MQTT_DOOR_STATUS_TOPIC "garage-cover/door/status"

const char* mqtt_broker = MQTT_BROKER;
const char* mqtt_clientId = MQTT_CLIENTID;
const char* mqtt_username = MQTT_USERNAME;
const char* mqtt_password = MQTT_PASSWORD;

const boolean activeHighRelay = ACTIVE_HIGH_RELAY;

const char* door_alias = DOOR_ALIAS;
const char* mqtt_door_action_topic = MQTT_DOOR_ACTION_TOPIC;
const char* mqtt_door_status_topic = MQTT_DOOR_STATUS_TOPIC;
const int door_openPin = DOOR_OPEN_PIN;
const int door_closePin = DOOR_CLOSE_PIN;
const int door_statusPin = DOOR_STATUS_PIN;
const int door_statusLed = DOOR_STATUS_LED;

const int relayActiveTime = 500;
int door_lastStatusValue = 2;
unsigned long door_lastSwitchTime = 0;
unsigned int debounceTime = 2000;

String availabilityBase = MQTT_CLIENTID;
String availabilitySuffix = "/availability";
String availabilityTopicStr = availabilityBase + availabilitySuffix;
const char* availabilityTopic = availabilityTopicStr.c_str();
const char* birthMessage = "online";
const char* lwtMessage = "offline";

MQTT client(mqtt_broker, 1883, callback);

// Callback when MQTT message is received; calls triggerDoorAction(), passing topic and payload as parameters
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  String topicToProcess = topic;
  payload[length] = '\0';
  String payloadToProcess = (char*)payload;
  triggerDoorAction(topicToProcess, payloadToProcess);
}

// Functions that check door status and publish an update when called
void publish_door_status() {
  if (digitalRead(door_statusPin) == LOW) {
      Serial.print(door_alias);
      Serial.print(" closed! Publishing to ");
      Serial.print(mqtt_door_status_topic);
      Serial.println("...");
      client.publish(mqtt_door_status_topic, "closed", true);
  }
  else {
      Serial.print(door_alias);
      Serial.print(" open! Publishing to ");
      Serial.print(mqtt_door_status_topic);
      Serial.println("...");
      client.publish(mqtt_door_status_topic, "open", true);
  }
}

// Functions that run in loop() to check each loop if door status (open/closed) has changed and call publish_doorX_status() to publish any change if so
void check_door_status() {
  int currentStatusValue = digitalRead(door_statusPin);
  if (currentStatusValue != door_lastStatusValue) {
    unsigned long currentTime = millis();
    if (currentTime - door_lastSwitchTime >= debounceTime) {
      publish_door_status();
      door_lastStatusValue = currentStatusValue;
      door_lastSwitchTime = currentTime;
    }
  }
}

// Function that publishes birthMessage
void publish_birth_message() {
  Serial.print("Publishing birth message \"");
  Serial.print(birthMessage);
  Serial.print("\" to ");
  Serial.print(availabilityTopic);
  Serial.println("...");
  client.publish(availabilityTopic, birthMessage, true);
}

// Function that toggles the relevant relay-connected output pin
void toggleRelay(int pin) {
  if (activeHighRelay) {
    digitalWrite(pin, HIGH);
    delay(relayActiveTime);
    digitalWrite(pin, LOW);
  }
  else {
    digitalWrite(pin, LOW);
    delay(relayActiveTime);
    digitalWrite(pin, HIGH);
  }
}

void triggerDoorAction(String requestedDoor, String requestedAction) {
  if (requestedDoor == mqtt_door_action_topic && requestedAction == "OPEN") {
    Serial.print("Triggering ");
    Serial.print(door_alias);
    Serial.println(" OPEN relay!");
    toggleRelay(door_openPin);
  }
  else if (requestedDoor == mqtt_door_action_topic && requestedAction == "CLOSE" ) {
    Serial.print("Triggering ");
    Serial.print(door_alias);
    Serial.println(" CLOSE relay!");
    toggleRelay(door_closePin);
  }
  else if (requestedDoor == mqtt_door_action_topic && requestedAction == "STATE") {
    Serial.print("Publishing on-demand status update for ");
    Serial.print(door_alias);
    Serial.println("!");
    publish_birth_message();
    publish_door_status();
  }
  else { Serial.println("taking no action!");
  }
}

// Function that runs in loop() to connect/reconnect to the MQTT broker, and publish the current door statuses on connect
void reconnect() {
  // Loop until we're reconnected
  while (!client.isConnected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_clientId, mqtt_username, mqtt_password, availabilityTopic, MQTT::EMQTT_QOS::QOS0, 1, lwtMessage, true)) {
      Serial.println("Connected!");
      publish_birth_message();
      // Subscribe to the action topics to listen for action messages
      Serial.print("Subscribing to ");
      Serial.print(mqtt_door_action_topic);
      Serial.println("...");
      client.subscribe(mqtt_door_action_topic);

      // Publish the current door status on connect/reconnect to ensure status is synced with whatever happened while disconnected
      publish_door_status();
    } 
    else {
      Serial.print("failed, rc=");
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// setup() runs once, when the device is first turned on.
void setup() {
  // Put initialization like pinMode and begin functions here.
  pinMode(door_openPin, OUTPUT);
  pinMode(door_closePin, OUTPUT);
  // Set output pins LOW with an active-high relay
  if (activeHighRelay) {
    digitalWrite(door_openPin, LOW);
    digitalWrite(door_closePin, LOW);
  }
  // Set output pins HIGH with an active-low relay
  else {
    digitalWrite(door_openPin, HIGH);
    digitalWrite(door_closePin, HIGH);
  }
  // Set input pin to use internal pullup resistor
  pinMode(door_statusPin, INPUT_PULLUP);
  door_lastStatusValue = digitalRead(door_statusPin);

  Serial.begin(115200);
  Serial.println("Starting GarHAge...");

  if (activeHighRelay) {
    Serial.println("Relay mode: Active-High");
  }
  else {
    Serial.println("Relay mode: Active-Low");
  }
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  // The core of your code will likely live here.
  // Connect/reconnect to the MQTT broker and listen for messages
  if (!client.isConnected()) {
    reconnect();
  }
  client.loop();
  // Check door open/closed status each loop and publish changes
  check_door_status();

}