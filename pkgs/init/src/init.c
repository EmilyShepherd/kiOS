
#include <net/if.h>
#include <sys/socket.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <linux/limits.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define STATIC_FLAGS MS_NODEV | MS_RDONLY | MS_NOSUID | MS_NOEXEC

#define INOTIFY_BUFFER_SIZE sizeof(struct inotify_event) + NAME_MAX + 1

#define DEFAULT_BOOTPART "/dev/mmcblk1p1"
#define DEFAULT_DATAPART "/dev/mmcblk1p2"

#define mkmount(source, target, type, flags, data) \
  mkdir(target, 0777); \
  mount(source, target, type, flags, data)

#define init_mount(type, target, flags) \
  mkmount(type, target, type, flags | MS_NOSUID | MS_NOEXEC, 0)

#define tmp_mount(target) \
  init_mount("tmpfs", target, MS_NODEV)

#define bind_mount(source, target, flags) \
  mkmount(source, target, 0, flags | MS_BIND, 0)

static void mount_fs();
static void bring_if_up(const char *);
void wait_for_path(const char* file);

static int fexists(const char *path) {
  struct stat _;
  return !stat(path, &_);
}

int main(int argc, char **argv) {
  bring_if_up("lo");
  bring_if_up("eth0");

  mount_fs();

  char *bootpart = DEFAULT_BOOTPART;
  char *datapart = DEFAULT_DATAPART;
  wait_for_path(bootpart);
  mkmount(bootpart, "/static", "vfat", STATIC_FLAGS, 0);
  bind_mount("/static/boot", "/boot", STATIC_FLAGS);
  bind_mount("/static/boot/modules", "/lib/modules", STATIC_FLAGS);

  wait_for_path(datapart);
  mkmount(datapart, "/srv", "ext2", 0, 0);
  bind_mount("/srv", "/var/lib", 0);
  bind_mount("/srv/log", "/var/log", MS_NODEV | MS_NOEXEC | MS_NOSUID);
  bind_mount("/var/lib/kubernetes", "/etc/kubernetes", MS_NODEV | MS_NOEXEC | MS_NOSUID);

  char hostname[256];
  FILE *fp = fopen("/srv/hostname", "r");
  if (fp) {
    if (fgets(&hostname, 256, fp)) {
      sethostname(hostname, strlen(hostname));
    }
  }

  return 0;
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
  init_mount("devpts", "/dev/pts", 0);
  init_mount("securityfs", "/sys/kernel/security", 0);

  // Sensible tmpfs mounts
  tmp_mount("/run");
  tmp_mount("/tmp");

  // This directory is used by the kubelet to watch for shutdown events
  mkdir("/run/system", 0777);

  mkdir("/var", 0777);
  tmp_mount("/var/cache");
  tmp_mount("/var/tmp");
  tmp_mount("/var/run");
  mount("tmpfs", "/var/run", "tmpfs", MS_SHARED, 0);

  // Mount the cgroup file systems
  // When k8s supports cgroupv2 we can drop almost all of this
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
      mkdir(path, 0777);
      mount("cgroup", path, "cgroup", 0, cgroup);
    }
  }
  fclose(fp);
}
