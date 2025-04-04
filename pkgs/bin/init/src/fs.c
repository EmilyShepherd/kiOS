
#include "include/fs.h"
#include "include/gpt.h"
#include "include/kmsg.h"

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

static void mkinit_mount(const char *type, const char *target, unsigned long flags) {
  mkdir(target, 0755);
  init_mount(type, target, flags);
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
  mkinit_mount("devpts", "/dev/pts", 0);
  mkinit_mount("securityfs", "/sys/kernel/security", 0);
  mkinit_mount("cgroup2", "/sys/fs/cgroup", 0);

  // Sensible tmpfs mounts
  tmp_mount("/run");
  tmp_mount("/tmp");

  tmp_mount("/var/cache");
  tmp_mount("/var/tmp");
  tmp_mount("/var/run");
  tmp_mount("/var/lib/kubelet");
  mount("tmpfs", "/var/run", "tmpfs", MS_SHARED, 0);
  mount("tmpfs", "/var/lib/kubelet", "tmpfs", MS_SHARED, 0);
  mkdir("/var/run/crio", 0700);
  mkdir("/var/lib/kubelet/pods", 0500);
  mkdir("/var/lib/kubelet/seccomp", 0500);

  // Like systemd, it is slightly more helpful to us to make our mounts
  // shared rather than private. This allows kubernetes mountPropagation
  // work as expected. This, ironically, is actually _more_ secure as,
  // without sharing our mounts, the only way to make changes to the
  // host system is to setns into the host mount namespace. This
  // requires granting the container SYS_CHROOT and SYS_ADMIN, which
  // gives way more permissions than just selectively mounting hostPaths
  // with mountPropagation set on them.
  mount(NULL, "/", NULL, MS_REC|MS_SHARED, NULL);
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
    if (umount(current.mount) != 0) {
      skip++;
    }
    rewind(mounts);
  } while (!feof(mounts));
}
