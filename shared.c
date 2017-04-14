#include <string.h>
#include <stdio.h>

#include "shared.h"

char *get_config(const struct output_config *config, const char *key) {
  for(const struct output_config *c = config; c->key; c++) {
    if (!strcmp(c->key, key)) {
      return c->value;
    }
  }

  printf("Expected configuration key not found: %s\n", key);
  return "";
}
