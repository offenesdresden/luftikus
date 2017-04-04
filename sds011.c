#include <stdio.h>
/* #include "FreeRTOS.h" */
/* #include <task.h> */
#include <softuart/softuart.h>
#include "sds011.h"

const uint8_t RX_PIN = 5;
const uint8_t TX_PIN = 4;

void sds011_setup() {
    softuart_open(UART_NO_SDS011, 9600, RX_PIN, TX_PIN);
}

static struct sensor_state state;

/* Circular buffer to match start/end in data */
static uint8_t input_buf[10];
static uint8_t *input = input_buf;
static uint8_t *input_buf_end = input_buf + sizeof(input_buf);
static uint8_t input_buf_size = 0;

static uint8_t get_input_buf(int8_t offset) {
    uint8_t *i = input + offset;
    while(i < input_buf) i += sizeof(input_buf);
    return *i;
}

struct sensor_state *sds011_read() {
    while (softuart_available(UART_NO_SDS011)) {
        *input = softuart_read(UART_NO_SDS011);
        input++;
        if (input >= input_buf_end) input = input_buf;
        if (input_buf_size < sizeof(input_buf)) input_buf_size++;

        if (input_buf_size < sizeof(input_buf)) continue;
        if (get_input_buf(-9) != 0xAA) continue;
        if (get_input_buf(-8) != 0xC0) continue;
        if (get_input_buf(0) != 0xAB) continue;

        uint8_t expected_checksum = 0;

        uint8_t pm2a = get_input_buf(-7);
        uint8_t pm2b = get_input_buf(-6);
        state.pm2 = ((uint16_t)pm2b << 8) | ((uint16_t)pm2a);
        expected_checksum += pm2a + pm2b;

        uint8_t pm10a = get_input_buf(-5);
        uint8_t pm10b = get_input_buf(-4);
        state.pm10 = ((uint16_t)pm10b << 8) | ((uint16_t)pm10a);
        expected_checksum += pm10a + pm10b;

        expected_checksum += get_input_buf(-3);
        expected_checksum += get_input_buf(-2);

        uint8_t actual_checksum = get_input_buf(-1);
        if (expected_checksum != actual_checksum) {
            printf("SDS011 checksum mismatch: expected %02X != %02X\n", expected_checksum, actual_checksum);
            continue;
        }

        return &state;
    }

    return NULL;
}
