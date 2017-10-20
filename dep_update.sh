#!/usr/bin/env bash

echo "Updating libraries."

git subtree pull --prefix example/lib/esp_tim esp-tim master --squash
git subtree pull --prefix example/lib/esp_eb esp-eb master --squash
git subtree pull --prefix example/lib/esp_json esp-json master --squash
git subtree pull --prefix example/lib/esp_cfg esp-cfg master --squash
git subtree pull --prefix example/lib/esp_cmd esp-cmd master --squash
git subtree pull --prefix example/lib/esp_aes esp-aes master --squash

echo "Done."
