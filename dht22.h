void dht22_setup();

struct dht22_state {
  // Stored with sensor-native scale: .1
  int16_t temperature;
  int16_t humidity;
};

struct dht22_state *dht22_read();
