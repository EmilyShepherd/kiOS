
#ifndef _SOCKET_H
#define _SOCKET_H 1

#include <linux/limits.h>

#define SYSTEM_SOCKET_CONNECTIONS 5

/**
 * Commands accepted by the system socket
 *
 * SHUTDOWN           Request a system shutdown
 * CONTINUE_SHUTDOWN  Continue the shutdown procedure by killing processes and umounting all file systems.
 * RESTART_KUBELET    Requests that the kubelet should be restarted
 * RESTART_CRIO       Requests that the container runtime should be restarted
 */
#define CMD_SHUTDOWN 0x01
#define CMD_CONTINUE_SHUTDOWN 0x02
#define CMD_RESTART_KUBELET 0x03
#define CMD_RESTART_CRIO 0x04

/**
 * Events send out by the system
 *
 * SHUTDOWN   Sent out when a shutdown request has been received
 */
#define EVENT_SHUTDOWN 0x01

/**
 * Send a notification to all clients of the system socket
 */
void notify_all(unsigned char);

/**
 * Start the system socket
 */
void start_socket(void);

typedef void (*event_cb)(uint32_t event, void *data);
typedef struct EventCallback event_t;
typedef struct client client_t;

struct EventCallback {
  event_cb cb;
  void *data;
};

struct client {
  int fd;
  client_t *next;
};

/**
 * Sends out a shutdown event to any current clients on the system
 * socket. This is intended to be used for the kubelet graceful shutdown
 * process.
 */
void soft_shutdown(int);

void run_socket_loop(void);

void add_event_listener(int fd, event_cb cb, void* data);
void remove_event_listener(int fd);

#endif
