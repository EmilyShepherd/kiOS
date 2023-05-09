
#include "include/exe.h"
#include "include/fs.h"
#include "include/socket.h"

#include <net/if.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

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
 * Enable IP Forwarding
 *
 * Although kiOS tries to be relatively unopinionated, enabling ip
 * forwarding is almost always a requirement for Pod / Service
 * Networking to work as expected, so we will set that sysctl here so as
 * to not require another service to set it. This is only set at boot,
 * so its value can be overriden at runtime by a sufficiently privileged
 * service, if required.
 */
void enable_ip_forwarding(void) {
  FILE *fp = fopen("/proc/sys/net/ipv4/ip_forward", "w");
  fputs("1", fp);
  fclose(fp);
}

/**
 * Start Console
 *
 * Opens up the console character device, if specified by the CONSOLE/
 * console environment variable, and sets it up as the init process'
 * stdin, stdout, and stderr.
 */
void start_console(void) {
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
}

/**
 * Bring Interface Up
 *
 * Updates the given interface name to be UP.
 */
static void bring_if_up(const char *iff) {
  int ip = socket(PF_INET, SOCK_DGRAM, 0);
  struct ifreq ifr;
  strncpy(ifr.ifr_name, iff, 16);
  ioctl(ip, SIOCGIFFLAGS, &ifr);
  ifr.ifr_flags |= IFF_UP;
  ioctl(ip, SIOCSIFFLAGS, &ifr);
}

int main(int argc, char **argv) {
  start_console();
  printf("Kios Init\n");

  // We want to enable networking as quickly as possible so that the
  // kernel can be getting on with SLAAC, if available on the network,
  // whilst we are busy with other things.
  bring_if_up("lo");
  bring_if_up("eth0");

  mount_fs();
  mount_datapart();

  putenv("PATH=/bin");
  start_container_runtime();

  set_hostname_from_file();
  enable_ip_forwarding();
  start_socket();

  wait_for_path(CRIO_SOCK);

  if (!fexists(KUBELET_CONFIG)) {
    start_kubelet();
    wait_for_path(KUBELET_CONFIG);
    set_hostname_from_file();

    stop_kubelet();
  }

  start_kubelet();
  run_wait_loop();
}
