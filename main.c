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

struct sensor_state dust_state;
struct dht22_state dht_state;
static int last_update = 0;

void resume_output_tasks() {
  int time = now();
  for(struct output_task *o = outputs; o->post_func; o++) {
    if (time >= o->last_run + o->interval) {
      o->last_run = time;
      vTaskResume(o->task);
    }
  }
}

static void sds_timer(TimerHandle_t t) {
    struct sensor_state *state = sds011_read();
    if (state) {
      printf("SDS011\tPM2=%.1f\tPM10=%.1f\n", state->pm2, state->pm10);
      last_update = now();
      memcpy(&dust_state, state, sizeof(dust_state));

      resume_output_tasks();
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

      resume_output_tasks();
    }
}

static void output_task(void *pvParameters) {
  struct output_task *output = pvParameters;

  for(;;) {
    vTaskSuspend(output->task);
    printf("Resumed task \"%s\"\n", output->name);

    led(false);

    char p1_value[16];
    char p2_value[16];
    char temp_value[16];
    char humid_value[16];
    struct sensordata values[5];
    int values_i = 0;
    if (output->flags & OUTPUT_SDS011) {
      snprintf(p1_value, sizeof(p1_value), "%.1f", dust_state.pm10);
      snprintf(p2_value, sizeof(p2_value), "%.1f", dust_state.pm2);
      int only_sds = output->flags & OUTPUT_SDS011;
      values[values_i].name = only_sds ? "P1" : "SDS_P1";
      values[values_i].value = p1_value;
      values_i++;
      values[values_i].name = only_sds ? "P2" : "SDS_P2";
      values[values_i].value = p2_value;
      values_i++;
    }
    if (output->flags & OUTPUT_DHT22) {
      snprintf(temp_value, sizeof(temp_value), "%.1f", dht_state.temperature);
      snprintf(humid_value, sizeof(humid_value), "%.1f", dht_state.humidity);
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
      xTaskCreate(&output_task, o->name, 1024, o, 2, &o->task);
    }

    printf("user_init done!\n");
}
