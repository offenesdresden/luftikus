/**
 * Configure your WiFi
 **/
#define WIFI_SSID "internet"
#define WIFI_PSK ""

/**
 * Configure outputs
 **/
static struct output_task outputs[] = { {
    .name = "SpaceAPI",
    .interval = 3,
    .post_func = &http_post,
    .config = (struct output_config []){
      { .key = "host", .value = "www.hq.c3d2.de" },
      { .key = "port", .value = "3000" },
      { .key = "path", .value = "/sensors/Innen" },
      { .key = NULL, .value = NULL }
    }
}, {
    .name = "InfluxDB",
    .interval = 10,
    .post_func = &influx_post,
    .config = (struct output_config []){
      { .key = "host", .value = "flatbert.hq.c3d2.de" },
      { .key = "port", .value = "8086" },
      { .key = "path", .value = "/write?db=luftdaten" },
      { .key = NULL, .value = NULL }
    }
}, {
    .post_func = NULL
} };
