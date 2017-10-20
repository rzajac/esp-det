## Store user data on flash memory.

The ESP8266 library for storing custom data structures on flash to keep it between resets / reboots.
Depending on your configuration, linker script and data size you may have to customize 
`ESP_CFG_START_SECTOR` in `esp_config.h` file. By default it is set 
to sector `0xC` (one sector is 4096 bytes) which is located before user app (`0x10000`).  

By default user can use two configuration structures. You may change it by defining `ESP_CFG_NUMBER`
or editing `esp_config.h` file (making this change requires changing `ESP_CFG_START_SECTOR`).

See (example)[example/main.c] program for usage.

## Build and flash the example.

By default Makefile:
- looks for esp-open-sdk in `$HOME/lib/esp-open-sdk/sdk`
- uses `/dev/ttyUSB0` for USB to serial communication
- assumes `esptool.py` exists in user `$PATH`

```
$ make
$ make flash
```

## Integration.

The best way to integrate this library is to use git subtree.

To add source to your project use:

```text
$ git remote add -f esp-cfg git@github.com:rzajac/esp-cfg.git
$ git subtree add --prefix lib/esp_cfg esp-cfg master --squash
```

To pull updates.

```text
$ git subtree pull --prefix lib/esp_cfg esp-cfg master --squash
```

## License.

[Apache License Version 2.0](LICENSE) unless stated otherwise.