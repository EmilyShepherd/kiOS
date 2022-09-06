
#ifndef _CHRONY_H
#define _CHRONY_H 1

#define CHRONY_FILE "/etc/kubernetes/manifests/chrony.yaml"

const char *CHRONY_MANIFEST =
"apiVersion: v1\n"
"kind: Pod\n"
"metadata:\n"
"  name: chrony\n"
"  namespace: kube-node-lease\n"
"spec:\n"
"  containers:\n"
"  - image: emilyls/chrony:4.2\n"
"    name: chrony\n"
"    securityContext:\n"
"      runAsNonRoot: true\n"
"      readOnlyRootFilesystem: true\n"
"      allowPrivilegeEscalation: false\n"
"      capabilities:\n"
"        drop:\n"
"        - ALL\n"
"        add:\n"
"        - SYS_TIME\n"
"    volumeMounts:\n"
"    - mountPath: /var/run/chrony\n"
"      name: run\n"
"  volumes:\n"
"  - emptyDir: {}\n"
"    name: run\n"
"  hostNetwork: true\n"
"  priorityClassName: system-node-critical\n"
;


#endif
