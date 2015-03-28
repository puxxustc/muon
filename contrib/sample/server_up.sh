#!/bin/sh

# available environment variables:
#   $server
#   $port
#   $tunif
#   $mtu
#   $address
#   $route

addr=${address%/*}
subnet=${address%.*}.0/${address#*/}

# turn on IP forwarding
sysctl -w net.ipv4.ip_forward=1 >/dev/null

# configure tun interface
ip link set $tunif up
ip link set $tunif mtu $mtu
ip addr add $address dev $tunif

# turn on NAT
iptables -t nat -A POSTROUTING -s $subnet -j MASQUERADE
iptables -A FORWARD -s $subnet -j ACCEPT
iptables -A FORWARD -d $subnet -j ACCEPT

# turn on MSS fix
iptables -t mangle -A FORWARD -p tcp -m tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu
