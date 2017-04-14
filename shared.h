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

struct output_task {
  // State
  TaskHandle_t task;
  TickType_t last_resume;

  // Configuration in config.h
  const char *name;
  void (*post_func)(const struct output_config *, const struct sensordata[]);
  struct output_config *config;
};
