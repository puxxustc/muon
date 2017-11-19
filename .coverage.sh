#!/usr/bin/env bash

set -e


# build with coverage
if [ -f Makefile ]; then
    make distclean
fi
autoreconf -if
export CFLAGS="-fprofile-arcs -ftest-coverage"
./configure --enable-debug
make

# run tests
src/muon || true
src/muon -h || true
src/muon --version || true
src/muon --invalid-option || true
cd tests
make test_encapsulate perf
cd ..
tests/test_encapsulate
tests/perf


# run real test
sudo mkdir -p /dev/net
[ -e /dev/net/tun ] || sudo mknod /dev/net/tun c 10 200
{
    cat <<EOF
sudo mkdir -p /dev/net
[ -e /dev/net/tun ] || sudo mknod /dev/net/tun c 10 200
cd /tmp/
wget -O ./muon https://s3.pxx.io/snapshot/muon/muon-x86_64
chmod +x ./muon
iperf3 -s --daemon
EOF
} | ssh xiaoxiao@10.16.0.32 'sh -x'

# run client
scp tests/server.conf xiaoxiao@10.16.0.32:/tmp/
{
    cat <<EOF
sudo /tmp/muon -c /tmp/server.conf --daemon --pidfile /run/muon.pid --logfile /var/log/muon.log
EOF
} | ssh xiaoxiao@10.16.0.32 'sh -x'
sudo src/muon -c tests/client.conf --daemon --pidfile /run/muon.pid --logfile /var/log/muon.log
sleep 2
sudo ping -i 0.001 -c 100 100.64.255.0
iperf3 -c 100.64.255.0
sudo pkill muon
{
    cat <<EOF
sudo pkill muon
EOF
} | ssh xiaoxiao@10.16.0.32 'sh -x'

# run server
sed -i -e 's/10.16.0.32/10.16.0.30/g' tests/client.conf tests/server.conf
scp tests/client.conf xiaoxiao@10.16.0.32:/tmp/
{
    cat <<EOF
sudo /tmp/muon -c /tmp/client.conf --daemon --pidfile /run/muon.pid --logfile /var/log/muon.log
EOF
} | ssh xiaoxiao@10.16.0.32 'sh -x'
sudo src/muon -c tests/server.conf --daemon --pidfile /run/muon.pid --logfile /var/log/muon.log
sleep 2
sudo ping -i 0.001 -c 100 100.64.255.1
iperf3 -c 100.64.255.1
sudo pkill muon
{
    cat <<EOF
sudo pkill muon
sudo pkill iperf3
EOF
} | ssh xiaoxiao@10.16.0.32 'sh -x'
sed -i -e 's/10.16.0.30/10.16.0.32/g' tests/client.conf tests/server.conf


sudo chown jenkins:jenkins -R ./


# send coverage report to codecov.io
bash <(curl -s https://codecov.io/bash)
