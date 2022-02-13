#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
#include "toml.h"
#include "manifests/dhcp.h"
#include "manifests/dhcp-cni.h"

#define REQUIRE(expected)                                     \
  if (val->type != expected) {                                \
    printf("[ERROR] %s has an unexpected value\n", val->key); \
  }

struct DHCPPod {
  char interface[10];
  char bridge[10];
  int cni;
};

FILE *resolv;
struct DHCPPod dhcp = { "", "", 0 };

static void add_nameserver(const char *server) {
  fprintf(resolv, "nameserver %s\n", server);
}

void root(struct Value *val) {
  if (strcmp(val->key, "network.hostname") == 0) {
    REQUIRE(STRING);
    sethostname(val->string.value, val->string.length);
  } else if (strcmp(val->key, "network.nameservers") == 0) {
    REQUIRE(STRING);
    if (strcmp(val->string.value, "google") == 0) {
      add_nameserver("2001:4860:4860::8888");
      add_nameserver("2001:4860:4860::8844");
      add_nameserver("8.8.8.8");
      add_nameserver("8.8.4.4");
    } else {
      add_nameserver(val->string.value);
    }
  }
}

void dhcp_value(struct Value *val) {
  if (strcmp(val->key, "interface") == 0) {
    REQUIRE(STRING);
    strcpy(dhcp.interface, val->string.value);
  } else if (strcmp(val->key, "cni") == 0) {
    REQUIRE(BOOLEAN);
    dhcp.cni = val->integer;
  } else if (strcmp(val->key, "bridge") == 0) {
    REQUIRE(STRING);
    strcpy(dhcp.bridge, val->string.value);
  } else {
    printf("[ERROR] Unknown value for dhcp: %s\n", val->key);
  }
}

void run_dhcp() {
  if (dhcp.interface[0] == 0) {
    return;
  }

  char file[NAME_MAX];
  sprintf(file, "/etc/kubernetes/manifests/dhcpd-%s.yaml", dhcp.interface);
  FILE *fp = fopen(file, "w");

  if (!dhcp.cni) {
    fprintf(fp, DHCP_MANIFEST, dhcp.interface, dhcp.interface);
  } else {
    if (dhcp.bridge[0] == 0) {
      strcpy(dhcp.bridge, "cni0");
    }
    fprintf(fp, DHCP_CNI_MANIFEST,
        dhcp.interface, dhcp.bridge,
        dhcp.interface, dhcp.bridge,
        dhcp.interface, dhcp.bridge
    );
  }

  dhcp.interface[0] = 0;
  dhcp.bridge[0] = 0;
  dhcp.cni = 0;
}

cb obj(char *name) {
  if (strcmp("pod.dhcp", name) == 0) {
    run_dhcp();
    strcpy(dhcp.interface, "eth0");
    return &dhcp_value;
  } else {
    printf("[ERROR] Unknown table type %s\n", name);
  }
}

int main(int argc, char **argv) {
  char *file = argv[1];
  resolv = fopen("/etc/resolv.conf", "w");

  struct TomlParser parser;
  toml_parser_init(&parser, file);
  parser.callback = &root;
  parser.objCallback = &obj;

  toml_parse(&parser);
  fclose(resolv);
  run_dhcp();

  return 0;
}
