struct sensordatavalue {
  char *value_type;
  char *value;
};

void http_post(const char *host, const uint16_t port, const char *path, const struct sensordatavalue *values);
