## Small ESP8266 library helping with timers.

See example [main.c](example/main.c).

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
$ git remote add -f esp-tim git@github.com:rzajac/esp-tim.git
$ git subtree add --prefix lib/esp_tim esp-tim master --squash
```

To pull updates.

```text
$ git subtree pull --prefix lib/esp_tim esp-tim master --squash
```

## License.

[Apache License Version 2.0](LICENSE) unless stated otherwise.