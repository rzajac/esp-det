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

## Subtrees

git remote add -f esp-cfg git@github.com:rzajac/esp-cfg.git
git subtree add --prefix example/lib/esp_cfg esp-cfg master --squash
git subtree pull --prefix example/lib/esp_cfg esp-cfg master --squash

git remote add -f esp-cmd git@github.com:rzajac/esp-cmd.git
git subtree add --prefix example/lib/esp_cmd esp-cmd master --squash
git subtree pull --prefix example/lib/esp_cmd esp-cmd master --squash

git remote add -f esp-eb git@github.com:rzajac/esp-eb.git
git subtree add --prefix example/lib/esp_eb esp-eb master --squash
git subtree pull --prefix example/lib/esp_eb esp-eb master --squash

git remote add -f esp-json git@github.com:rzajac/esp-json.git
git subtree add --prefix example/lib/esp_json esp-json master --squash
git subtree pull --prefix example/lib/esp_json esp-json master --squash

git remote add -f esp-aes git@github.com:rzajac/esp-aes.git
git subtree add --prefix example/lib/esp_aes esp-aes master --squash
git subtree pull --prefix example/lib/esp_aes esp-aes master --squash