static const uint8_t UART_NO_SDS011 = 1;

void sds011_setup();

struct sensor_state {
    // Stored as sensor-native unit: .1 µg/m³
    uint16_t pm2;
    uint16_t pm10;
};
struct sensor_state *sds011_read();
