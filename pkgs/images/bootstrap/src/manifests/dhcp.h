
#ifndef _DHCP_H
#define _DHCP_H 1

const char *DHCP_MANIFEST =
"apiVersion: v1\n"
"kind: Pod\n"
"metadata:\n"
"  name: dhcpd-%s\n"
"  namespace: kube-node-lease\n"
"spec:\n"
"  containers:\n"
"  - image: emilyls/dhcp:v1\n"
"    name: busybox\n"
"    args:\n"
"    - -p\n"
"    - %s\n"
"    volumeMounts:\n"
"    - mountPath: /etc\n"
"      name: config\n"
"    - mountPath: /var/run/dhcpcd\n"
"      name: runtime\n"
"      subPath: run\n"
"    - mountPath: /var/db/dhcpcd\n"
"      name: runtime\n"
"      subPath: db\n"
"    # Currently we require running as root but this may change\n"
"    securityContext:\n"
"      privileged: true\n"
"      capabilities:\n"
"        drop:\n"
"        - ALL\n"
"        add:\n"
"        - NET_ADMIN\n"
"        - NET_RAW\n"
"  hostNetwork: true\n"
"  priorityClassName: system-node-critical\n"
"  tolerations:\n"
"  - operator: Exists\n"
"  volumes:\n"
"  - hostPath:\n"
"      path: /etc\n"
"      type: Directory\n"
"    name: config\n"
"  - emptyDir: {}\n"
"    name: runtime\n"
;

#endif