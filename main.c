#include "FreeRTOSConfig.h"
#include <espressif/esp_common.h>
#include <esp8266.h>
#include <esp/uart.h>
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include <timers.h>
//#include <ssid_config.h>
#include <httpd/httpd.h>
#include "sds011.h"

#define LED_PIN 2

static bool led_state = true;

void led(bool value) {
    gpio_write(LED_PIN, value);
}

static void blink_timer(TimerHandle_t t) {
    led_state = led_state ? false : true;
    /* printf("led_state = %i at %ul\n", led_state, xTaskGetTickCount()); */
    led(led_state);

    /* vTaskDelay(configTICK_RATE_HZ / 4); */
}

static void sds_timer(TimerHandle_t t) {
    struct sensor_state *state = sds011_read();
    if (state) {
        printf("SDS011\tPM2=%.1f\tPM10=%.1f\n", (float)state->pm2 / 10.0f, (float)state->pm10 / 10.0f);
    }
}

void user_init(void)
{
    uart_set_baud(0, 115200);
    printf("SDK version:%s\n", sdk_system_get_sdk_version());

    /* turn off LED */
    gpio_enable(LED_PIN, GPIO_OUTPUT);
    led(true);

    sds011_setup();

    TimerHandle_t t2 = xTimerCreate("sds011", configTICK_RATE_HZ / 8, pdTRUE, NULL, &sds_timer);
    xTimerStart(t2, 0);

    TimerHandle_t t1 = xTimerCreate("blinker", configTICK_RATE_HZ / 2, pdTRUE, NULL, &blink_timer);
    xTimerStart(t1, 0);

    printf("user_init done!\n");
}
