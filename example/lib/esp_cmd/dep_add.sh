#!/usr/bin/env bash

echo "Adding library repositories."

git remote add -f esp-pin git@github.com:rzajac/esp-pin.git

git subtree add --prefix example/lib/esp_pin esp-pin master --squash

echo "Done."
