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
#include "backend_http_post.h"

#define LED_PIN 2

void led(bool value) {
    gpio_write(LED_PIN, value);
}

struct sensor_state last_state;
static TickType_t last_update = 0;
static TaskHandle_t http_post_task;

static void sds_timer(TimerHandle_t t) {
    struct sensor_state *state = sds011_read();
    if (state) {
      printf("SDS011\tPM2=%.1f\tPM10=%.1f\n", (float)state->pm2 / 10.0f, (float)state->pm10 / 10.0f);
      last_update = xTaskGetTickCountFromISR();
      memcpy(&last_state, state, sizeof(last_state));
      vTaskResume(http_post_task);
    }
}

static void http_post_task_loop(void *pvParameters) {
  TickType_t last_resume;
  while(1) {
    vTaskSuspend(http_post_task);
    last_resume = xTaskGetTickCount();

    led(false);
    char p1_value[16];
    snprintf(p1_value, sizeof(p1_value), "%.1f", (float)last_state.pm10 / 10.0f);
    char p2_value[16];
    snprintf(p2_value, sizeof(p2_value), "%.1f", (float)last_state.pm2 / 10.0f);
    struct sensordatavalue values[] = {
      { .value_type = "SDS_P1", .value = p1_value },
      { .value_type = "SDS_P2", .value = p2_value },
      { .value_type = NULL, .value = NULL }
    };
    http_post("api-rrd.madavi.de", 80, "/data.php", values);
    led(true);

    vTaskDelayUntil(&last_resume, 30 * configTICK_RATE_HZ);
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
        .ssid = "internet",
        .password = "",
    };
    sdk_wifi_station_set_config(&wifi_config);
    sdk_wifi_station_connect();

    TimerHandle_t t2 = xTimerCreate("sds011", configTICK_RATE_HZ / 8, pdTRUE, NULL, &sds_timer);
    xTimerStart(t2, 0);

    xTaskCreate(&http_post_task_loop, "http_post", 256, NULL, 2, &http_post_task);

    printf("user_init done!\n");
}
