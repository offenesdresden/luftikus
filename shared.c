#include <string.h>
#include <stdio.h>

#include "shared.h"

char *get_config(const struct output_config *config, const char *key) {
  for(const struct output_config *c = config; c->key; c++) {
    if (!strcmp(c->key, key)) {
      return c->value;
    }
  }

  return NULL;
}

int now() {
  return xTaskGetTickCountFromISR() / configTICK_RATE_HZ;
}
