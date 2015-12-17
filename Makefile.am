ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src

EXTRA_DIST = man/muon.8 contrib/systemd/muon@.service \
             contrib/sample/client.conf \
             contrib/sample/server.conf

man_MANS = man/muon.8

confdir=$(sysconfdir)/muon/examples
conf_DATA=contrib/sample/client.conf \
          contrib/sample/server.conf

systemddir=$(libdir)/systemd/system
systemd_DATA=contrib/systemd/muon@.service
