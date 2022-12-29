#/bin/busybox sh
set -e
BB=/bin/busybox

if test "$1" == "init"
then
  for plugin in bridge dhcp portmap
  do
    $BB cp /opt/cni/bin/$plugin /host/opt/cni/bin/
  done

  $BB cp /etc/cni/net.d/default.conflist /host/etc/cni/net.d/

  if test -n "$BRIDGE_WITH"
  then
    test -e /sys/class/net/$BRIDGE || $BB ip link add $BRIDGE type bridge

    $BB ip link set dev $BRIDGE up
    $BB ip link set dev $BRIDGE_WITH master $BRIDGE
  fi
else
  $BB rm -f /host/run/cni/dhcp.sock
  exec /opt/cni/bin/dhcp daemon -hostprefix /host
fi
