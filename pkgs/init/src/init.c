
#include <signal.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define STATIC_FLAGS MS_NODEV | MS_RDONLY | MS_NOSUID | MS_NOEXEC

#define INOTIFY_BUFFER_SIZE sizeof(struct inotify_event) + NAME_MAX + 1

#define DEFAULT_BOOTPART "/dev/mmcblk1p1"
#define DEFAULT_DATAPART "/dev/mmcblk1p2"

#define init_mount(type, target, flags) \
  mount(type, target, type, flags | MS_NOSUID | MS_NOEXEC, 0)

#define tmp_mount(target) \
  init_mount("tmpfs", target, MS_NODEV)

#define bind_mount(source, target, flags) \
  mount(source, target, 0, flags | MS_BIND, 0)

#define CRIO_SOCK "/var/run/crio/crio.sock"
#define CRIO_LOG "/var/log/crio.log"
#define KUBELET_LOG "/var/log/kubelet.log"
#define KUBELET_CONFIG "/var/lib/kubelet/config.yaml"
#define INIT_MANIFEST "/var/etc/kubernetes/manifests/init.yaml"

static void mount_fs();
static void bring_if_up(const char *);
void wait_for_path(const char* file);

static int fexists(const char *path) {
  struct stat _;
  return !stat(path, &_);
}

