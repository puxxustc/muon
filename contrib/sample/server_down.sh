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

# turn off IP forwarding
#sysctl -w net.ipv4.ip_forward=0 >/dev/null

# turn off NAT
iptables -t nat -D POSTROUTING -s $net -j MASQUERADE
iptables -D FORWARD -s $net -j ACCEPT
iptables -D FORWARD -d $net -j ACCEPT

# turn off MSS fix
iptables -t mangle -D FORWARD -p tcp -m tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu
