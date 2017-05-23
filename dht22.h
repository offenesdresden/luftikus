void dht22_setup();

struct dht22_state {
  float temperature;
  float humidity;
};

struct dht22_state *dht22_read();
