
#ifndef _EXE_H
#define _EXE_H 1

#define CRIO_SOCK "/var/run/crio/crio.sock"
#define CRIO_LOG "/var/run/crio.log"
#define KUBELET_LOG "/var/run/kubelet.log"
#define KUBELET_CONFIG "/etc/kubernetes/config.yaml"
#define KUBELET_NODE_LABELS "/etc/kubernetes/node-labels"
#define KUBELET_KUBECONFIG "/etc/kubernetes/kubelet.conf"
#define KUBELET_BOOTSTRAP_KUBECONFIG "/etc/kubernetes/bootstrap-kubelet.conf"
#define KUBELET_IMAGE_CREDENTIAL_PROVIDER_CONFIG "/etc/kubernetes/credential-providers.yaml"
#define KUBELET_CREDENTIAL_PROVIDER_BIN_DIR "/usr/libexec/kubernetes/kubelet-plugins/credential-provider/exec"
#define KUBELET_MAX_OPTIONS 6

extern int should_restart_processes;

void start_container_runtime(void);
void stop_container_runtime(void);

void start_kubelet(void);
void stop_kubelet(void);

void sig_child(int);

#endif
