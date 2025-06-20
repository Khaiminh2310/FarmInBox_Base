#include "common.h"
#include "network.h"
#include "mqtt.h"
#include "RS485.h"
#include <RTOS.h>

Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
ColorMode colorMode = OFF;
WorkMode workMode = MANUAL;

dataSHTC3 shtc3_data;
dataPzem pzem_data;

bool _param = false;
bool config_triggered = false;

TaskHandle_t buttonTask = NULL;
TaskHandle_t mqttTask = NULL;
TaskHandle_t configPortalTask = NULL;
TaskHandle_t connectionTask = NULL;
TaskHandle_t sendDataTask = NULL;

SemaphoreHandle_t xSemaphoreWiFi = NULL;
SemaphoreHandle_t xSemaphoreConfig = NULL;
SemaphoreHandle_t xMutexConfig = NULL;

void button_task(void *pvParameters);
void config_portal_task(void *pvParameters);
void mqtt_task(void *pvParameters);
void connection_task(void *pvParameters);
void send_data_task(void* pvParameters);
void suppend_tasks();
void resume_tasks();

void setup()
{
    Serial.begin(115200);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    DEBUG_SERIAL("Starting...");

    pixels.begin();
    pixels.show();

    pinMode(CONFIG_PIN, INPUT_PULLUP);

    _param = network.getParameters();

    xSemaphoreConfig = xSemaphoreCreateBinary();
    xMutexConfig = xSemaphoreCreateMutex();
    xSemaphoreWiFi = xSemaphoreCreateBinary();

    xTaskCreatePinnedToCore(button_task, "buttonTask", STACK_SIZE_DEFAULT * 6, NULL, 1, &buttonTask, 1);
    xTaskCreatePinnedToCore(config_portal_task, "configPortalTask", STACK_SIZE_DEFAULT * 8, NULL, 1, &configPortalTask, 1);

    network.initWifi();
    // --> WiFi connected
    if (_param)
    {
        xTaskCreatePinnedToCore(mqtt_task, "mqttTask", STACK_SIZE_DEFAULT * 6, NULL, 2, &mqttTask, 1);
        _param = false;
    }

    if (connectionTask == NULL)
    {
        while (mqttTask != NULL)
        {
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
        xTaskCreatePinnedToCore(connection_task, "connectionTask", STACK_SIZE_DEFAULT * 8, NULL, 3, &connectionTask, 1);
    }
}

void loop() {}

void button_task(void *pvParameters)
{
    DEBUG_SERIAL("Create %s", BUTTON_TASK);
    unsigned long _start = 0;
    unsigned long last_change = 0;
    int press_count = 0;
    int last_button_state = digitalRead(CONFIG_PIN);

    while (1)
    {
        if (xSemaphoreTake(xMutexConfig, portMAX_DELAY) == pdTRUE)
        {
            int button_state = digitalRead(CONFIG_PIN);

            if (button_state != last_button_state)
            {
                if (button_state == BUTTON_ACTIVE)
                {
                    DEBUG_SERIAL("[%s] Button is pressed", BUTTON_TASK);
                    press_count++;
                }
                last_button_state = button_state;
                last_change = millis();
            }
            if (millis() - last_change > CONFIG_TIME)
            {
                press_count = 0;
            }
            if (press_count == 3)
            {
                DEBUG_SERIAL("[%s] Triple press detected", BUTTON_TASK);
                workMode = AUTO;
                press_count = 0;
            }

            if (button_state == BUTTON_ACTIVE)
            {
                if (_start == 0)
                {
                    _start = millis();
                }
                else if (!config_triggered && millis() - _start >= CONFIG_TIME)
                {
                    DEBUG_SERIAL("[%s] Button is pressed (hold)", BUTTON_TASK);
                    ledPurple();
                    xSemaphoreGive(xSemaphoreConfig);
                    config_triggered = true;
                }
            }
            else
            {
                _start = 0;
                config_triggered = false;
            }

            xSemaphoreGive(xMutexConfig);
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void config_portal_task(void *pvParameters)
{
    DEBUG_SERIAL("Create %s", CONFIG_TASK);
    while (1)
    {
        if (xSemaphoreTake(xSemaphoreConfig, portMAX_DELAY) == pdTRUE)
        {
            if (xSemaphoreTake(xMutexConfig, portMAX_DELAY) == pdTRUE)
            {
                if (connectionTask != NULL)
                {
                    DEBUG_SERIAL("[%s] Suspending %s...", CONFIG_TASK, CHECK_CONNECTION_TASK);
                    vTaskSuspend(connectionTask);
                }

                bool config = network.portalConfig();

                if (network.getParameters() && network.new_config && config)
                {
                    _param = false;
                    xTaskCreatePinnedToCore(mqtt_task, "mqttTask", STACK_SIZE_DEFAULT * 6, NULL, 2, &mqttTask, 1);
                }

                if (config && xSemaphoreWiFi != NULL)
                {
                    xSemaphoreGive(xSemaphoreWiFi);
                }

                xSemaphoreGive(xMutexConfig);
                DEBUG_SERIAL("[%s] End config portal.", CONFIG_TASK);

                if (connectionTask != NULL)
                {
                    while (mqttTask != NULL)
                    {
                        vTaskDelay(200 / portTICK_PERIOD_MS);
                    }
                    DEBUG_SERIAL("[%s] Resuming %s...", CONFIG_TASK, CHECK_CONNECTION_TASK);
                    vTaskResume(connectionTask);
                }
            }
        }
    }
}

void connection_task(void *pvParameters)
{
    DEBUG_SERIAL("Create %s", CHECK_CONNECTION_TASK);
    unsigned long wifi_lost_time = 0;
    const unsigned long WIFI_RECONNECT_INTERVAL = 60000;
    while (1)
    {
        bool wifi_connected = (WiFi.status() == WL_CONNECTED);
        DEBUG_SERIAL("[%s] %s", CHECK_CONNECTION_TASK, wifi_connected ? "WiFi connected" : "WiFi connection lost!");

        if (wifi_connected)
        {
            wifi_lost_time = 0;
            if (!mqtt.loop())
            {
                DEBUG_SERIAL("[%s] MQTT connection lost!", CHECK_CONNECTION_TASK);
                if (colorMode != BLUE)
                {
                    ledBlue();
                }
                mqtt.connect();
            }
            else
            {
                DEBUG_SERIAL("[%s] MQTT connected", CHECK_CONNECTION_TASK);
                if (colorMode != GREEN)
                {
                    ledGreen();
                }
            }
        }
        else
        {
            ledToggle();
            if (wifi_lost_time == 0)
            {
                wifi_lost_time = millis();
            }
            else if (millis() - wifi_lost_time > WIFI_RECONNECT_INTERVAL)
            {
                DEBUG_SERIAL("[%s] WiFi reconnect timeout reached. Restarting WiFi...", CHECK_CONNECTION_TASK);
                WiFi.disconnect();
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                WiFi.begin(network.ssid.c_str(), network.password.c_str());
                wifi_lost_time = millis();
            }
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void mqtt_task(void *pvParameters)
{
    DEBUG_SERIAL("Create %s", MQTT_TASK);
    if (network.new_config)
    {
        DEBUG_SERIAL("[%s] Disconnect old MQTT connection", MQTT_TASK);
        mqtt.disconnect();
        network.new_config = false;
    }
    DEBUG_SERIAL("[%s] Setup MQTT connection...", MQTT_TASK);
    mqtt.setup();

    DEBUG_SERIAL("Deleted %s", MQTT_TASK);
    mqttTask = NULL;
    vTaskDelete(NULL);
}

void send_data_task(void* pvParameters)
{
  const TickType_t xCycle = pdMS_TO_TICKS(10000);
  TickType_t xLastWakeTime = xTaskGetTickCount();

  while (1)
  {
    getShtc3Data();
    getPzemData();

    if (mqtt.isConnected())
    {
        //@TODO: mqtt.publish
    }

    vTaskDelayUntil(&xLastWakeTime, xCycle);
  }
}

void suppend_tasks()
{
    if (buttonTask != NULL)
    {
        DEBUG_SERIAL("Suspending %s...", BUTTON_TASK);
        vTaskSuspend(buttonTask);
    }
    if (configPortalTask != NULL)
    {
        DEBUG_SERIAL("Suspending %s...", CONFIG_TASK);
        vTaskSuspend(configPortalTask);
    }
    if (connectionTask != NULL)
    {
        DEBUG_SERIAL("Suspending %s...", CHECK_CONNECTION_TASK);
        vTaskSuspend(connectionTask);
    }
}

void resume_tasks()
{
    if (configPortalTask != NULL)
    {
        DEBUG_SERIAL("Resuming %s...", CONFIG_TASK);
        vTaskResume(configPortalTask);
    }
    if (connectionTask != NULL)
    {
        DEBUG_SERIAL("Resuming %s...", CHECK_CONNECTION_TASK);
        vTaskResume(connectionTask);
    }
    if (buttonTask != NULL)
    {
        DEBUG_SERIAL("Resuming %s...", BUTTON_TASK);
        vTaskResume(buttonTask);
    }
}
