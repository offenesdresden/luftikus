#include "FreeRTOSConfig.h"
#include <espressif/esp_common.h>
#include <esp8266.h>
#include <esp/uart.h>
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include <timers.h>
#include <httpd/httpd.h>
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
static TickType_t last_update = 0;
static TaskHandle_t http_post_task;
static TaskHandle_t influxdb_task;

static void sds_timer(TimerHandle_t t) {
    struct sensor_state *state = sds011_read();
    if (state) {
      printf("SDS011\tPM2=%.1f\tPM10=%.1f\n", (float)state->pm2 / 10.0f, (float)state->pm10 / 10.0f);
      last_update = xTaskGetTickCountFromISR();
      memcpy(&dust_state, state, sizeof(dust_state));
      vTaskResume(http_post_task);
      vTaskResume(influxdb_task);
    }
}

static void dht_timer(TimerHandle_t t) {
    struct dht22_state *state = dht22_read();
    if (state) {
      printf("DHT22\tHumidity=%.1f%%\tTemperature=%.1f°C\n",
             (float)state->humidity / 10.0f,
             (float)state->temperature / 10.0f);
      last_update = xTaskGetTickCountFromISR();
      memcpy(&dht_state, state, sizeof(dht_state));
      vTaskResume(http_post_task);
      vTaskResume(influxdb_task);
    }
}

static void http_post_task_loop(void *pvParameters) {
  TickType_t last_resume;
  while(1) {
    vTaskSuspend(http_post_task);
    last_resume = xTaskGetTickCount();

    led(false);
    char p1_value[16];
    snprintf(p1_value, sizeof(p1_value), "%.1f", (float)dust_state.pm10 / 10.0f);
    char p2_value[16];
    snprintf(p2_value, sizeof(p2_value), "%.1f", (float)dust_state.pm2 / 10.0f);
    char temp_value[16];
    snprintf(temp_value, sizeof(temp_value), "%.1f", (float)dht_state.temperature / 10.0f);
    char humid_value[16];
    snprintf(humid_value, sizeof(humid_value), "%.1f", (float)dht_state.humidity / 10.0f);
    struct sensordatavalue values[] = {
      { .value_type = "SDS_P1", .value = p1_value },
      { .value_type = "SDS_P2", .value = p2_value },
      { .value_type = "temperature", .value = temp_value },
      { .value_type = "humidity", .value = humid_value },
      { .value_type = NULL, .value = NULL }
    };
    http_post("api-rrd.madavi.de", 80, "/data.php", values);
    led(true);

    vTaskDelayUntil(&last_resume, 30 * configTICK_RATE_HZ);
  }
}

static void influxdb_task_loop(void *pvParameters) {
  TickType_t last_resume;
  while(1) {
    vTaskSuspend(influxdb_task);
    last_resume = xTaskGetTickCount();

    led(false);
    char p1_value[16];
    snprintf(p1_value, sizeof(p1_value), "%.1f", (float)dust_state.pm10 / 10.0f);
    char p2_value[16];
    snprintf(p2_value, sizeof(p2_value), "%.1f", (float)dust_state.pm2 / 10.0f);
    char temp_value[16];
    snprintf(temp_value, sizeof(temp_value), "%.1f", (float)dht_state.temperature / 10.0f);
    char humid_value[16];
    snprintf(humid_value, sizeof(humid_value), "%.1f", (float)dht_state.humidity / 10.0f);
    struct influx_sensordatavalue values[] = {
      { .value_type = "SDS_P1", .value = p1_value },
      { .value_type = "SDS_P2", .value = p2_value },
      { .value_type = "temperature", .value = temp_value },
      { .value_type = "humidity", .value = humid_value },
      { .value_type = NULL, .value = NULL }
    };
    influx_post("flatbert.hq.c3d2.de", 8086, "/write?db=luftdaten-innen", NULL, values);
    led(true);

    /* vTaskDelayUntil(&last_resume, 30 * configTICK_RATE_HZ); */
  }
}

void user_init(void)
{
    uart_set_baud(0, 115200);
    printf("SDK version: %s\n", sdk_system_get_sdk_version());
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

    TimerHandle_t sds_timer_handle = xTimerCreate("sds011", configTICK_RATE_HZ / 8, pdTRUE, NULL, &sds_timer);
    xTimerStart(sds_timer_handle, 0);

    TimerHandle_t dht_timer_handle = xTimerCreate("dht22", configTICK_RATE_HZ / 8, pdTRUE, NULL, &dht_timer);
    xTimerStart(dht_timer_handle, 0);

    xTaskCreate(&http_post_task_loop, "http_post", 1024, NULL, 2, &http_post_task);
    xTaskCreate(&influxdb_task_loop, "influxdb", 1024, NULL, 2, &influxdb_task);

    printf("user_init done!\n");
}
