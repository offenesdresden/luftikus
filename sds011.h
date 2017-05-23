static const uint8_t UART_NO_SDS011 = 1;

void sds011_setup();

struct sds011_state {
    // unit: 1 µg/m³
    float p1;
    float p2;
};
struct sds011_state *sds011_read();
