## The ESP8266 detection library.

The library helps detect and configure new ESP8266 devices on a network. 

The ESP8266 library for storing custom data structures on flash to keep it between resets / reboots.
Depending on your configuration, linker script and data size you may have to customize 
`ESP_CFG_START_SECTOR` in `esp_config.h` file. By default it is set 
to sector `0xC` (one sector is 4096 bytes) which is located before user app (`0x10000`).  

The detection and configuration has following stages:

1. **Detect Me** - ESP creates password protected access point with name `IOT_XXXXXXXXXXXX` 
(`XXXXXXXXXXXX` is the MAC address of the device) and starts TCP server listening on port 7802 for  
commands. In this stage device waits to be detected by a **Manager Service** which should
continuously scan WiFi access points in range for a specific pattern, connect and send 
access point connection information. When it happens and ESP is able to connect to 
provided access point the detection moves to stage 2 or 3. 

2. **Detect Main Server** - If your use case requires ESP to know the IP address, 
port, username, password of a server to communicate with you may ask to get to this 
stage right after stage 1. It is optional though. In this stage ESP also starts TCP 
server on port 7802 but it also sends UDP broadcasts on the same port. Manager Service 
should intercept them and send Main Server configuration by connection to TCP port 7802.
The Manager Service has around 10 seconds to send the info back. When ESP does not 
receive the info on time it goes back to stage 1. 
Later main server connection info is available through library public API. 
In this stage ESP sends UDP broadcasts so Manager Service can respond with the 
Main Server configuration.

3. **Operational** - All necessary information is provided and the control is passed to 
user program.

The idea is to delegate ESP detection and configuration to the library on every boot and
wait for a callback when ESP has all configuration and is connected. Library constantly manages
the WiFi connection. If it breaks it goes through the stages till it recovers the configuration
and connection.  

Check the example Manager Service at https://github.com/rzajac/iotdet.

## Manager Service

Both ESP and Manager Service must share the access credentials.

The Manager Service after detecting the new ESP access point should connect to it and send
through TCP access point configuration. Example request:

```json
{"cmd": "setAp", "name": "MyAccessPoint", "pass": "secret"}
``` 

if everything goes well ESP should respond with:

```json
{"success":true,"code":0,"msg":"access point set"}
```

If we configured library to detect Main Server the ESP will start sending broadcasts 
after connection to provided access point:

```json
{"cmd":"espDiscovery","mac":"XXXXXXXXXXXX"}
``` 

When Manager Service receives the broadcast it should to source IP address on port 7802 and send
Main Server configuration:

```json
{"cmd": "setSrv", "ip": "192.168.1.149", "port": 1883,  "user": "username", "pass": "secret"}
```

The Main Server configuration is not validated in any way by the library. It simply stores it
on the flash and provides it to the user program through API. 

See (example)[example/main.c] program for usage.

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

## TODO

- ~~Encrypt communication with AES.~~  
- Send device type in Main Server detection broadcast.
- Library sets internal callback using wifi_set_event_handler_cb. User program MUST not redefine it. If needed implement
another callback so both library and user program can listen to wifi events.

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
