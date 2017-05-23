#include "FreeRTOSConfig.h"
#include <espressif/esp_common.h>
#include <esp8266.h>
#include <esp/uart.h>
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include <timers.h>
#include <httpd/httpd.h>

#include "shared.h"
#include "sds011.h"
#include "dht22.h"
#include "backend_http_post.h"
#include "backend_influxdb.h"
#include "config.h"

#define LED_PIN 2

void led(bool value) {
    gpio_write(LED_PIN, value);
}

struct sds011_state dust_state;
struct dht22_state dht_state;
static int last_update = 0;

TaskHandle_t output_task;

void resume_output_task() {
  vTaskResume(output_task);
}

static void sds_timer(TimerHandle_t t) {
    struct sds011_state *state = sds011_read();
    if (state) {
      printf("SDS011\tPM2=%.1f\tPM10=%.1f\n", state->p1, state->p2);
      last_update = now();
      memcpy(&dust_state, state, sizeof(dust_state));

      resume_output_task();
    }
}

static void dht_timer(TimerHandle_t t) {
    struct dht22_state *state = dht22_read();
    if (state) {
      printf("DHT22\tHumidity=%.1f%%\tTemperature=%.1f°C\n",
             state->humidity,
             state->temperature);
      last_update = now();
      memcpy(&dht_state, state, sizeof(dht_state));

      resume_output_task();
    }
}

static void output_loop(void *pvParameters) {
  for(;;) {
    vTaskSuspend(output_task);
    led(false);

    char p1_value[16];
    snprintf(p1_value, sizeof(p1_value), "%.1f", dust_state.pm10);
    char p2_value[16];
    snprintf(p2_value, sizeof(p2_value), "%.1f", dust_state.pm2);
    char temp_value[16];
    snprintf(temp_value, sizeof(temp_value), "%.1f", dht_state.temperature);
    char humid_value[16];
    snprintf(humid_value, sizeof(humid_value), "%.1f", dht_state.humidity);

    for(struct output_task *output = outputs; output->post_func; output++) {
      int time = now();
      if (time < output->last_run + output->interval) {
        /* Skip for now */
        continue;
      }
      output->last_run = time;
      printf("Running output \"%s\"\n", output->name);

      struct sensordata values[5];
      int values_i = 0;
      if (output->flags & OUTPUT_SDS011) {
        int only_sds = output->flags == OUTPUT_SDS011;
        values[values_i].name = only_sds ? "P1" : "SDS_P1";
        values[values_i].value = p1_value;
        values_i++;
        values[values_i].name = only_sds ? "P2" : "SDS_P2";
        values[values_i].value = p2_value;
        values_i++;
      }
      if (output->flags & OUTPUT_DHT22) {
        values[values_i].name = "temperature";
        values[values_i].value = temp_value;
        values_i++;
        values[values_i].name = "humidity";
        values[values_i].value = humid_value;
        values_i++;
      }
      values[values_i].name = NULL;
      values[values_i].value = NULL;

      output->post_func(output->config, values);
    }

    led(true);
  }
}

void user_init(void)
{
    uart_set_baud(0, 115200);
    printf("SDK version: %s\n", sdk_system_get_sdk_version());
    printf("Luftikus version: %s\n", VERSION);
    printf("Chip ID: %u\n", sdk_system_get_chip_id());
    printf("Timer/task stack depth: %u\n", configTIMER_TASK_STACK_DEPTH);

    /* turn off LED */
    gpio_enable(LED_PIN, GPIO_OUTPUT);
    led(true);

    sds011_setup();

    sdk_wifi_set_opmode(STATION_MODE);
    struct sdk_station_config wifi_config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PSK,
    };
    sdk_wifi_station_set_config(&wifi_config);
    sdk_wifi_station_connect();

    TimerHandle_t sds_timer_handle = xTimerCreate("sds011", configTICK_RATE_HZ / 2, pdTRUE, NULL, &sds_timer);
    xTimerStart(sds_timer_handle, 0);

    TimerHandle_t dht_timer_handle = xTimerCreate("dht22", configTICK_RATE_HZ / 2, pdTRUE, NULL, &dht_timer);
    xTimerStart(dht_timer_handle, 0);

    for(struct output_task *o = outputs; o->post_func; o++) {
      o->last_run = 0;
    }
    xTaskCreate(&output_loop, "output", 1024, NULL, 2, &output_task);

    printf("user_init done!\n");
}
