#include "FreeRTOS.h"
#include <task.h>

struct sensordata {
  char *name;
  char *value;
};

struct output_config {
  char *key;
  char *value;
};
char *get_config(const struct output_config *config, const char *key);

enum output_flags {
  OUTPUT_SDS011 = 0x01,
  OUTPUT_DHT22 = 0x02,
  OUTPUT_ALL = 0xff,
};

struct output_task {
  // State
  TaskHandle_t task;
  int last_run;

  // Configuration in config.h
  const char *name;
  const int interval;
  void (*post_func)(const struct output_config *, const struct sensordata[]);
  struct output_config *config;
  enum output_flags flags;
};

// returns seconds
int now();
