#!/bin/sh

# available environment variables:
#   $mode
#   $server
#   $port
#   $tunif
#   $mtu
#   $address
#   $up
#   $down

addr=${address%/*}
net=${address%.*}.0/${address#*/}

# configure tun interface
ip link set $tunif up
ip link set $tunif mtu $mtu
ip addr add $address dev $tunif

# add route
ip route replace $(ip route get $server | sed -n 1p)
ip route add 0.0.0.0/1   dev $tunif
ip route add 128.0.0.0/1 dev $tunif
