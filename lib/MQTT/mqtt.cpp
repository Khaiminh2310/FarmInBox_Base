#include "mqtt.h"
#include "network.h"
#include "RS485.h"

MQTTClient::~MQTTClient() {}

MQTTClient::MQTTClient() : client(espClient) {}

void MQTTClient::setup()
{
    client.setServer(THINGSBOARD_SERVER, MQTT_PORT);
    client.setKeepAlive(60);
    client.setCallback(callback);
    connect();
    client.subscribe(MQTT_SUB_TOPIC);
}

void MQTTClient::publish(const char* msg)
{
    // Publish message
    DEBUG_SERIAL("Message: %s", msg);
    if (client.publish(MQTT_PUB_TELEMETRY, msg, true))
    {
        DEBUG_SERIAL("Message published successfully!");
    }
    else
    {
        DEBUG_SERIAL("Failed to publish message!");
    }
}

void MQTTClient::connect()
{
    if (!client.connected())
    {
        reconnect();
    }
}

void MQTTClient::disconnect()
{
    client.disconnect();
}

bool MQTTClient::loop()
{
    return client.loop();
}

void MQTTClient::reconnect()
{
    if (!client.connected())
    {
        DEBUG_SERIAL("Connecting to MQTT...");

        if (client.connect(network.device_id, network.token, nullptr))
        {
            DEBUG_SERIAL("===> connected to ThingsBoard");
            client.subscribe(MQTT_SUB_TOPIC);
        }
        else
        {
            DEBUG_SERIAL("===> failed, ret=%d try again in 5 seconds", client.state());
            vTaskDelay(4000 / portTICK_PERIOD_MS);
        }
    }
}

void MQTTClient::callback(char* topic, uint8_t* payload, unsigned int length)
{
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    DEBUG_SERIAL("Message arrived [%s]: %s", topic, message);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    if (error)
    {
        DEBUG_SERIAL("Invalid json format!");
    }
    else
    {
        handleCommand(&mqtt, doc);
    }
}

void MQTTClient::handleCommand(MQTTClient *subclient, const JsonDocument &doc)
{
    if (doc["ping"].is<String>())
    {
        String text = doc["ping"].as<String>();
        if (text == "mohub")
        {
            subclient->publish("{\"ping\": \"success\"}");
        }
    }
    else
    {
        DEBUG_SERIAL("Key not found!");
    }
}

class MQTTClient mqtt;