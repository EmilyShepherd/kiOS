
#include "image/image.h"
#include "socket.h"

int main(int argc, char** argv) {
  start_socket();
  init_http_client();

  ImageDownload* i = download_image("registry.k8s.io", "kube-apiserver", "v1.26.0");

  run_socket_loop();

  return 0;
}
