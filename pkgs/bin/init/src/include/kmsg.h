
#ifndef _KMSG_H
#define _KMSG_H 1

#define KERN_EMERG 0
#define KERN_ALERT 1
#define KERN_CRIT 2
#define KERN_ERR 3
#define KERN_WARNING 4
#define KERN_NOTICE 5
#define KERN_INFO 6
#define KERN_DEBUG 7

/**
 * Send a message to the kernel ring buffer
 */
void kmsg(int level, char *msg);

/**
 * Send an emergency message
 */
void emerg(char *msg);

/**
 * Send an alert message
 */
void alert(char *msg);

/**
 * Send a critical message
 */
void crit(char *msg);

/**
 * Send a warning message
 */
void warn(char *msg);

/**
 * Send a notice message
 */
void notice(char *msg);

/**
 * Send an info message
 */
void info(char *msg);

/**
 * Send a debug message
 */
void debug(char *msg);

#endif
