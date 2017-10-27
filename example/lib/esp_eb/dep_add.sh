#!/usr/bin/env bash

echo "Adding library repositories."

git remote add -f esp-tim git@github.com:rzajac/esp-tim.git
git remote add -f esp-pin git@github.com:rzajac/esp-pin.git

git subtree add --prefix example/lib/esp_tim esp-tim master --squash
git subtree add --prefix example/lib/esp_pin esp-pin master --squash

echo "Done."
