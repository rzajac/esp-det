#!/usr/bin/env bash

echo "Adding library repositories."

git remote add -f esp-tim git@github.com:rzajac/esp-tim.git
git remote add -f esp-eb git@github.com:rzajac/esp-eb.git
git remote add -f esp-json git@github.com:rzajac/esp-json.git
git remote add -f esp-cfg git@github.com:rzajac/esp-cfg.git
git remote add -f esp-cmd git@github.com:rzajac/esp-cmd.git
git remote add -f esp-aes git@github.com:rzajac/esp-aes.git

git subtree add --prefix example/lib/esp_tim esp-tim master --squash
git subtree add --prefix example/lib/esp_eb esp-eb master --squash
git subtree add --prefix example/lib/esp_json esp-json master --squash
git subtree add --prefix example/lib/esp_cfg esp-cfg master --squash
git subtree add --prefix example/lib/esp_cmd esp-cmd master --squash
git subtree add --prefix example/lib/esp_aes esp-aes master --squash

echo "Done."
