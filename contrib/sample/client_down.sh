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

# delete route
ip route del $(ip route get $server | sed -n 1p)
ip route del 0.0.0.0/1   dev $tunif
ip route del 128.0.0.0/1 dev $tunif
