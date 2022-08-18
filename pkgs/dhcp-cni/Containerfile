FROM docker.io/emilyls/musl:1.1.24

COPY /bin/busybox /bin/
COPY /entrypoint.sh /
COPY /default.conflist /etc/cni/net.d/
COPY /bin/dhcp /bin/portmap /bin/bridge /opt/cni/bin

ENTRYPOINT ["/bin/busybox", "sh", "/entrypoint.sh"]