static pid_t start_exe(const char *exe, const char *log, char * const *argv) {
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

static void set_hostname_from_file(void) {
  char hostname[HOST_NAME_MAX];
  FILE *fp = fopen("/etc/hostname", "r");
  if (fp) {
    if (fgets(&hostname, HOST_NAME_MAX, fp)) {
      sethostname(hostname, strlen(hostname));
    }
    fclose(fp);
  }
}

static void start_container_runtime(void) {
  // wait_for_path only works when the directory the expected file will
  // be in already exists (it does not recursively check up). Ensure
  // that the directory exists here.
  mkdir("/var/run/crio", 0600);
  char const *nullArgs[] = {"/bin/crio", NULL};
  pid_t crio = start_exe("/bin/crio", CRIO_LOG, nullArgs);
  wait_for_path(CRIO_SOCK);

  if (fexists(KUBELET_CONFIG)) {
    unlink(KUBELET_CONFIG);
  }

  char const *initArgs[] = {
    "/bin/kubelet",
    "--container-runtime-endpoint", "unix:///var/run/crio/crio.sock",
    "--pod-manifest-path=/etc/kubernetes/manifests",
    NULL
  };
  pid_t initkubelet = start_exe("/bin/kubelet", KUBELET_LOG, initArgs);

  wait_for_path(KUBELET_CONFIG);
  kill(initkubelet, SIGTERM);

  // Kubelet normally shuts down pretty quickly, but on the off change
  // we are waiting, we'll do our required admin first before blocking
  // on waiting for it.
  set_hostname_from_file();

  waitpid(initkubelet, NULL, 0);

  char * const kubeletArgs[] = {
    "/bin/kubelet",
    "--config", KUBELET_CONFIG,
    "--kubeconfig", "/etc/kubernetes/kubelet.conf",
    "--bootstrap-kubeconfig", "/etc/kubernetes/bootstrap-kubelet.conf",
    "--container-runtime-endpoint", "unix:///var/run/crio/crio.sock",
    NULL
  };
  pid_t kubelet = start_exe("/bin/kubelet", KUBELET_LOG, kubeletArgs);

  while (1) {
    wait();
  }
}

int main(int argc, char **argv) {
  char *console;
  console = getenv("CONSOLE");
  if (!console) {
    console = getenv("console");
  }

  if (console) {
    int fd = open(console, O_RDWR | O_NONBLOCK | O_NOCTTY);
    if (fd >= 0) {
      dup2(fd, STDIN_FILENO);
      dup2(fd, STDOUT_FILENO);
      dup2(fd, STDERR_FILENO);
      close(fd);
    }
  }

  printf("Kios Init\n");

  bring_if_up("lo");
  bring_if_up("eth0");

  mount_fs();

  char *bootpart = DEFAULT_BOOTPART;
  char *datapart = DEFAULT_DATAPART;
  wait_for_path(bootpart);
  mkdir("/tmp/bootpart", 0500);
  mount(bootpart, "/tmp/bootpart", "vfat", STATIC_FLAGS, 0);
  bind_mount("/tmp/bootpart/boot/modules", "/lib/modules", STATIC_FLAGS);

  wait_for_path(datapart);
  mount(datapart, "/var/lib", "ext2", 0, 0);
  bind_mount("/var/lib/log", "/var/log", MS_NODEV | MS_NOEXEC | MS_NOSUID);

  putenv("PATH=/bin");
  start_container_runtime();
}

static void bring_if_up(const char *iff) {
  int ip = socket(PF_INET, SOCK_DGRAM, 0);
  struct ifreq ifr;
  strncpy(&ifr.ifr_name, iff, 16);
  ioctl(ip, SIOCGIFFLAGS, &ifr);
  ifr.ifr_flags |= IFF_UP;
  ioctl(ip, SIOCSIFFLAGS, &ifr);
}

void wait_for_path(const char *path) {
  // If the file exists, we are good to go
  if (fexists(path)) {
    return;
  }

  char file[NAME_MAX];
  char dir[PATH_MAX - NAME_MAX];
  int lastSlash = 0;
  int k = 0;

  for (int i = 0; path[i] != 0; i++) {
    if (path[i] == '/') {
      k = 0;
      file[0] = 0;
      lastSlash = i;
    } else {
      file[k++] = path[i];
    }
  }
  file[k] = 0; // Null terminate

  for (int i = 0; i < lastSlash; i++) {
    dir[i] = path[i];
  }
  dir[lastSlash] = 0; // Null terminate

  int fd = inotify_init();
  char buffer[INOTIFY_BUFFER_SIZE];
  int watcher = inotify_add_watch(fd, dir, IN_CREATE);

  // We'll check again just incase the drive appeared while we set up
  // inotify
  if (fexists(path)) {
    goto end;
  }

  for (;;) {
    int length, i = 0;
    length = read(fd, &buffer, INOTIFY_BUFFER_SIZE);
    do {
      struct inotify_event *event = (struct inotify_event*)&buffer[i];
      if (strcmp(event->name, file) == 0) {
        goto end;
      }
      i += sizeof(struct inotify_event) + event->len;
    } while (i < length);
  }

end:
  inotify_rm_watch(fd, watcher);
  close(fd);
}

static void mount_fs() {
  // Base system mounts
  init_mount("proc", "/proc", MS_NODEV);
  init_mount("sysfs", "/sys", MS_NODEV);
  init_mount("devtmpfs", "/dev", 0);
  mkdir("/dev/pts", 0755);
  init_mount("devpts", "/dev/pts", 0);
  mkdir("/sys/kernel/security", 0755);
  init_mount("securityfs", "/sys/kernel/security", 0);

  // Sensible tmpfs mounts
  tmp_mount("/run");
  tmp_mount("/tmp");

  // This directory is used by the kubelet to watch for shutdown events
  mkdir("/run/system", 0777);

  tmp_mount("/var/cache");
  tmp_mount("/var/tmp");
  tmp_mount("/var/run");
  mount("tmpfs", "/var/run", "tmpfs", MS_SHARED, 0);

  // Mount the cgroup file systems
  // When k8s supports cgroupv2 we can drop almost all of this
  mkdir("/sys/fs/cgroup", 0755);
  tmp_mount("/sys/fs/cgroup");
  char line[50];
  FILE *fp = fopen("/proc/cgroups", "r");
  fgets(line, sizeof(line), fp);

  while (fgets(line, sizeof(line), fp) != NULL) {
    char cgroup[20];
    char path[50] = "/sys/fs/cgroup/";
    int i = 0;
    for (; i < sizeof(line); i++) {
      if (line[i] != '\t') {
        cgroup[i] = line[i];
        path[i + 15] = line[i];
      } else {
        cgroup[i] = 0;
        path[i + 15] = 0;
        break;
      }
    }

    i++;
    while (line[i++] != '\t'); // Skip over "heirarchy" col
    while (line[i++] != '\t'); // Skip over "num_cgroups" col

    // At the "enabled" col
    // If the cgroup is enabled, we can mount it
    if (line[i] == '1') {
      mkdir(path, 0555);
      mount("cgroup", path, "cgroup", 0, cgroup);
    }
  }
  fclose(fp);
}
