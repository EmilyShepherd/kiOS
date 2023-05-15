
#include "include/fs.h"
#include "include/gpt.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

char datapart[NAME_MAX + 5];

/**
 * File Exists
 *
 * Helper function to check if the given file exists
 */
int fexists(const char *path) {
  struct stat _;
  return !stat(path, &_);
}

static void init_mount(const char *type, const char *target, unsigned long flags) {
  mount(type, target, type, flags | MS_NOSUID | MS_NOEXEC, 0);
}

static void tmp_mount(const char *target) {
  init_mount("tmpfs", target, MS_NODEV);
}

static void bind_mount(const char *src, const char *target, unsigned long flags) {
  mount(src, target, 0, flags | MS_BIND, 0);
}

/**
 * Mount Filesystem
 *
 * Sets up and mounts kiOS' base OS file system. This includes most
 * "standard" things like devtmpfs, sysfs, proc, etc as well as a few
 * sensible tmpfs directories.
 *
 * Cgroup v1 mounts are also created.
 */
void mount_fs(void) {
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
  tmp_mount("/var/lib/kubelet");
  mount("tmpfs", "/var/run", "tmpfs", MS_SHARED, 0);
  mkdir("/var/run/crio", 0700);
  mkdir("/var/lib/kubelet/pods", 0700);

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

/**
 * Mount Datapart
 *
 * Mounts the data partition folders. The data partition to use is
 * normally given by the datapart kernel command line parameter, however
 * kiOS will use an internal default if nothing else is provided.
 *
 * In the data partition, kiOS expects the following directories to
 * exist:
 *   modules - mounted to /lib/modules
 *   log - mounted to /var/log
 *   lib - mounted to /var/lib
 *   etc - mounted to /etc
 *
 * Other files and directories in the datapartition are ignored and are
 * not accessible at runtime by init-created mounts.
 */
void mount_datapart(void) {
  determine_datapart(datapart);

  wait_for_path(datapart);
  mount(datapart, "/tmp", "ext4", 0, 0);

  mkdir("/tmp/meta", 0700);
  bind_mount("/tmp/meta", "/var/meta", MS_NODEV | MS_NOEXEC | MS_NOSUID);

  mkdir("/var/meta/log", 0700);
  bind_mount("/var/meta/log", "/var/log", 0);

  mkdir("/var/meta/etc", 0700);
  mkdir("/var/meta/.etc.work", 0700);
  mount("etc", "/etc", "overlay", 0, "lowerdir=/etc,workdir=/var/meta/.etc.work,upperdir=/var/meta/etc");

  mkdir("/tmp/data", 0700);
  mkdir("/tmp/data/pods", 0700);
  mkdir("/tmp/data/oci", 0700);
  bind_mount("/tmp/data/pods", "/var/lib/kubelet/pods", 0);
  bind_mount("/tmp/data/oci", "/var/lib/containers/storage", 0);

  bind_mount("/tmp/modules", "/lib/modules", STATIC_FLAGS);
  umount("/tmp");
}

/**
 * Wait for Path
 *
 * Waits for a path to exist by setting up an inotify watcher for its
 * parent directory.
 *
 * WARNING: This will never return, unless the file comes into
 * existence. Only use this for files which are expected to exist, but
 * may not yet early in the boot process.
 */
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

/**
 * Reads the next line of the mounts file and loads the relevant
 * information about the mount (its id, parent id, and mount point). If
 * the mount point is "/proc" - this will be skipped over.
 */
static void read_next(struct mountinfo *m, FILE *mounts) {
  do {
    fscanf(mounts, "%d %d %*d:%*d %*s %s", &m->id, &m->parent, m->mount);
    while (fgetc(mounts) != '\n' && !feof(mounts)) ;;
  } while (strcmp(m->mount, "/proc") == 0);
}

/**
 * umount_all() is responsible for looping over all of the current mount
 * points and unmounting them in a sensible order.
 */
void umount_all(void) {
  FILE *mounts = fopen("/proc/self/mountinfo", "rb");
  struct mountinfo current;
  struct mountinfo cmp;
  int skip = 0;

  do {
    for (int i = 0; i <= skip; i++) {
      if (feof(mounts)) {
        return;
      }
      read_next(&current, mounts);
    }
    while (!feof(mounts)) {
      read_next(&cmp, mounts);
      if (cmp.id == current.id) {
        continue;
      } else if (cmp.parent == current.id) {
        current.id = cmp.id;
        current.parent = cmp.parent;
        strcpy(current.mount, cmp.mount);
        rewind(mounts);
      }
    }
    printf("Unmount %s...", current.mount);
    if (umount(current.mount) != 0) {
      printf("Err!\n");
      skip++;
    } else {
      printf("Ok\n");
    }
    rewind(mounts);
  } while (!feof(mounts));
}
