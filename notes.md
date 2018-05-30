## Useful commands

```
make clean && make DEBUG_ON=1 && make flash && miniterm.py /dev/ttyUSB0 74880

echo -n '{"cmd": "setAp", "name": "AccessPoint", "pass": "password"}' | nc 192.168.42.1 7802
echo -n '{"cmd": "setSrv", "ip": "192.168.1.149", "port": 1883,  "user": "username", "pass": "password"}' | nc 192.168.42.1 7802

nc -u -l -k 7802

sudo wpa_supplicant -D nl80211 -i wlx000f55a93e30 -c /tmp/iot_wpa_supplicant.conf
sudo dhclient -d wlx000f55a93e30
```

```text
ctrl_interface=/var/run/wpa_supplicant
network={
     ssid="ESP_5CCF7F80CE79"
     psk="password"
}
```

http://msr-nodemcu.readthedocs.io/en/dev-rtd-test/en/flash/

0xfc000 - 1MB
0x3fc000 - 4MB

```
~/esproot/esptool/esptool.py -p /dev/ttyUSB0 -b 74880 write_flash 0x3fc000 esp_init_data_default.bin
~/esproot/esptool/esptool.py -p /dev/ttyUSB0 erase_region 0xC000 0x1000
```
