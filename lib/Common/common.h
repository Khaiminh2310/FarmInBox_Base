#ifndef _COMMON_H_
#define _COMMON_H_

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

/* THINGBOARD */
#define THINGSBOARD_SERVER  "thingsboard.cloud"
#define MQTT_PORT           1883
#define MQTT_SUB_TOPIC      "v1/devices/me/attributes"
#define MQTT_PUB_TELEMETRY  "v1/devices/me/telemetry"

// -----------------------------
// RX/TX pins for Modbus
// -----------------------------
#define RX_PIN 23
#define TX_PIN 22

// Serial2 (UART2) → Stepper trục Y & Z
#define STEPPER_YZ_RX_PIN 21
#define STEPPER_YZ_TX_PIN 19

// RS-485 Modbus
#define MODBUS_RX_PIN 17
#define MODBUS_TX_PIN 16
#define MAX485_RE     18
#define MAX485_DE     4

// -----------------------------
// Stepper IDs on each bus
// -----------------------------
#define X1_ID 0  // stepper1, channel 0 controls +X
#define X2_ID 1  // stepper1, channel 1 controls –X
#define Y_ID 2   // stepper2, channel 2 controls +Y/–Y
#define Z_ID 3   // stepper2, channel 3 controls +Z/–Z

// -----------------------------
// NeoPixel LED ring
// -----------------------------
#define LED_PIN     25
#define NUMPIXELS   16

// -----------------------------
// “SmartConfig” button
// -----------------------------
#define CONFIG_PIN              5
#define BUTTON_ACTIVE           LOW
#define CONFIG_TIME             3000    // ms

#define CONFIG_PORTAL_TIMEOUT   150     // s
#define WIFI_CONNECT_TIMEOUT    60      // s

// -----------------------------
// Remote attribute control pins
// -----------------------------
#define DEVICE_STATE1_PIN 26
#define DEVICE_STATE2_PIN 27

// -----------------------------
// SHTC3 Sensor address
// -----------------------------
#define SHTC3_ADDR_1    0x01
#define SHTC3_ADDR_2    0x02

// -----------------------------
// Pressure
// -----------------------------
#define PRESSURE_PIN    32 //ADC1_CH4
#define ADC_RESOLUTION  4095.0
#define V_REF           3.3
#define R1              10000.0
#define R2              10000.0

// -----------------------------
// Timing intervals
// -----------------------------
#define SENSOR_INTERVAL_S 10UL
#define NETWORK_INTERVAL_MS 200
#define MOTOR_INTERVAL_MS 50UL

/* DEBUG */
#if SERIAL_DEBUG
#define DEBUG_SERIAL(msg, ...) log_msgfmt(msg, ##__VA_ARGS__)
#else
#define DEBUG_SERIAL
#endif

/* RTOS */
#define STACK_SIZE_DEFAULT      1024
#define BUTTON_TASK             "BUTTON TASK"
#define CONFIG_TASK             "CONFIG TASK"
#define CHECK_CONNECTION_TASK   "CONNECTION TASK"
#define MQTT_TASK               "MQTT TASK"

/* WiFi */
#define AP_NAME "FarmInBox"
#define AP_PW   "12345678"

enum ColorMode {
  OFF = 0,
  RED = 1,
  GREEN = 2,
  BLUE = 3,
  PURPLE = 4
};

enum WorkMode {
  MANUAL = 0,
  AUTO = 1
};

struct dataSHTC3
{
  float temperature;
  float humidity;
};

struct dataPzem
{
  float volt;
  float ampe;
  float power;
  float energy;
  float freq;
  float powerFactor;
};

esp_timer_handle_t run_in_periodic_timer(esp_timer_cb_t cb, uint64_t period_ms);
void log_msgfmt(const char *msg="", ...);
void setColor(uint8_t r, uint8_t g, uint8_t b);
void ledToggle();
void ledBlue();
void ledGreen();
void ledRed();
void ledPurple();
void getShtc3Data();
void getPzemData();
void getPressureData();

extern SemaphoreHandle_t xSemaphoreWiFi;
extern SemaphoreHandle_t xSemaphoreConfig;

extern bool _param;
extern Adafruit_NeoPixel pixels;
extern ColorMode colorMode;
extern dataSHTC3 shtc3_data_1;
extern dataSHTC3 shtc3_data_2;
extern dataPzem pzem_data;
extern float pressure_value;

extern void suppend_tasks();
extern void resume_tasks();

#endif /* _COMMON_H_ */
