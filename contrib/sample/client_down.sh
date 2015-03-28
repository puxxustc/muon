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

# delete route
if [[ $route == "yes" ]]; then
	if type python3 >/dev/null 2>&1; then
		{
			python3 <<EOF
from ipaddress import ip_network

net = ip_network('0.0.0.0/0')
server = ip_network('$server')
net = net.address_exclude(server)

for subnet in net:
    print('route del %s dev $tunif' % subnet)
EOF
		} | ip -b -
	else
		ip route del $server
		ip route del 0.0.0.0/1 dev $tunif
		ip route del 128.0.0.0/1 dev $tunif
	fi
fi
