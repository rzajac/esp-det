#!/usr/bin/env bash

echo "Adding library repositories."

git remote add -f esp-tim git@github.com:rzajac/esp-tim.git

git subtree add --prefix example/lib/esp_tim esp-tim master --squash

echo "Done."
