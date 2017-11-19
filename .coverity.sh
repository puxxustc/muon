#!/usr/bin/env bash

set -e


if [ -f Makefile ]; then
    make distclean
fi
autoreconf -if
./configure --enable-debug

rm -rf cov-int
export PATH="$PATH:/opt/cov-analysis-linux64-7.7.0.4/bin/"
cov-build --dir cov-int make
tar czvf muon.tgz cov-int

if [[ ${COVERITY_TOKEN} ]]; then
    curl --form token=${COVERITY_TOKEN} \
        --form email=i@pxx.io \
        --form file=@muon.tgz \
        --form version="0.4.0" \
        --form description="muon" \
        'https://scan.coverity.com/builds?project=puxxustc%2Fmuon'
else
    echo 'COVERITY_TOKEN not set, do not submit build'
fi

make distclean
