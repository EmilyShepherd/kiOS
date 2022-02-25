
#include <sys/time.h>
#include <unistd.h>
#include "wait_for_time.h"

#ifndef SLEEP_TIME
#define SLEEP_TIME 50
#endif

#ifndef SENSIBLE_TIME
#define SENSIBLE_TIME 1641560331
#endif

void wait_for_sensible_time() {
  struct timeval time;

  while (1) {
    gettimeofday(&time, (void *)0);
    if (time.tv_sec < SENSIBLE_TIME) {
      usleep(SLEEP_TIME);
    } else {
      return;
    }
  }
}
