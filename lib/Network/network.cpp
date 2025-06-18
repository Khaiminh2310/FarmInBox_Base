#include "network.h"
#include "common.h"
#include <WiFi.h>

const String ap_ssid = String(AP_NAME) + "-" + String(WIFI_getChipId(), HEX);

Network::Network() {};
Network::~Network() {
    delete custom_device_id;
    delete custom_token;
};

void Network::initWifi()
{
    // update ssid & password
    getWifiCredentials();

    if (ssid.isEmpty() || password.isEmpty())
    {
        DEBUG_SERIAL("No saved WiFi credentials!");
        if (colorMode != RED)
        {
            ledRed();
        }
        // Stop the program and wait until WiFi is configured and connected successfully, led off.
        xSemaphoreTake(xSemaphoreWiFi, portMAX_DELAY);
        vSemaphoreDelete(xSemaphoreWiFi);
        xSemaphoreWiFi = NULL;
        return;
    }

    DEBUG_SERIAL("Saved WiFi found, attempting to connect %s", ssid.c_str());
    WiFi.mode(WIFI_STA);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    WiFi.begin(ssid.c_str(), password.c_str());

    for (uint8_t i = 0; i < 2; i++)
    {
        if (i == 1)
        {
            WiFi.disconnect();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            WiFi.begin(ssid.c_str(), password.c_str());
        }

        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECT_TIMEOUT * 1000)
        {
            ledToggle();
            Serial.print(".");
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        DEBUG_SERIAL();

        if (WiFi.status() == WL_CONNECTED)
        {
            break;
        }
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        DEBUG_SERIAL("Unable to connect %s", ssid.c_str());
        // Notifies user of WiFi connection failure
        if (colorMode != RED)
        {
            ledRed();
        }
        // Stop the program and wait until WiFi is configured and connected successfully.
        xSemaphoreTake(xSemaphoreWiFi, portMAX_DELAY);
        vSemaphoreDelete(xSemaphoreWiFi);
        xSemaphoreWiFi = NULL;
    }
    else
    {
        DEBUG_SERIAL("Connected to WiFi: %s", ssid.c_str());
    }
}

bool Network::portalConfig()
{
    if (!portal_initialized)
    {
        if (custom_device_id == nullptr) custom_device_id = new WiFiManagerParameter("deviceId", "DEVICE ID", device_id, 50);
        if (custom_token == nullptr) custom_token = new WiFiManagerParameter("token", "TOKEN", token, 100);

        wm.addParameter(custom_device_id);
        wm.addParameter(custom_token);

        wm.setTitle(String("FarmInBox"));
        wm.setSaveParamsCallback(saveParamCallback);
        wm.setParamsPage(true);
        wm.setDebugOutput(false);
        wm.setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT);

        portal_initialized = true;
    }

    WiFi.mode(WIFI_AP_STA);
    DEBUG_SERIAL("Connect WiFi to accesspoint: %s and connect to IP: 192.168.4.1", ap_ssid.c_str());

    wm.startConfigPortal(ap_ssid.c_str(), AP_PW);
    ledRed();

    // Use in case of only configuring MQTT parameters and exiting the portal.
    // The device will connect to previously saved WiFi information if available.
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 5000)
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        DEBUG_SERIAL("Connected to WiFi: %s", WiFi.SSID().c_str());
        putWifiCredentials();
        getWifiCredentials();

        return true;
    }

    return false;
}

void Network::getWifiCredentials()
{
    DEBUG_SERIAL("Get WiFi credentials...");
    preferences.begin("storage", false);
    ssid = preferences.getString("ssid");
    password = preferences.getString("password");
    preferences.end();
}

void Network::putWifiCredentials()
{
    DEBUG_SERIAL("Put WiFi credentials...");
    preferences.begin("storage", false);
    preferences.putString("ssid", WiFi.SSID().c_str());
    preferences.putString("password", WiFi.psk().c_str());
    preferences.end();
}

bool Network::getParameters()
{
    DEBUG_SERIAL("Get parameters...");
    preferences.begin("storage", false);
    String _token   = preferences.getString("token");
    String _id      = preferences.getString("device_id");
    preferences.end();

    if (_id.isEmpty() || _token.isEmpty())
    {
        DEBUG_SERIAL("Missing device ID or token!");
        return false;
    }

    snprintf(device_id, sizeof(device_id), "%s", _id.c_str());
    snprintf(token, sizeof(token), "%s", _token.c_str());

    DEBUG_SERIAL("Token     : %s", token);
    DEBUG_SERIAL("Device_id : %s", device_id);

    return true;
}

void Network::putParamaters()
{
    DEBUG_SERIAL("Put parameters...");
    preferences.begin("storage", false);
    preferences.putString("token", token);
    preferences.putString("device_id", device_id);
    preferences.end();
}

// class obj
class Network network;

///////////////////////////////////////////////////////////////////////////////////////
//callback notification to us when save parameters
void saveParamCallback()
{
    DEBUG_SERIAL("Save param callback");

    const char* new_token = network.custom_token->getValue();
    if (strcmp(new_token, network.token) != 0)
    {
        snprintf(network.token, sizeof(network.token), "%s", new_token);
        network.new_config = true;
    }
    const char* new_device_id = network.custom_device_id->getValue();
    if (strcmp(new_device_id, network.device_id) != 0)
    {
        snprintf(network.device_id, sizeof(network.device_id), "%s", network.custom_device_id->getValue());
        network.new_config = true;
    }

    network.putParamaters();
}
///////////////////////////////////////////////////////////////////////////////////////