#include "common.h"
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