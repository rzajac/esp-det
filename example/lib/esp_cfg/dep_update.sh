#!/usr/bin/env bash

echo "Updating libraries."

git subtree pull --prefix example/lib/esp_pin esp-pin master --squash

echo "Done."
