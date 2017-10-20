#!/usr/bin/env bash

echo "Updating libraries."

git subtree pull --prefix example/lib/esp_tim esp-tim master --squash

echo "Done."
