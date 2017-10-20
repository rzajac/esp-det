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
```
## Usage.

See example [main.c](example/main.c).

## Key generation.

Use [gen_aes.sh](gen_aes.sh) to generate keys using `openssl rand`.

## License.

[Apache License Version 2.0](LICENSE) unless stated otherwise.