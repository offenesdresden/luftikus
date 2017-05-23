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
      { .key = "path", .value = "/sensors/Aussen" },
      { .key = NULL, .value = NULL }
    },
    .flags = OUTPUT_ALL
}, {
    .name = "madavi.de",
    .interval = 30,
    .post_func = &http_post,
    .config = (struct output_config []){
      { .key = "host", .value = "api-rrd.madavi.de" },
      { .key = "path", .value = "/data.php" },
      { .key = NULL, .value = NULL }
    },
    .flags = OUTPUT_ALL
}, {
    .name = "luftdaten.info Feinstaub",
    .interval = 30,
    .post_func = &http_post,
    .config = (struct output_config []){
      { .key = "host", .value = "api.luftdaten.info" },
      { .key = "path", .value = "/v1/push-sensor-data/" },
      { .key = "x-pin", .value = "1" },
      { .key = NULL, .value = NULL }
    },
    .flags = OUTPUT_SDS011
}, {
    .name = "luftdaten.info Temp+Hum",
    .interval = 30,
    .post_func = &http_post,
    .config = (struct output_config []){
      { .key = "host", .value = "api.luftdaten.info" },
      { .key = "path", .value = "/v1/push-sensor-data/" },
      { .key = "x-pin", .value = "7" },
      { .key = NULL, .value = NULL }
    },
    .flags = OUTPUT_DHT22
}, {
    .name = "InfluxDB",
    .interval = 10,
    .post_func = &influx_post,
    .config = (struct output_config []){
      { .key = "host", .value = "flatbert.hq.c3d2.de" },
      { .key = "port", .value = "8086" },
      { .key = "path", .value = "/write?db=luftdaten" },
      { .key = NULL, .value = NULL }
    },
    .flags = OUTPUT_ALL
}, {
    .post_func = NULL
} };
