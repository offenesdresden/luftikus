static const uint8_t UART_NO_SDS011 = 1;

void sds011_setup();

struct sensor_state {
    // unit: 1 µg/m³
    float pm2;
    float pm10;
};
struct sensor_state *sds011_read();
