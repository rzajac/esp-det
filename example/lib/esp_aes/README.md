## The ESP8266 library implementing AES CBC encoding and decoding algorithms.

This repository is adoption of https://github.com/kokke/tiny-AES-c for ESP8266. Requires SDK 2.1.0.

## Build and flash the example.

By default Makefile:
- looks for esp-open-sdk in `$HOME/lib/esp-open-sdk/sdk`
- uses `/dev/ttyUSB0` for USB to serial communication
- assumes `esptool.py` exists in user `$PATH`

```
$ make
$ make flash
$ miniterm.py /dev/ttyUSB0 74880
```
## Usage.

See example [main.c](example/main.c).

## Key generation.

Use [gen_aes.sh](gen_aes.sh) to generate keys using `openssl rand`.

## Integration.

The best way to integrate this library is to use git subtree.

To add source to your project use:

```text
$ git remote add -f esp-aes git@github.com:rzajac/esp-aes.git
$ git subtree add --prefix lib/esp_aes esp-aes master --squash
```

To pull updates.

```text
$ git subtree pull --prefix lib/esp_aes esp-aes master --squash
```

## License.

[Apache License Version 2.0](LICENSE) unless stated otherwise.