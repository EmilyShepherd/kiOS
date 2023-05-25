#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int logs = open(argv[1], O_RDONLY);

  char buf[100];
  size_t read_bytes;

  while (read_bytes = read(logs, buf, sizeof(buf))) {
    write(STDOUT_FILENO, buf, read_bytes);
  }

  close(logs);
}
