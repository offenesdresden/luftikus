struct influx_sensordatavalue {
  char *value_type;
  char *value;
};

void influx_post(const char *host, const uint16_t port, const char *path, const char *auth, const struct influx_sensordatavalue *values);
