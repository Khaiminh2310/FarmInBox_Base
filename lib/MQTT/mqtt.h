#ifndef _MQTT_H_
#define _MQTT_H_

#include "common.h"
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

class MQTTClient
{
public:
    MQTTClient();
    ~MQTTClient();

    WiFiClient espClient;
    PubSubClient client;

    void setup();
    void publish(const char* msg);
    void connect();
    void disconnect();
    bool loop();
    void reconnect();
    bool isConnected();
    static void callback(char* topic, uint8_t* payload, unsigned int length);

private:
    static void handleCommand(MQTTClient *subclient, const JsonDocument &doc);
};

extern class MQTTClient mqtt;

#endif