# muon #

[![License](https://a.pxx.io/badge/badge/license-GPL%20v3.0-blue.svg)](https://www.gnu.org/licenses/gpl.html)
[![Build Status](https://ci.pxx.io/buildStatus/icon?job=muon)](https://ci.pxx.io/job/muon)

A fast stateless VPN with simple obfuscation, inspired by [ShadowVPN](https://github.com/clowwindy/ShadowVPN) and [GoHop](https://github.com/bigeagle/gohop).


## Features ##

1. Stateless
2. Perform naïve obfuscation by compression, padding and delayed transmission
3. Frequent port hopping every 0.5s to escape traffic monitoring


## Supported platforms ##

* GNU/Linux
* macOS

## Dependencies ##

1. [libmill](http://libmill.org/)

2. [minilzo](http://www.oberhumer.com/opensource/lzo/#minilzo) (embedded)


## Pre-builds ##

Platform  | Architecture | URL
----------|--------------|----
GNU/Linux | x86_64       | https://s3.pxx.io/snapshot/muon/muon-x86_64
&nbsp;    | armv6l       | https://s3.pxx.io/snapshot/muon/muon-armv6l
&nbsp;    | armv7l       | https://s3.pxx.io/snapshot/muon/muon-armv7l
&nbsp;    | aarch64      | https://s3.pxx.io/snapshot/muon/muon-aarch64

## Build ##

### 1. Linux/macOS ###

install GNU Autotools, then:

```bash
# build libmill
curl -s -L https://github.com/sustrik/libmill/archive/master.tar.gz | tar -zxf -
mv libmill-master libmill
cd libmill
./autogen.sh
./configure --enable-shared=false
make
cd ../
# build muon
autoreconf -if
# export CFLAGS=-march=native
export CPPFLAGS=-I$(pwd)/libmill
export LDFLAGS=-L$(pwd)/libmill/.libs
./configure --prefix=/usr --sysconfdir=/etc
make
make check
sudo make install
```

on macOS, install via [Homebrew](https://brew.sh/) is also supported:

```bash
brew install --HEAD libmill
brew install --HEAD https://raw.githubusercontent.com/XiaoxiaoPu/muon/master/contrib/homebrew/muon.rb
```


### 2. Cross compile ###

```bash
# setup cross compile tool chain:
export PATH="$PATH:/pato/to/cross/compile/toolchain/bin/"
# build libmill
curl -s -L https://github.com/sustrik/libmill/archive/master.tar.gz | tar -zxf -
mv libmill-master libmill
cd libmill
./autogen.sh
./configure --enable-shared=false --host=arm-unknown-linux-gnueabihf
make
cd ../
# build muon
autoreconf -if
# export CFLAGS=-march=native
export CPPFLAGS=-I$(pwd)/libmill
export LDFLAGS=-L$(pwd)/libmill/.libs
./configure --host=arm-unknown-linux-gnueabihf \
    --prefix=/usr --sysconfdir=/etc
make
```


### 3. Build with static linking ###

append `--enable-static` while running `./configure`.


## Usage ##

See man:muon(8).


## License ##

Copyright (C) 2014 - 2017, Xiaoxiao <i@pxx.io>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
