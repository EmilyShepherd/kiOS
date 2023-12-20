
#include "include/exe.h"
#include "include/fs.h"
#include "include/kmsg.h"
#include "include/socket.h"
#include "http/http.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

client_t *clients = NULL;

int server_socket;
int epollfd;

/**
 * Notifies all active connections with the given CMD
 */
void notify_all(unsigned char event) {
  client_t *client = clients;
  while (client) {
    write(client->fd, &event, sizeof(event));
    client = client->next;
  }
}

/**
 * soft_shutdown() is called when the system gets a shutdown request.
 * Its only job is to notify all current socket clients of the system's
 * desire to shutdown. It is expected that kubelet will be one of these
 * clients - it will trigger its own graceful shutdown process, which
 * shuts down pods according to their own rules. When finished, it will
 * notify the system socket that it should continue the shutdown.
 *
 * We have an int argument so that this function can be used as a signal
 * handler (for shutdown signals). However we do not use this arg.
 */
void soft_shutdown(int) {
  notice("System Shutdown Requested.");
  info("Waiting for kubelet graceful shutdown");
  notify_all(EVENT_SHUTDOWN);
}

/**
 * do_shutdown() is responsible for doing the "conventional" steps an
 * init program takes to shutdown the system, ie: forcing all processes
 * to stop, unmount the file systems, call sync() and then ask the
 * kernel to power off.
 *
 * This is called _after_ kubelet has finished its graceful shutdown, so
 * the process killing at this stage will be fairly abrupt.
 */
void do_shutdown(void) {
  info("Kubelet graceful shutdown complete");
  warn("System shutting down");

  // Update the flag to ensure that crio and kubelet are not
  // automatically restarted while we shutdown.
  should_restart_processes = 0;

  kill(-1, SIGTERM);
  sleep(1);
  kill(-1, SIGKILL);

  umount_all();
  sync();

  pid_t pid = fork();
  if (pid == 0) {
    reboot(RB_POWER_OFF);
    // Kernel calls exit - shouldn't get here
  }
}

/**
 * Runs a listener and processes any commands received
 */
static void client_thread(uint32_t event, client_t *client) {
  unsigned char cmd;

  read(client->fd, &cmd, sizeof(cmd));

  switch (cmd) {
    case CMD_SHUTDOWN:
      soft_shutdown(0);
      break;
    case CMD_CONTINUE_SHUTDOWN:
      do_shutdown();
      break;
    case CMD_RESTART_KUBELET:
      // It is good enough to simply stop the kubelet as the wait
      // loop will notice and restart it for us.
      stop_kubelet();
      break;
    case CMD_RESTART_CRIO:
      stop_container_runtime();
      break;
  }
}

static void new_client(uint32_t event, void* data) {
  client_t *client = malloc(sizeof(client_t));

  client->next = clients;
  clients = client;

  client->fd = accept(server_socket, NULL, NULL);

  add_event_listener(client->fd, (event_cb)&client_thread, client);
}

/**
 * Starts the system socket by spawning a number of threads and
 * listening in each
 */
void start_socket(void) {
  epollfd = epoll_create1(0);

  server_socket = socket(PF_UNIX, SOCK_STREAM, 0);

  struct sockaddr_un addr;
  addr.sun_family = PF_UNIX;
  strcpy(addr.sun_path, "/run/system.sock");
  bind(server_socket, (struct sockaddr *)&addr, sizeof(addr));
  listen(server_socket, SYSTEM_SOCKET_CONNECTIONS);

  add_event_listener(server_socket, &new_client, NULL);
}

void add_event_listener(int fd, event_cb cb, void* data) {
  event_t *evt_cb = malloc(sizeof(event_t));
  evt_cb->cb = cb;
  evt_cb->data = data;

  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
  ev.data.ptr = evt_cb;

  epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}

void remove_event_listener(int fd) {
  epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
}

void run_socket_loop(void) {
  struct epoll_event events[SYSTEM_SOCKET_CONNECTIONS];

  while (1) {
    int nfds = epoll_wait(epollfd, events, SYSTEM_SOCKET_CONNECTIONS, -1);

    for (int n = 0; n < nfds; ++n) {
      struct EventCallback *callback = events[n].data.ptr;
      callback->cb(events[n].events, callback->data);
    }
  }
}

