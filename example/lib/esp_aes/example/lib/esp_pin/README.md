## ESP8266 pin library.

Small library for manipulating GPIO pins on ESP8266. 

- Set any pin as GPIO.
- Use RX pin as GPIO3.
- Operates on register values witch makes all GPIO operations faster.

The code to use RX as GPIO3 is modified to my needs Jeroen Domburg `stdout.c`.

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

Example source [main.c](example/main.c).

## Integration.

The best way to integrate this library is to use git subtree.

To add source to your project use:

```text
$ git remote add -f esp-pin git@github.com:rzajac/esp-pin.git
$ git subtree add --prefix lib/esp_pin esp-pin master --squash
```

To pull updates.

```text
$ git subtree pull --prefix lib/esp_pin esp-pin master --squash
```

## TODO

- [ ] Support for GPIO16
- [ ] Interrupts

## License.

[Apache License Version 2.0](LICENSE) unless stated otherwise.