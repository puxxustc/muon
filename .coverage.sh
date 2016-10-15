#!/usr/bin/env bash

set -e


# build libmill
rm -rf libmill
curl -s -L https://github.com/sustrik/libmill/archive/master.tar.gz | tar -zxf -
mv libmill-master libmill
cd libmill
./autogen.sh
./configure --enable-shared=false
make libmill.la
cd ../

# build with coverage
if [ -f Makefile ]; then
    make distclean
fi
autoreconf -if
export CPPFLAGS
CPPFLAGS=-I$(pwd)/libmill
export LDFLAGS
LDFLAGS=-L$(pwd)/libmill/.libs
export CFLAGS="-fprofile-arcs -ftest-coverage"
./configure --enable-debug
make

# run tests
src/muon || true
src/muon -h || true
src/muon --version || true
src/muon --invalid-option || true
cd tests
make test_rc4 test_md5 test_hmac_md5 test_encapsulate perf
cd ..
tests/test_rc4
tests/test_md5
tests/test_hmac_md5
tests/test_encapsulate
tests/perf

# send coverage report to codecov.io
bash <(curl -s https://codecov.io/bash)

# cleanup
make distclean
