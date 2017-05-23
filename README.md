# Synopsis

FreeRTOS-based firmware for ESP8266/NodeMCU boards to collect and send
sensor data

See original code for Arduino at https://github.com/opendata-stuttgart/sensors-software/blob/master/esp8266-arduino/ppd42ns-wificonfig-ppd-sds-dht/ppd42ns-wificonfig-ppd-sds-dht.ino


# Instructions

## Prepare esp-open-sdk

```shell
sudo apt-get install make unrar-free autoconf automake libtool gcc g++ gperf \
    flex bison texinfo gawk ncurses-dev libexpat-dev python-dev python python-serial \
    sed git unzip bash help2man wget bzip2

git clone --recursive https://github.com/pfalcon/esp-open-sdk.git
cd esp-open-sdk
make toolchain esptool libhal

# Make toolchain known to this shell
export PATH=$PATH:`pwd`/xtensa-lx106-elf/bin
```

## Build + flash

First, edit `config.h` for configuration.

```shell
git clone --recursive https://github.com/offenesdresden/luftikus.git
cd luftikus
make flash ESPPORT=/dev/ttyUSB0
```

Attach the serial console to see `printf()` output:
```shell
cu -s 115200 -l /dev/ttyUSB0
```


# TODO

* don't send 0 values for missing sensors
* more sensors
* https
* mdns
* mqtt
* http server with data.json
* web interface
* setup mode
