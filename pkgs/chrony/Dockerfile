FROM --platform=$BUILDPLATFORM k8s.gcr.io/build-image/setcap:bullseye-v1.0.0

ADD /sbin/chronyd /bin/chronyd
RUN setcap cap_sys_time=+ep /bin/chronyd

FROM docker.io/emilyls/musl:1.1.24

COPY --from=0 /bin/chronyd /bin/
COPY /etc/chrony.conf /etc/chrony.conf
COPY /etc/passwd /etc/group /etc/

USER 999:999

ENTRYPOINT ["/bin/chronyd", "-d"]

