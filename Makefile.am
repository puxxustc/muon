SUBDIRS = src

EXTRA_DIST = man/sipvpn.8 contrib/systemd/sipvpn@.service \
             contrib/sample/client.conf \
             contrib/sample/client_down.sh \
             contrib/sample/client_up.sh \
             contrib/sample/server.conf \
             contrib/sample/server_down.sh \
             contrib/sample/server_up.sh

man_MANS = man/sipvpn.8

confdir=$(sysconfdir)/sipvpn/examples
conf_DATA=contrib/sample/client.conf \
          contrib/sample/server.conf

hooksdir=$(sysconfdir)/sipvpn/hooks
hooks_DATA=contrib/sample/client_down.sh \
           contrib/sample/client_up.sh \
           contrib/sample/server_down.sh \
           contrib/sample/server_up.sh

systemddir=$(libdir)/systemd/system
systemd_DATA=contrib/systemd/sipvpn@.service
