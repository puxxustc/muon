#!/bin/sh

# available environment variables:
#   $server
#   $port
#   $tunif
#   $mtu
#   $address
#   $route
#   $nat

addr=${address%/*}
subnet=${address%.*}.0/${address#*/}

# configure tun interface
ip link set $tunif up
ip link set $tunif mtu $mtu
ip addr add $address dev $tunif

# add route
if [[ $route == "yes" ]]; then
	if type python3 >/dev/null 2>&1; then
		{
			python3 <<EOF
from ipaddress import ip_network

net = ip_network('0.0.0.0/0')
server = ip_network('$server')
net = net.address_exclude(server)

for subnet in net:
    print('route add %s dev $tunif' % subnet)
EOF
		} | ip -b -
	else
		ip route replace $(ip route get $server | sed -n 1p)
		ip route add 0.0.0.0/1 dev $tunif
		ip route add 128.0.0.0/1 dev $tunif
	fi
fi
