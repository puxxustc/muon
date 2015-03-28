# sipvpn #

[![Release](https://img.shields.io/github/release/XiaoxiaoPu/sipvpn.svg?style=flat)](https://github.com/XiaoxiaoPu/sipvpn/releases/latest)
[![License](https://img.shields.io/badge/license-GPL%203-blue.svg?style=flat)](http://www.gnu.org/licenses/gpl.html)
[![Build Status](https://travis-ci.org/XiaoxiaoPu/sipvpn.svg?branch=master)](https://travis-ci.org/XiaoxiaoPu/sipvpn)
[![Build Status](https://jenkins.xiaoxiao.im/buildStatus/icon?job=sipvpn)](https://jenkins.xiaoxiao.im/job/sipvpn)

**Si**m**p**le stateless **VPN**.

## Build ##

1. configure and make

	```bash
	autoreconf -if
	./configure --prefix=/usr --sysconfdir=/etc
	make
	```

2. install

	```bash
	sudo make install
	sudo chmod +x /etc/sipvpn/hooks/*
	```

## Cross compile ##

1. setup cross compile tool chain

2. build

	```bash
	autoreconf -if
	./configure --host=arm-unknown-linux-gnueabihf \
	    --prefix=/usr --sysconfdir=/etc
	make
	```

## Usage ##

See man:sipvpn(8).

## License ##

Copyright (C) 2014 - 2015, Xiaoxiao <i@xiaoxiao.im>

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
