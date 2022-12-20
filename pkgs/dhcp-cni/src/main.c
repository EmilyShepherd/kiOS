#include <string.h>
#include <net/if.h>
#include <sys/socket.h>
#include <linux/sockios.h>
#include <stdio.h>
#include <sys/stat.h>

int cp(char *to);

int main(int argc, char **argv) {
  const char *bridge = argv[1];
  const char *bridge_with = argv[2];

  int ip = socket(AF_INET, SOCK_STREAM, 0);

  ioctl(ip, SIOCBRADDBR, bridge);

  struct ifreq ifr;
  strncpy(&ifr.ifr_name, bridge, 16);
  ifr.ifr_ifindex = if_nametoindex(bridge_with);
  ioctl(ip, SIOCBRADDIF, &ifr);
  ioctl(ip, SIOCGIFFLAGS, &ifr);
  ifr.ifr_flags |= IFF_UP;
  ioctl(ip, SIOCSIFFLAGS, &ifr);

  cp("/host/opt/cni/bin/dhcp");
  cp("/host/opt/cni/bin/bridge");
  cp("/host/etc/cni/net.d/default.conflist");

  return 0;
}

int cp(char *to) {
  // Strip off the "/host" part of the path
  const char *from = &to[5];

  char c[4096];
  FILE *fromFile = fopen(from, "r");
  FILE *toFile = fopen(to, "w");

  while (!feof(fromFile)) {
    size_t bytes = fread(c, 1, sizeof(c), fromFile);
    if (bytes) {
      fwrite(c, 1, bytes, toFile);
    }
  }

  // Copy accross the file permissions to the location
  struct stat fromStat;
  stat(from, &fromStat);
  chmod(to, fromStat.st_mode);

  fclose(fromFile);
  fclose(toFile);
}
