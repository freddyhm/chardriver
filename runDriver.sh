#!/usr/bin/env bash
make -C /lib/modules/$(uname -r)/build M=$PWD modules
sudo rmmod pilote_char.ko
sudo insmod pilote_char.ko