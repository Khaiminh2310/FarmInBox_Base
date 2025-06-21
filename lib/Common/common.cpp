#include "common.h"
#include "RS485.h"
#include <esp_wifi.h>

///////////////////////////////// COMMON FUNCTIONS /////////////////////////////////

esp_timer_handle_t run_in_periodic_timer(esp_timer_cb_t cb, uint64_t period_ms)
{
    esp_timer_create_args_t timer_args;

    timer_args.callback = cb;
    timer_args.arg = NULL;
    timer_args.dispatch_method = ESP_TIMER_TASK;
    esp_timer_handle_t timer;
    esp_timer_create(&timer_args, &timer);
    esp_timer_start_periodic(timer, period_ms); // period (in microseconds)

    return timer;
}

void log_msgfmt(const char *msg, ...)
{
    if (!Serial || !Serial.availableForWrite())
    {
        return;
    }

    if (msg[0] == '\0')
    {
        Serial.println();
        return;
    }

    char buffer[200];
    int length = 0;

    va_list args;

    uint32_t curent_millis = millis();
    length = snprintf(buffer, sizeof(buffer), "[%lu] ", curent_millis);
    if (length < 0 || length >= sizeof(buffer))
    {
        return;
    }
    int remaining_buffer_size = sizeof(buffer) - length;
    char *msg_buffer = buffer + length;

    va_start(args, msg);
    length += vsnprintf(msg_buffer, remaining_buffer_size, msg, args);
    va_end(args);

    if (length + 1 < sizeof(buffer))
    {
        Serial.println(buffer);
    }
    else
    {
        buffer[sizeof(buffer) - 1] = '\0';
        Serial.println(buffer);
    }
}

void getShtc3Data()
{
    shtc3.begin(4800);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    shtc3.setAddr(SHTC3_ADDR_1);
    uint16_t data[2] = {0};
    if (shtc3.readHoldingRegisters(0x0000, 2, data))
    {
        DEBUG_SERIAL("Read SHTC3 1st success.");
        shtc3_data_1.humidity = data[0] / 10.0f;
        if (data[1] >= 32768.0) // Negative value
        {
            data[1] = data[1] - 65536.0;
        }
        shtc3_data_1.temperature = data[1] / 10.0;
    }
    else
    {
        DEBUG_SERIAL("Read SHTC3 1st fail.");
        shtc3_data_1.humidity = 0.0f;
        shtc3_data_1.temperature = 0.0f;
    }

    shtc3.setAddr(SHTC3_ADDR_2);
    if (shtc3.readHoldingRegisters(0x0000, 2, data))
    {
        DEBUG_SERIAL("Read SHTC3 2nd success.");
        shtc3_data_2.humidity = data[0] / 10.0f;
        if (data[1] >= 32768.0) // Negative value
        {
            data[1] = data[1] - 65536.0;
        }
        shtc3_data_2.temperature = data[1] / 10.0;
    }
    else
    {
        DEBUG_SERIAL("Read SHTC3 2nd fail.");
        shtc3_data_2.humidity = 0.0f;
        shtc3_data_2.temperature = 0.0f;
    }
    DEBUG_SERIAL("Temp: %.02f oC - Hum: %.02f %%", shtc3_data_1.temperature, shtc3_data_1.humidity);
    DEBUG_SERIAL("Temp2: %.02f oC - Hum2: %.02f %%", shtc3_data_2.temperature, shtc3_data_2.humidity);
}

void getPzemData()
{
    pzem.begin(9600);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    uint16_t data[REG_TOTAL] = {0};
    if (pzem.readInputRegisters(0x0000, 10, data))
    {
        DEBUG_SERIAL("Read PZEM004T success.");
        pzem_data.volt         = PZEM_GET_VALUE(REG_VOLTAGE, SCALE_V);
        pzem_data.ampe         = PZEM_GET_VALUE2(REG_CURRENT_H, REG_CURRENT_L, SCALE_A);
        pzem_data.power        = PZEM_GET_VALUE2(REG_POWER_H, REG_POWER_L, SCALE_P);
        pzem_data.energy       = PZEM_GET_VALUE2(REG_ENERGY_H, REG_ENERGY_L, SCALE_E);
        pzem_data.freq         = PZEM_GET_VALUE(REG_FREQ, SCALE_H);
        pzem_data.powerFactor  = PZEM_GET_VALUE(REG_PF, SCALE_PF);
    }
    else
    {
        DEBUG_SERIAL("Read PZEM004T fail.");
        pzem_data.volt = 0.0;
        pzem_data.ampe = 0.0;
        pzem_data.power = 0.0;
        pzem_data.energy = 0.0;
        pzem_data.freq = 0.0;
        pzem_data.powerFactor = 0.0;
    }
}

void getPressureData()
{
    uint16_t value;
    if (pressure.readHoldingRegisters(0x0000, 1, &value))
    {
        float voltage = value * (5.0 / 4095.0);
        pressure_value = (voltage - 0.5) / 4.0;
    }
    else
    {
        pressure_value = -1.0f;
    }
}

void setColor(uint8_t r, uint8_t g, uint8_t b)
{
  for (int i = 0; i < NUMPIXELS; i++)
  {
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  }
  pixels.show();
}

// -----------------------------
// Try connect to WiFi, flashing blue
// -----------------------------
void ledToggle()
{
    if (colorMode != OFF)
    {
        setColor(0, 0, 0);
    }
    else
    {
        ledBlue();
    }

}

// -----------------------------
// WiFi connected but MQTT not connected
// -----------------------------
void ledBlue()
{
  setColor(0, 0, 150);
  colorMode = BLUE;
}

// -----------------------------
// WiFi & MQTT connected
// -----------------------------
void ledGreen()
{
  setColor(0, 150, 0);
  colorMode = GREEN;
}

// -----------------------------
// WiFi not connected
// -----------------------------
void ledRed()
{
  setColor(150, 0, 0);
  colorMode = RED;
}

// -----------------------------
// Start config
// -----------------------------
void ledPurple()
{
  setColor(150, 0, 150);
  colorMode = PURPLE;
}