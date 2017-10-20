#!/usr/bin/env bash

echo "Easy to copy and paste AES128 key and initialization vector generator."
echo

exec 3>&1

KEY=`openssl rand -hex 16`
IV=`openssl rand -hex 16`

c_style_init() {
    exec 4>&1 >&3
    local RES=""
    while read -N2 code; do
        RES="${RES}, 0x${code}"
    done <<< "${1}"
    RES=${RES#", "}

    exec >&4-
    echo "${RES}"
}

KEY_C_INIT=$( c_style_init ${KEY} )
VI_C_INIT=$( c_style_init ${IV} )

echo "C style defines:"
echo
echo "#define AES_KEY_INIT { ${KEY_C_INIT} }"
echo "#define AES_VI_INIT { ${VI_C_INIT} }"
echo
echo "Go style:"
echo
echo "[]byte{ ${KEY_C_INIT} }"
echo "[]byte{ ${VI_C_INIT} }"
echo
echo "INI style:"
echo
echo "key=${KEY}"
echo "vi=${IV}"
echo
