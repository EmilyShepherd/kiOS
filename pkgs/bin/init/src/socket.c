
#include "include/exe.h"
#include "include/fs.h"
#include "include/socket.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <pthread.h>
#include <unistd.h>

int server_socket;

/**
 * We have a static list of connections as file descriptors require very
 * little memory and we do not anticipate needing to support many
 * clients to the system socket.
 */
int connections[SYSTEM_SOCKET_CONNECTIONS];

/**
 * Notifies all active connections with the given CMD
 */
void notify_all(unsigned char event) {
  for (int i = 0; i < SYSTEM_SOCKET_CONNECTIONS; i++) {
    if (connections[i] > 0) {
      write(connections[i], &event, sizeof(event));
    }
  }
}

/**
 * soft_shutdown() is called when the system gets a shutdown request.
 * Its only job is to notify all current socket clients of the system's
 * desire to shutdown. It is expected that kubelet will be one of these
 * clients - it will trigger its own graceful shutdown process, which
 * shuts down pods according to their own rules. When finished, it will
 * notify the system socket that it should continue the shutdown.
 */
void soft_shutdown(void) {
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
  // Update the flag to ensure that crio and kubelet are not
  // automatically restarted while we shutdown.
  should_restart_processes = 0;

  kill(-1, SIGTERM);
  sleep(1);
  kill(-1, SIGKILL);

  umount_all();
  sync();

  sleep(30);

  pid_t pid = fork();
  if (pid == 0) {
    reboot(RB_POWER_OFF);
    // Kernel calls exit - shouldn't get here
  }

  waitpid(pid, NULL, 0);
  while (1) sleep(1);
}

/**
 * Runs a listener and processes any commands received
 */
void client_thread(int *connection) {
  while (1) {
    *connection = accept(server_socket, NULL, NULL);
    int ret;
    unsigned char cmd;
    while (read(*connection, &cmd, sizeof(cmd)) > 0) {
      switch (cmd) {
        case CMD_SHUTDOWN:
          soft_shutdown();
          break;
        case CMD_CONTINUE_SHUTDOWN:
          do_shutdown();
          break;
      }
    }
    close(*connection);
    *connection = 0;
  }
}

/**
 * Starts the system socket by spawning a number of threads and
 * listening in each
 */
void start_socket(void) {
  server_socket = socket(PF_UNIX, SOCK_STREAM, 0);

  struct sockaddr_un addr;
  addr.sun_family = PF_UNIX;
  strcpy(addr.sun_path, "/run/system.sock");
  bind(server_socket, (struct sockaddr *)&addr, sizeof(addr));

  listen(server_socket, SYSTEM_SOCKET_CONNECTIONS);
  for (int i = 0; i < SYSTEM_SOCKET_CONNECTIONS; i++) {
    pthread_t thread;
    pthread_create(&thread, NULL, (void *(*)(void *))&client_thread, &connections[i]);
  }
}

