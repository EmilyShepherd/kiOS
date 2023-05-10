
#include "include/exe.h"
#include "include/fs.h"

#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int should_restart_processes = 1;

pid_t crio_pid;
pid_t kubelet_pid;

/**
 * Start Exe
 *
 * Helper function to fork, update stdout and stderr to the given log
 * file, and then exec the given binary. This is used to startup kubelet
 * and crio, both of which need to log to kubelet.log and crio.log,
 * respectively.
 */
pid_t start_exe(const char *exe, const char *log, char * const *argv) {
  pid_t pid;
  pid = fork();

  if (pid != 0) {
    return pid; /* Parent */
  }

  /* Child */
  int fd = open(log, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  dup2(fd, 1);
  dup2(fd, 2);
  close(fd);

  execv(exe, argv);
}

/**
 * Convenience macro to update arguments to be passed to the kubelet
 * daemon
 */
#define SET_ARG(flag, value) \
  arg++; \
  kubeletArgs[arg * 2 - 1] = flag; \
  kubeletArgs[arg * 2] = value;

/**
 * Start Container Runtime
 *
 * Starts up crio, waits for it to be ready, then starts up kubelet. If
 * the kubelet config file does not exist, kubelet will be started in
 * standalone mode. Whilst running in standalone mode, if the config
 * file is ever created (eg by an init container setting one up),
 * kubelet will be stopped, and restarted in API server mode.
 */
void start_container_runtime(void) {
  // wait_for_path only works when the directory the expected file will
  // be in already exists (it does not recursively check up). Ensure
  // that the directory exists here.
  char *nullArgs[] = {"/bin/crio", NULL};
  crio_pid = start_exe("/bin/crio", CRIO_LOG, nullArgs);
}

void stop_kubelet(void) {
  kill(kubelet_pid, SIGTERM);
  waitpid(kubelet_pid, NULL, 0);
}

void start_kubelet(void) {
  char * kubeletArgs[1 + 2 * KUBELET_MAX_OPTIONS + 1] = {
    "/bin/kubelet",
    "--container-runtime-endpoint", "unix:///var/run/crio/crio.sock"
  };

  // We are keeping the first arg (container-runtime-endpoint). Others
  // will be overridden from there when calling set_arg.
  int arg = 1;

  if (fexists(KUBELET_CONFIG)) {
    SET_ARG("--config", KUBELET_CONFIG);
  } else {
    SET_ARG("--pod-manifest-path", "/etc/kubernetes/manifests");
  }

  if (fexists(KUBELET_KUBECONFIG)) {
    SET_ARG("--kubeconfig", KUBELET_KUBECONFIG);
  }

  // If there is a bootstrap kubeconfig file, lets tell kubelet about it
  // also.
  if (fexists(KUBELET_BOOTSTRAP_KUBECONFIG)) {
    SET_ARG("--bootstrap-kubeconfig", KUBELET_BOOTSTRAP_KUBECONFIG);
  }

  // If a credential provider config file exists, tell kubelet about it
  // and also the path to credential provider binaries.
  if (fexists(KUBELET_IMAGE_CREDENTIAL_PROVIDER_CONFIG)) {
    SET_ARG("--image-credential-provider-config", KUBELET_IMAGE_CREDENTIAL_PROVIDER_CONFIG);
    SET_ARG("--image-credential-provider-bin-dir", KUBELET_CREDENTIAL_PROVIDER_BIN_DIR);
  }

  kubelet_pid = start_exe("/bin/kubelet", KUBELET_LOG, kubeletArgs);
}

void run_wait_loop() {
  while (1) {
    pid_t pid = wait(0);

    // Under normal circumstances, critical processes should always be
    // restarted, however, this can be turned off for system shutdown.
    if (should_restart_processes == 0) {
      continue;
    } else if (pid == crio_pid) {
      printf("WARNING: crio has exited! Restarting...\n");
      start_container_runtime();
    } else if (pid == kubelet_pid) {
      printf("WARNING: kubelet has exited! Restarting\n");
      start_kubelet();
    }
  }
}
