#include "espressif/esp_common.h"
#include "esp8266.h"
#include <dht/dht.h>

#include "dht22.h"

const uint8_t DHT22_PIN = 13;

void dht22_setup() {
  gpio_set_pullup(DHT22_PIN, false, false);
}

static struct dht22_state state;

struct dht22_state *dht22_read() {
  int16_t humidity;
  int16_t temperature;
  if (dht_read_data(DHT_TYPE_DHT22, DHT22_PIN, &humidity, &temperature)) {
    state.humidity = (float)humidity / 10.0f;
    state.temperature = (float)temperature / 10.0f;
    return &state;
  } else {
    return NULL;
  }
}
