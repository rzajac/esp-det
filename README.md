## The ESP8266 detection and configuration library.

The purpose of this library is to manage initial detection and configuration of 
a device through the wifi network. The configuration process is managed 
internally and passed back to the main program when the device is configured and 
connected to access point. It happens on every boot.

The process of configuration is divided into three stages:

1. Detect - create access point and wait for configuration.
2. Connect - connect to access point configured in stage 1.
3. Operational - callback to user program and monitor access point connection.

The current device stage and configuration is kept on the flash memory and 
recalled on every boot. The library also implements recovery logic for cases:

- Access point cannot be joined - go to detect stage.
- Access point connection / reconnection. After 10 failed retries switch to 
  detect stage.
- Configuration read failure - reset config and go to detect stage.

Basically the idea is to delegate ESP detection and configuration to this 
library on every boot and wait in user program for a callback when device is 
configured and connected to WiFi. 

If at any point in time the network connection breaks user program is 
notified by a callback and library goes through the stages till it 
recovers the configuration and connection at which point it calls back the 
user program again.

## Stages details.

#### Detect.

Library creates password protected access point with name `PREXIX_XXXXXXXXXXXX` 
and starts TCP server listening on port 7802 for commands (see below). 
In this stage device waits to be detected and configured by an external program 
like [HHQ](https://github.com/rzajac/hhq) which continuously scans for WiFi 
access points in range for a specific pattern, connects to them and sends 
the necessary configuration commands.     

The `PREFIX` in access point name is configurable and `XXXXXXXXXXXX` is always 
the MAC address of the device.

#### Connect.

Connect to access point configured in detect stage. If the connection fails 
library will reset the config and go back to detect stage.
 
#### Operational.

All necessary information is provided and the control is passed to user program.
 
## Command server.

The TCP command server started in detect stage expects configuration command 
in JSON format to be send by a client. The configuration command must be 
in the following format:

```json
{
  "cmd":"cfg",
  "ap_name":"IoTAccessPoint",
  "ap_pass":"secret",
  "mqtt_ip":"192.168.11.127",
  "mqtt_port":1883,
  "mqtt_user":"iot_user",
  "mqtt_pass":"top secret"
}
```

In the response client will receive either success message:

```json
{"success":true, "ic":"ESP2866", "memory":1048576}
```

or error message:

```json
{"success":false, "code":14, "msg": "missing mqtt_port key"}
```

The error codes can be found in [esp_det.h](src/include/esp_det.h)

## Usage.

See [example](example/main.c) program for usage.

## Build environment.

This library is part of my build system for ESP8266 based on CMake.
To compile / flash examples you will have to have the ESP development 
environment setup as described at https://github.com/rzajac/esp-dev-env.

## Flash example.

```
$ cd build
$ cmake ..
$ make esp_det_ex_flash
$ miniterm.py /dev/ttyUSB0 74880
```

## Integration.

If you're using my build environment you can install this library by issuing:

```
$ wget -O - https://raw.githubusercontent.com/rzajac/esp-det/master/install.sh | bash
```

or if you already cloned this repository you can do:

```
$ cd build
$ cmake ..
$ make
$ make install
```

which will install the library, headers and scripts in appropriate places 
in `$ESPROOT`.

# Dependencies.

This library depends on:

- https://github.com/rzajac/esp-ecl
- https://github.com/rzajac/esp-cmd

to install dependencies run:

```
$ wget -O - https://raw.githubusercontent.com/rzajac/esp-ecl/master/install.sh | bash
$ wget -O - https://raw.githubusercontent.com/rzajac/esp-cmd/master/install.sh | bash
```

## License.

[Apache License Version 2.0](LICENSE) unless stated otherwise.
