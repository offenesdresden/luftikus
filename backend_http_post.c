#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "backend_http_post.h"

#define SDS_API_PIN 1


static bool send_str(int sock, const char *str) {
  if (write(sock, str, strlen(str)) < 0) {
    printf("... socket send failed\r\n");
    close(sock);
    return false;
  }
  return true;
}

void http_post(const char *host, const uint16_t port, const char *path, const struct sensordatavalue *values) {
  const struct addrinfo hints = {
    .ai_family = AF_INET,
    .ai_socktype = SOCK_STREAM,
  };
  struct addrinfo *res;

  char port_str[6];
  snprintf(port_str, sizeof(port_str), "%u", port);
  printf("Running DNS lookup for %s:%s...\r\n", host, port_str);
  int err = getaddrinfo(host, port_str, &hints, &res);

  if(err != 0 || res == NULL) {
    printf("DNS lookup failed err=%d res=%p\r\n", err, res);
    if (res) {
      freeaddrinfo(res);
    }
    return;
  }
  /* Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
  struct in_addr *addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
  printf("DNS lookup succeeded. IP=%s\r\n", inet_ntoa(*addr));

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

  printf("... connected\r\n");
  freeaddrinfo(res);

  char body[128];
  snprintf(body, sizeof(body), "{"
           "\"sensordatavalues\":[");
  int bodylen;
  for(const struct sensordatavalue *v = values; v->value_type; v++) {
    bodylen = strlen(body);
    snprintf(body + bodylen, sizeof(body) - bodylen,
             "%s{"
             "\"value_type\":\"%s\","
             "\"value\":\"%s\""
             "}",
             (v == values ? "" : ","),
             v->value_type,
             v->value
      );
  }
  bodylen = strlen(body);
  snprintf(body + bodylen, sizeof(body) - bodylen, "]}");

  static char buf[128];
  snprintf(buf, sizeof(buf),
           "POST %s HTTP/1.1\r\n", path);
  if (!send_str(s, buf)) return;
  snprintf(buf, sizeof(buf),
           "Host: %s\r\n", host);
  if (!send_str(s, buf)) return;
  if (!send_str(s, "Connection: close\r\n")) return;
  if (!send_str(s, "Content-Type: application/json\r\n")) return;
  snprintf(buf, sizeof(buf),
           "Content-Length: %i\r\n", strlen(body));
  if (!send_str(s, buf)) return;
  snprintf(buf, sizeof(buf),
           "X-Sensor: esp8266-%u\r\n", sdk_system_get_chip_id());
  if (!send_str(s, buf)) return;
  snprintf(buf, sizeof(buf),
           "X-PIN: %u\r\n", SDS_API_PIN);
  if (!send_str(s, buf)) return;
  if (!send_str(s, "\r\n")) return;
  if (!send_str(s, body)) return;
  printf("... socket send success\r\n");

  int r;
  do {
    bzero(buf, sizeof(buf));
    r = read(s, buf, sizeof(buf) - 1);
  } while(r > 0);

  printf("... done reading from socket. Last read return=%d errno=%d\r\n", r, errno);
  close(s);
}
