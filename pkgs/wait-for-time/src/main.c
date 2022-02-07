
#include <sys/time.h>
#include <unistd.h>

#ifndef SLEEP_TIME
#define SLEEP_TIME 50
#endif

#ifndef SENSIBLE_TIME
#define SENSIBLE_TIME 1641560331
#endif

int main(int argc, char **argv) {
  struct timeval time;

  while (1) {
    gettimeofday(&time, (void *)0);
    if (time.tv_sec < SENSIBLE_TIME) {
      usleep(SLEEP_TIME);
    } else {
      return 0;
    }
  }

  // Shouldn't reach this?
  return 1;
}
