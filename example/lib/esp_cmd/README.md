## Command server for ESP8266.

Fast way to add TCP command server to your ESP8266.

The user API is just four functions:

- `esp_cmd_start` - to start the server,
- `esp_cmd_reg_cb` - to register your command handler,
- `esp_cmd_stop` and `esp_cmd_schedule_stop` - to stop stop the server,
- `esp_cmd_block` block further connections

The `esp_cmd_schedule_stop` should be used if you want to stop the server from 
`espconn` callback, see the example.

You can turn on debug mode by defining ESP_CMD_DEBUG_ON 1 or changing it in `esp_cmd/include/esp_cmd_debug.h` 

## Build and flash the example.

By default Makefile:
- looks for esp-open-sdk in `$HOME/lib/esp-open-sdk/sdk`
- uses `/dev/ttyUSB0` for USB to serial communication
- assumes `esptool.py` exists in user `$PATH`

Rename `example/include/user_config.example.h` to `example/include/user_config.h` and put your
access point connection details then:

```
$ make DEBUG_ON=1
$ make flash
$ miniterm.py /dev/ttyUSB0 74880
```

If everything goes well you should see messages like:

```
connected with AccessPoint, channel 1
dhcp client start...
wifi event: EVENT_STAMODE_CONNECTED
ip:192.168.1.116,mask:255.255.255.0,gw:192.168.1.1
got IP: 192.168.1.116
msk IP: 255.255.255.0
pm open,type:2 0
``` 

In another console issue:

```
$ echo -n 'test' | nc 192.168.1.116 7802
OK
```

Back in ESP8266 console you should see something like this:

```
CMD DBG: CONN: 192.168.1.149:34146
Received command (1): test
CMD DBG: REC: test (4)
CMD DBG: SENT: 192.168.1.149:34146
CMD DBG: DISC: 192.168.1.149:34146
```

Sending the test command twice will close the server and your test messages will no longer return 
`OK`.

## Integration.

The best way to integrate this library is to use git subtree.

To add source to your project use:

```text
$ git remote add -f esp-cmd git@github.com:rzajac/esp-cmd.git
$ git subtree add --prefix lib/esp_cmd esp-cmd master --squash
```

To pull updates.

```text
$ git subtree pull --prefix lib/esp_cmd esp-cmd master --squash
```

## TODO

Unfortunately stopping the server does not close the port. Esp keeps it open which can be checked with:

```
$ nc -w 2 -v 192.168.1.116 7802 </dev/null
Connection to 192.168.1.116 7802 port [tcp/*] succeeded!
```

## License.

[Apache License Version 2.0](LICENSE) unless stated otherwise.