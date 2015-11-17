#!/bin/bash
# Discover the LabJack UE9's IP address using UDP broadcast
ipaddrhex=$(echo -ne "\x22\x78\x00\xA9\x00\x00" | \
    socat - udp-datagram:255.255.255.255:52362,broadcast | \
    xxd -p | cut -c21-28)        # e.g. d101a8c0
ip4=$(echo $ipaddrhex|cut -c1-2) # d1
ip3=$(echo $ipaddrhex|cut -c3-4) # 01
ip2=$(echo $ipaddrhex|cut -c5-6) # a8
ip1=$(echo $ipaddrhex|cut -c7-8) # c0
echo $[0x$ip1].$[0x$ip2].$[0x$ip3].$[0x$ip4]

