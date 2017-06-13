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
#include "timeseries.h"
#include "config.h"

#define LED_PIN 2

void led(bool value) {
    gpio_write(LED_PIN, value);
}

timeseries_t sds011_p1;
timeseries_t sds011_p2;
timeseries_t dht22_temp;
timeseries_t dht22_humid;

TaskHandle_t output_task;

void resume_output_task() {
  vTaskResume(output_task);
}

static void sds_timer(TimerHandle_t t) {
    struct sds011_state *state = sds011_read();
    if (state) {
      printf("SDS011\tPM2=%.1f\tPM10=%.1f\n", state->p1, state->p2);
      timeseries_add(&sds011_p1, state->p1);
      timeseries_add(&sds011_p2, state->p2);

      resume_output_task();
    }
}

static void dht_timer(TimerHandle_t t) {
    struct dht22_state *state = dht22_read();
    if (state) {
      printf("DHT22\tHumidity=%.1f%%\tTemperature=%.1f°C\n",
             state->humidity,
             state->temperature);
      timeseries_add(&dht22_temp, state->temperature);
      timeseries_add(&dht22_humid, state->humidity);

      resume_output_task();
    }
}

static void output_loop(void *pvParameters) {
  for(;;) {
    vTaskSuspend(output_task);
    led(false);

    int outputs_done = 0;
    for(struct output_task *output = outputs; output->post_func; output++) {
      int time = now();
      if (time < output->last_run) {
        /* Current time is less than last_run, clock as overflown */
        output->last_run = time;
        continue;
      }
      if (time < output->last_run + output->interval) {
        /* Interval has not yet elapsed */
        continue;
      }
      int previous_run = output->last_run;
      output->last_run = time;
      printf("Running output \"%s\"\n", output->name);

      struct sensordata values[5];
      int values_i = 0;
      char p1_value[16];
      char p2_value[16];
      if (output->flags & OUTPUT_SDS011) {
        snprintf(p1_value, sizeof(p1_value), "%.1f", timeseries_median_since(&sds011_p1, previous_run));
        snprintf(p2_value, sizeof(p2_value), "%.1f", timeseries_median_since(&sds011_p2, previous_run));

        int only_sds = output->flags == OUTPUT_SDS011;
        values[values_i].name = only_sds ? "P1" : "SDS_P1";
        values[values_i].value = p1_value;
        values_i++;
        values[values_i].name = only_sds ? "P2" : "SDS_P2";
        values[values_i].value = p2_value;
        values_i++;
      }
      char temp_value[16];
      char humid_value[16];
      if (output->flags & OUTPUT_DHT22) {
        snprintf(temp_value, sizeof(temp_value), "%.1f", timeseries_median_since(&dht22_temp, previous_run));
        snprintf(humid_value, sizeof(humid_value), "%.1f", timeseries_median_since(&dht22_humid, previous_run));

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
      outputs_done++;
    }
    if (outputs_done > 0)
      printf("%i outputs done\n", outputs_done);

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

    timeseries_init(&sds011_p1);
    timeseries_init(&sds011_p2);
    TimerHandle_t sds_timer_handle = xTimerCreate("sds011", configTICK_RATE_HZ / 2, pdTRUE, NULL, &sds_timer);
    xTimerStart(sds_timer_handle, 0);

    timeseries_init(&dht22_temp);
    timeseries_init(&dht22_humid);
    TimerHandle_t dht_timer_handle = xTimerCreate("dht22", configTICK_RATE_HZ / 2, pdTRUE, NULL, &dht_timer);
    xTimerStart(dht_timer_handle, 0);

    for(struct output_task *o = outputs; o->post_func; o++) {
      o->last_run = 0;
    }
    xTaskCreate(&output_loop, "output", 1024, NULL, 2, &output_task);

    printf("user_init done!\n");
}
