
#include "include/exe.h"
#include "include/fs.h"
#include "include/kmsg.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int should_restart_processes = 1;

pid_t crio_pid;
pid_t kubelet_pid;

/**
 * Set Hostname From File
 *
 * Checks to see if /etc/hostname exists and contains content - if it
 * does the hostname is updated.
 */
static void set_hostname_from_file(void) {
  char hostname[HOST_NAME_MAX];
  FILE *fp = fopen("/etc/hostname", "r");
  if (fp) {
    if (fgets(hostname, HOST_NAME_MAX, fp)) {
      sethostname(hostname, strlen(hostname));
    }
    fclose(fp);
  }
}

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

  if (!fexists(log)) {
    mkfifo(log, 0700);
  }

  /* Child */
  int fd = open(log, O_RDWR | O_NONBLOCK);
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
  char *crioArgs[] = {
    "/bin/crio",
    "--runtimes", "crun:/bin/crun:/var/run/crun:oci",
    "--cgroup-manager", "cgroupfs",
    "--default-runtime", "crun",
    "--listen", "/var/run/crio/crio.sock",
    "--conmon", "/bin/conmon",
    "--conmon-cgroup", "pod",
    "--pinns-path", "/bin/pinns",
    NULL
  };

  crio_pid = start_exe("/bin/crio", CRIO_LOG, crioArgs);
}

void stop_kubelet(void) {
  kill(kubelet_pid, SIGTERM);
}

void stop_container_runtime(void) {
  kill(crio_pid, SIGTERM);
}

/**
 * Helper function which checks if the given character is an acceptable
 * character in a node label
 */
static int is_label_char(char chr) {
  return ('A' <= chr && chr <= 'Z')
    ||   ('a' <= chr && chr <= 'z')
    ||   ('0' <= chr && chr <= '9')
    ||   (chr == '.')
    ||   (chr == '_')
    ||   (chr == '-')
    ||   (chr == '/');
}

/**
 * The reads the pseudo-yaml "node-labels" configuration file, and
 * converts it into a command line string of node labels to be passed to
 * the kubelet.
 *
 * This function only exists because kubelet _only_ supports receiving
 * its labels via command line flag (not via the KubeletConfiguration
 * file) so we have to have a custom file to specify it in kiOS.
 *
 * The format of this file is:
 *
 * ```
 * # Comment
 * label-name: label-value
 *
 * # Blank lines and white space are ignored
 * another-label-name:         lots-of-whitespace
 *
 * final-label: foobar # Comments at the end of lines are fine too
 * ```
 */
void populate_labels(char *labels) {
  FILE *file = fopen(KUBELET_NODE_LABELS, "r");
  if (!file) return;

  char line[63 + 2 + 63];
  int labels_idx = 0;
  while (fgets(line, sizeof(line), file)) {
    int i = 0;
    int line_has_label = 0;
    do {
      if (line[i] == '#') {
        break;
      } else if (line[i] == ':') {
        labels[labels_idx++] = '=';
      } else if (is_label_char(line[i])) {
        labels[labels_idx++] = line[i];
        line_has_label = 1;
      }
    } while(line[++i]);

    if (line_has_label) {
      labels[labels_idx++] = ',';
    }
  }
  if (labels_idx) {
    labels[labels_idx - 1] = 0;
  }

  fclose(file);
}

void start_kubelet(void) {
  char * kubeletArgs[1 + 2 * KUBELET_MAX_OPTIONS + 1] = {
    "/bin/kubelet",
    "--config", KUBELET_CONFIG,
    "--node-ip", "::"
  };

  // We are keeping the first two args.
  // Others will be overridden from there when calling set_arg.
  int arg = 2;

  char labels[2000];
  populate_labels(labels);
  if (labels[0]) {
    SET_ARG("--node-labels", labels);
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

  set_hostname_from_file();
  kubelet_pid = start_exe("/bin/kubelet", KUBELET_LOG, kubeletArgs);
}

void sig_child(int) {
  pid_t pid = wait(0);

  // Under normal circumstances, critical processes should always be
  // restarted, however, this can be turned off for system shutdown.
  if (should_restart_processes == 0) {
    return;
  } else if (pid == crio_pid) {
    warn("crio has exited! Restarting...\n");
    start_container_runtime();
  } else if (pid == kubelet_pid) {
    warn("kubelet has exited! Restarting\n");
    start_kubelet();
  }
}
