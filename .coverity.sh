#!/usr/bin/env bash

set -e

if ! [[ ${COVERITY_TOKEN} ]]; then
    echo 'COVERITY_TOKEN not set, exit'
    exit 0
fi


rm -rf libmill
curl -s -L https://github.com/sustrik/libmill/archive/master.tar.gz | tar -zxf -
mv libmill-master libmill
cd libmill
./autogen.sh
./configure
make
rm $(ls .libs/* | grep -v "\.a$")
cd ../

if [ -f Makefile ]; then
    make distclean
fi
autoreconf -ifv
export CPPFLAGS=-I$(pwd)/libmill
export LDFLAGS=-L$(pwd)/libmill/.libs
./configure --enable-debug

rm -rf cov-int
export PATH="$PATH:/opt/cov-analysis-linux64-7.7.0.4/bin/"
cov-build --dir cov-int make
tar czvf muon.tgz cov-int

curl --form token=${COVERITY_TOKEN} \
    --form email=i@pxx.io \
    --form file=@muon.tgz \
    --form version="0.4.0" \
    --form description="$(date)" \
    'https://scan.coverity.com/builds?project=XiaoxiaoPu%2Fmuon'
