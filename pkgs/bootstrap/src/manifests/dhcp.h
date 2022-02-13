
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
"    volumeMounts:\n"
"    - mountPath: /etc\n"
"      name: config\n"
"    # Currently we require running as root but this may change\n"
"    securityContext:\n"
"      allowPrivilegeEscalation: false\n"
"      capabilities:\n"
"        drop:\n"
"        - ALL\n"
"        add:\n"
"        - NET_ADMIN\n"
"        - NET_RAW\n"
"    env:\n"
"    - name: INTERFACE\n"
"      value: %s\n"
"  hostNetwork: true\n"
"  priorityClassName: system-node-critical\n"
"  tolerations:\n"
"  - operator: Exists\n"
"  volumes:\n"
"  - hostPath:\n"
"      path: /etc\n"
"      type: Directory\n"
"    name: config"
;

#endif
