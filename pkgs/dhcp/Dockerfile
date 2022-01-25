FROM docker.io/emilyls/musl:1.1.24

COPY /bin/busybox /bin/
COPY /sbin/dhcpc /sbin/

ENTRYPOINT ["/sbin/dhcpc"]
CMD ["init"]

