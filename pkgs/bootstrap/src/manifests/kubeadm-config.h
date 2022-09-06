#ifndef _KUBEADM_CONFIG_H
#define _KUBEADM_CONFIG_H 1

const char *KUBEADM_CONFIG =
"apiVersion: kubeadm.k8s.io/v1beta3\n"
"kind: InitConfiguration\n"
"nodeRegistration:\n"
"  taints: []\n"
"---\n"
"apiVersion: kubeadm.k8s.io/v1beta3\n"
"kind: ClusterConfiguration\n"
"clusterName: kios\n"
"kubernetesVersion: 1.23.1\n"
"# Example configuration to support static token auth (nice for quick\n"
"# 'n easy cluster control plane init)\n"
"apiServer:\n"
"  extraArgs:\n"
"    token-auth-file: /etc/kubernetes/tokens\n"
"  extraVolumes:\n"
"  - name: token-file\n"
"    hostPath: /etc/kubernetes/tokens\n"
"    mountPath: /etc/kubernetes/tokens\n"
"    readOnly: true\n"
"    pathType: File\n"
"---\n"
"apiVersion: kubelet.config.k8s.io/v1beta1\n"
"kind: KubeletConfiguration\n"
"cgroupDriver: cgroupfs\n"
"shutdownGracePeriod: 30s\n"
"shutdownGracePeriodCriticalPods: 10s\n"
;

const char *TOKEN_CONFIG = "%s,admin,admin,system:masters";

#endif /* kubeadm-config.h */
