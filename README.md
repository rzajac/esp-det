## ESP8266 library making JSON manipulation easier.

This repository is adoption of https://github.com/DaveGamble/cJSON for ESP8266. Requires SDK 2.1.0.

## Build and flash the example.

By default Makefile:
- looks for esp-open-sdk in `$HOME/lib/esp-open-sdk/sdk`
- uses `/dev/ttyUSB0` for USB to serial communication
- assumes `esptool.py` exists in user `$PATH`

```
$ make
$ make flash
```

## Usage.

See [example](example/main.c).

## Integration.

The best way to integrate this library is to use git subtree.

To add source to your project use:

```text
$ git remote add -f esp-json git@github.com:rzajac/esp-json.git
$ git subtree add --prefix lib/esp_json esp-json master --squash
```

To pull updates.

```text
$ git subtree pull --prefix lib/esp_json esp-json master --squash
```

## License.

[Apache License Version 2.0](LICENSE) unless stated otherwise.