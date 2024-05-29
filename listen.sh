#!/bin/sh
# wlp5s0

ifconfig wlp5s0 down
iwconfig wlp5s0 mode monitor
ifconfig wlp5s0 up
cd /appdata/PlantSupervisor/bin
/appdata/PlantSupervisor/bin/exec wlp5s0
# tcpdump -i wlan0 'type 0 subtype 0xd0 and wlan[24:4]=0x7f18fe34' -dd