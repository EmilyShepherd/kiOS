#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
#include <wait_for_time.h>
#include "toml.h"
#include "manifests/dhcp.h"
#include "manifests/dhcp-cni.h"
#include "manifests/kubeadm-config.h"

#define REQUIRE(expected)                                     \
  if (val->type != expected) {                                \
    printf("[ERROR] %s has an unexpected value\n", val->key); \
  }

#define TOKENS_FILE "/etc/kubernetes/tokens"
#define KUBEADM_FILE "/tmp/kubeadm.yaml"
#define RESOLV_FILE "/etc/resolv.conf"
#define MANIFEST_DIR "/etc/kubernetes/manifests"

struct DHCPPod {
  char *interface;
  char *bridge;
  int cni;
  int enabled;
};

struct ControlPlane {
  int enabled;
  char *admin_token;
};

FILE *resolv;
struct DHCPPod dhcp = { NULL, NULL, 0, 0 };
struct ControlPlane cp = {0, NULL};

const char *CONTROL_PLANE_ARGS[] = {
  "kubeadm",
  "init",
  "--config",
  KUBEADM_FILE,
  "--skip-phases=preflight",
  NULL
};

char const *JOIN_ARGS[] = {
  "kubeadm",
  "join",
  NULL,
  "--token",
  NULL,
  "--discovery-token-ca-cert-hash",
  NULL,
  "--skip-phases=preflight",
  NULL
};

static void add_nameserver(const char *server) {
  fprintf(resolv, "nameserver %s\n", server);
}

void root(struct Value *val) {
  if (strcmp(val->key, "network.hostname") == 0) {
    REQUIRE(STRING);
    sethostname(val->string.value, val->string.length);

    // Write the hostname back to /etc/hostname.
    // Init will pick this up and set the hostname in the host uts
    // namespace prior to relaunching kubelet.
    FILE *hostname = fopen("/etc/hostname", "w");
    if (hostname) {
      fprintf(hostname, val->string.value);
      fclose(hostname);
    }
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
  } else if (strcmp(val->key, "join.master") == 0) {
    REQUIRE(STRING);
    JOIN_ARGS[2] = val->string.value;
    return;
  } else if (strcmp(val->key, "join.token") == 0) {
    REQUIRE(STRING);
    JOIN_ARGS[4] = val->string.value;
    return;
  } else if (strcmp(val->key, "join.discovery_hash") == 0) {
    REQUIRE(STRING);
    JOIN_ARGS[6] = val->string.value;
    return;
  } else if (strcmp(val->key, "control_plane.enabled") == 0) {
    REQUIRE(BOOLEAN);
    cp.enabled = val->integer;
  } else if (strcmp(val->key, "control_plane.api.admin_token") == 0) {
    REQUIRE(STRING);
    cp.admin_token = val->string.value;
    return;
  }
  if (val->type == STRING) {
    free(val->string.value);
  }
}

void dhcp_value(struct Value *val) {
  if (strcmp(val->key, "interface") == 0) {
    REQUIRE(STRING);
    dhcp.interface = val->string.value;
  } else if (strcmp(val->key, "cni") == 0) {
    REQUIRE(BOOLEAN);
    dhcp.cni = val->integer;
  } else if (strcmp(val->key, "bridge") == 0) {
    REQUIRE(STRING);
    dhcp.bridge = val->string.value;
  } else {
    printf("[ERROR] Unknown value for dhcp: %s\n", val->key);
    if (val->type == STRING) {
      free(val->string.value);
    }
  }
}

void run_dhcp() {
  if (dhcp.enabled == 0) {
    return;
  }

  char file[NAME_MAX];
  sprintf(file, "%s/dhcpd-%s.yaml", MANIFEST_DIR, dhcp.interface);
  FILE *fp = fopen(file, "w");
  if (!fp) {
    printf("[ERROR] Cannot create manifest file\n");
  } else if (!dhcp.cni) {
    fprintf(fp, DHCP_MANIFEST, dhcp.interface, dhcp.interface);
  } else {
    fprintf(fp, DHCP_CNI_MANIFEST,
        dhcp.interface, dhcp.bridge,
        dhcp.interface, dhcp.bridge,
        dhcp.bridge
    );
  }
  fclose(fp);

  if (dhcp.interface != NULL) {
    free(dhcp.interface);
    dhcp.interface = NULL;
  }
  if (dhcp.bridge != NULL) {
    free(dhcp.bridge);
    dhcp.bridge = NULL;
  }
  dhcp.cni = 0;
  dhcp.enabled = 0;
}

cb obj(char *name) {
  if (strcmp("pod.dhcp", name) == 0) {
    run_dhcp();
    dhcp.enabled = 1;
    return &dhcp_value;
  } else {
    printf("[ERROR] Unknown table type %s\n", name);
  }
}

int main(int argc, char **argv) {
  char *file = argv[1];
  resolv = fopen(RESOLV_FILE, "w");
  if (!resolv) {
    printf("[ERROR] Cannot open resolv.conf file\n");
    return 1;
  }

  struct TomlParser parser;
  toml_parser_init(&parser, file);
  parser.callback = &root;
  parser.objCallback = &obj;

  toml_parse(&parser);
  fclose(resolv);
  run_dhcp();

  const char **args;
  if (!cp.enabled) {
    args = JOIN_ARGS;
  } else {
    args = CONTROL_PLANE_ARGS;
    FILE *tokens = fopen(TOKENS_FILE, "w");
    fprintf(tokens, TOKEN_CONFIG, cp.admin_token);
    fclose(tokens);

    FILE *kubeadm = fopen(KUBEADM_FILE, "w");
    fprintf(kubeadm, KUBEADM_CONFIG);
    fclose(kubeadm);
  }

  wait_for_sensible_time();

  execv("/bin/kubeadm", (char * const*)args);

  // Shouldn't reach here
  printf("[ERROR] Couldn't run kubeadm!\n");

  return 1;
}
