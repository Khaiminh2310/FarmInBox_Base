#ifndef _NETWORK_H
#define _NETWORK_H

#include <WiFiManager.h>
#include <Preferences.h>

void saveParamCallback();

class Network
{
public:
    Network();
    ~Network();

    void initWifi();
    bool portalConfig();

    bool getParameters();
    void putParamaters();

    WiFiManagerParameter *custom_device_id = nullptr;
    WiFiManagerParameter *custom_token = nullptr;

    char device_id[51] = {0};
    char token[101] = {0};

    bool new_config = false;
    bool portal_initialized = false;

    String ssid = String();
    String password = String();

private:
    void getWifiCredentials();
    void putWifiCredentials();

    Preferences preferences;
    WiFiManager wm;
};

extern class Network network;

#endif // _NETWORK_H