#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <espressif/esp_common.h>
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "shared.h"
#include "backend_influxdb.h"


static bool send_str(int sock, const char *str) {
  if (write(sock, str, strlen(str)) < 0) {
    printf("... socket send failed\r\n");
    close(sock);
    return false;
  }
  return true;
}

void influx_post(const struct output_config *config, const struct sensordata *values) {
  char *host = get_config(config, "host");
  char *port_str = get_config(config, "port");
  char *path = get_config(config, "path");
  if (!host || !port_str || !path) {
    printf("Invalid influx_post configuration!\n");
    return;
  }

  const struct addrinfo hints = {
    .ai_family = AF_INET,
    .ai_socktype = SOCK_STREAM,
  };
  struct addrinfo *res;

  int err = getaddrinfo(host, port_str, &hints, &res);

  if(err != 0 || res == NULL) {
    printf("DNS lookup failed err=%d res=%p\r\n", err, res);
    if (res) {
      freeaddrinfo(res);
    }
    return;
  }

  int s = socket(res->ai_family, res->ai_socktype, 0);
  if(s < 0) {
    printf("... Failed to allocate socket.\r\n");
    freeaddrinfo(res);
    return;
  }

  if (connect(s, res->ai_addr, res->ai_addrlen) != 0) {
    close(s);
    freeaddrinfo(res);
    printf("... socket connect failed.\r\n");
    return;
  }
  freeaddrinfo(res);

  char body[256];
  snprintf(body, sizeof(body), "feinstaub,node=esp8266-%u", sdk_system_get_chip_id());

  char *location = get_config(config, "location");
  if (location) {
    int bodylen = strlen(body);
    snprintf(body + bodylen, sizeof(body) - bodylen,
             ",location=%s", location);
  }

  for(const struct sensordata *v = values; v->name; v++) {
    int bodylen = strlen(body);
    snprintf(body + bodylen, sizeof(body) - bodylen,
             ",%s=%s",
             v->name,
             v->value
      );
  }

  char buf[128];
  snprintf(buf, sizeof(buf),
           "POST %s HTTP/1.1\r\n", path);
  if (!send_str(s, buf)) return;
  snprintf(buf, sizeof(buf),
           "Host: %s\r\n", host);
  if (!send_str(s, buf)) return;
  if (!send_str(s, "Connection: close\r\n")) return;
  if (!send_str(s, "Content-Type: application/x-www-form-urlencoded\r\n")) return;
  snprintf(buf, sizeof(buf),
           "Content-Length: %i\r\n", strlen(body));
  if (!send_str(s, buf)) return;
  if (!send_str(s, "\r\n")) return;
  if (!send_str(s, body)) return;

  printf("POST influx://%s:%s%s\n", host, port_str, path);

  int r;
  do {
    bzero(buf, sizeof(buf));
    r = read(s, buf, sizeof(buf) - 1);
  } while(r > 0);

  close(s);
}
