#!/bin/bash

# LabJack UE9
# Default IP address: 192.168.1.209
# Port: 52360



### Automatic discovery of UE9 using UDP broadcast

# # DiscoveryUDP, chapter 5.2.3, see
# # https://labjack.com/support/datasheets/ue9/low-level-function-reference/comm-functions/discovery-udp
# # Broadcast 6 byte command to port 52362
# # Expect a 38 byte response
# echo -ne "\x22\x78\x00\xA9\x00\x00" | \
#     socat - udp-datagram:255.255.255.255:52362,broadcast | \
#     xxd -p | cat
# # Example output:
# #                              209  1 168 192
# # 29 78 10 01 94 0b 00 00 01 00 d1 01 a8 c0
# # 01 01 a8 c0 00 ff ff ff 88 cc 89 cc 00 09 c1 06
# # 00872e900a01280129781001940b00000100d101a8c00101a8c000ffffff
# # 88cc89cc0009c10600872e900a012801
# # CommFW Version 1.40
# # HW Version 1.10
# # So the ip addres is 192.168.1.209
#
# echo -ne "\x22\x78\x00\xA9\x00\x00" | \
#     socat - udp-datagram:255.255.255.255:52362,broadcast | \
#     xxd -p | cut -c21-28 | cat
# # Output:
# # d101a8c0

# Discover IP address
# Example usage: discover_ue9_ipaddr > test.txt
function discover_ue9_ipaddr {
    ipaddrhex=$(echo -ne "\x22\x78\x00\xA9\x00\x00" | \
        socat - udp-datagram:255.255.255.255:52362,broadcast | \
        xxd -p | cut -c21-28)        # e.g. d101a8c0
    ip4=$(echo $ipaddrhex|cut -c1-2) # d1
    ip3=$(echo $ipaddrhex|cut -c3-4) # 01
    ip2=$(echo $ipaddrhex|cut -c5-6) # a8
    ip1=$(echo $ipaddrhex|cut -c7-8) # c0
    echo $[0x$ip1].$[0x$ip2].$[0x$ip3].$[0x$ip4]
}


### Read TEST
# TEST starts at address 55100.  If you read a UINT32 (2 registers) from here,
# you should get 0x00112233 (d1122867), or if you read just a UIN16 (1 register)
# from here you should get 0x0011 (d17).
#
# UINT32 read:
# Command:  0x00 0x00 0x00 0x00 0x00 0x06 0x01 0x03 0xD7 0x3C 0x00 0x02
# Response:  0x00 0x00 0x00 0x00 0x00 0x07 0x01 0x03 0x04 0x00 0x11 0x22 0x33
#
# UINT16 read:
# Command:  0x00 0x00 0x00 0x00 0x00 0x06 0x01 0x03 0xD7 0x3C 0x00 0x01
# Response:  0x00 0x00 0x00 0x00 0x00 0x05 0x01 0x03 0x02 0x00 0x11

ipaddr=$(discover_ue9_ipaddr)
echo $ipaddr

# UINT32 read
echo -ne "\x00\x00\x00\x00\x00\x06\x01\x03\xD7\x3C\x00\x02" | \
    nc $ipaddr 52360 | xxd -p | cat

# UINT16 read
echo -ne "\x00\x00\x00\x00\x00\x06\x01\x03\xD7\x3C\x00\x01" | \
    nc $ipaddr 52360 | xxd -p | cat

# UINT16 read
echo -ne "\x00\x00\x00\x00\x00\x06\x01\x03\xD7\x3C\x00\x01" | \
    nc $ipaddr 52360 >blah.txt

# Read FIO
echo -ne "\x00\x00\x00\x00\x00\x06\x01\x03\x07\xD0\x00\x01" | \
    nc $ipaddr 52360 | xxd -p | cat

# Debugging
# Determine which ports are open and running services on a target machine
nc -z 192.168.1.209 1-60000
# Connection to 192.168.1.209 port 52360 [tcp/*] succeeded!
# Connection to 192.168.1.209 port 52361 [tcp/*] succeeded!

# Read register 0, AIN0
echo -ne "\x00\x00\xa6\x3f\x00\x00\x00\x06\x00\x03\x00\x00\x00\x02" | \
    nc $ipaddr 52360 | xxd -p | cat

# Read register 2, AIN1
echo -ne "\x00\x00\xa6\x40\x00\x00\x00\x06\x00\x03\x00\x02\x00\x02" | \
    nc $ipaddr 52360 | xxd -p | cat

# low level command
echo -ne "\x60\xA3\x01\x05\xFF\x00\x00\x00" | \
    nc $ipaddr 52360 | xxd -p

0xB5,0xA3,0x00,0x13,0x00,0x00,0x00,0x00
0x60,0xA3,0x01,0x05,0xFF,0x00,0x00,0x00
# 5.1 General protocol
# https://labjack.com/support/datasheets/ue9/low-level-function-reference/general-protocal
# 5.3 Control functions

Before sending the ConfigTimerClock command, you should send a ConfigIO (Section 5.2.3 in the User's Guide) command first to enable Timer/Counter lines, then a ConfigTimerClock command to configure the clock and last the Feedback command to set the individual Timer settings.


