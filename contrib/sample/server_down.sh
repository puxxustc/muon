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

if [[ $nat == "yes" ]]; then
	# turn off IP forwarding
	#sysctl -w net.ipv4.ip_forward=0 >/dev/null

	# turn off NAT
	iptables -t nat -D POSTROUTING -s $subnet -j MASQUERADE
	iptables -D FORWARD -s $subnet -j ACCEPT
	iptables -D FORWARD -d $subnet -j ACCEPT

	# turn off MSS fix
	iptables -t mangle -D FORWARD -p tcp -m tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu
fi
