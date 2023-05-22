
#include "include/exe.h"
#include "include/fs.h"
#include "include/kmsg.h"
#include "include/socket.h"

#include <net/if.h>
#include <signal.h>
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

/**
 * Clears the process' command line and replaces it with "kiOS".
 *
 * This is a vanity move so that it shows up pleasantly in tools like
 * "ps"
 */
static void clear_cmd_line(char **argv) {
  strncpy(argv[0], "kiOS", strlen(argv[0]));
  while (*++argv) {
    memset(*argv, 0, strlen(*argv));
  }
}

int main(int argc, char **argv) {
  notice("kiOS Init\n");

  mount_fs();
  mount_datapart();

  putenv("PATH=/bin");
  start_container_runtime();

  // Cri-o (started in the step above) typically takes ~3 seconds to
  // start up on a modern machine, so we start it as soon as is
  // reasonable to do so. All our other init steps are performed in that
  // three second window.
  bring_if_up("lo");
  bring_if_up("eth0");
  set_hostname_from_file();
  enable_ip_forwarding();
  start_socket();
  signal(SIGTERM, &soft_shutdown);
  clear_cmd_line(argv);

  // Now we are finished with our own setup, wait for crio to be ready.
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
