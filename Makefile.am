ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src

EXTRA_DIST = man/sipvpn.8 contrib/systemd/sipvpn@.service \
             contrib/sample/client.conf \
             contrib/sample/server.conf

man_MANS = man/sipvpn.8

confdir=$(sysconfdir)/sipvpn/examples
conf_DATA=contrib/sample/client.conf \
          contrib/sample/server.conf

systemddir=$(libdir)/systemd/system
systemd_DATA=contrib/systemd/sipvpn@.service
