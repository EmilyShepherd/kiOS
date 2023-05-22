
#include "include/kmsg.h"

#include <stdio.h>

/**
 * Send a message to the kernel ring buffer
 */
void kmsg(int level, char *msg) {
  FILE *kmsg_fd = fopen("/dev/kmsg", "w");
  fprintf(kmsg_fd, "<%d>kiOS: %s", level, msg);
  fclose(kmsg_fd);
}

/**
 * Send an emergency message
 */
void emerg(char *msg) {
  kmsg(KERN_EMERG, msg);
}

/**
 * Send an alert message
 */
void alert(char *msg) {
  kmsg(KERN_ALERT, msg);
}

/**
 * Send a critical message
 */
void crit(char *msg) {
  kmsg(KERN_CRIT, msg);
}

/**
 * Send a warning message
 */
void warn(char *msg) {
  kmsg(KERN_WARNING, msg);
}

/**
 * Send a notice message
 */
void notice(char *msg) {
  kmsg(KERN_NOTICE, msg);
}

/**
 * Send an info message
 */
void info(char *msg) {
  kmsg(KERN_INFO, msg);
}

/**
 * Send a debug message
 */
void debug(char *msg) {
  kmsg(KERN_DEBUG, msg);
}
