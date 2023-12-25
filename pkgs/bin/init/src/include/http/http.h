
#ifndef _H_HTTP
#define _H_HTTP 1

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

#include "parser/parser.h"

#define UNKNOWN 0
#define HTTP1 1
#define HTTP2 2

#define DNS_NAME_MAX 253
#define IMAGE_NAME_MAX 128

#define STATUS_OK 0
#define STATUS_ONGOING 1
#define STATUS_UNNEEDED 2
#define STATUS_PENDING 3

typedef struct Host host_t;
typedef struct Connection conn_t;
typedef struct ArtifactRequest req_t;

struct Connection {
  /**
   * Each connection has a dedicated SSL session associated with it
   */
  WOLFSSL* ssl_session;

  /**
   * File descriptor for the connection socket
   */
  int socket;

  /**
   * The host that this connection is part of
   */
  host_t *host;

  /**
   * The status of this connection
   */
  int status;

  /**
   * If the content-length header is seen, it is saved there - this is
   * used for fast skipping over the rest of a message body when it is
   * no longer needed.
   */
  int expected_length;
  int read_length;

  /**
   * When connections are in the available pool, they are queued as a
   * linked list
   */
  conn_t *next;

  /**
   * The request this connection is servicing.
   */
  req_t *req;

  /**
   * Parser state for the connection's HTTP parser.
   *
   * This is _not_ used for body parsing. Requests should use their own
   * nested parsers if needed.
   */
  Parser p;
};

struct Host {
  /**
   * Hostname is used for DNS lookups and as the Host: vhost header for
   * all requests
   */
  char name[DNS_NAME_MAX];

  /**
   * Cached IP DNS lookup
   */
  struct addrinfo *addr;

  /**
   * The type of connection this host supports (HTTP1 or HTTP2)
   */
  int type;

  /**
   * A linked list of available connections. These will be picked up in
   * a Last-In-First-Out (stack) method.
   */
  int pending_connections;
  conn_t *available_conn;

  /**
   * When we follow a redirect to a new host, we'll save a pointer to it
   * here, on the assumption that future requests are likely to be
   * redirected too.
   */
  host_t *next_host_hint;

  /**
   * A linked list of pending requests. These will be picked up and
   * processed in a First-In-First-Out (queue) method.
   */
  int pending_requests;
  req_t *next_request;
  req_t *last_request;

  /**
   * Next host in the list of all hosts
   */
  host_t* next;
};

struct ArtifactRequest {
  /**
   * A request has a one to many relationship with connections (as
   * redirects sometimes require multiple connections).
   *
   * This marks the current connection in use.
   */
  conn_t *conn;

  /**
   * A function that is called to process the response body.
   */
  write_callback body_cb;

  /**
   * Arbitrary data for the write callback
   */
  void* data;

  /**
   * The path of the request
   *
   * NB: This may change when redirections are followed.
   */
  char path[1000];

  /**
   * When requests are pending, they are queued in a linked list.
   */
  req_t *next;
};

/**
 * Call once to init the http client
 */
void init_http_client();

/**
 * Call once per registry
 */
host_t* new_host(char *name);

/**
 * Perform a new artifact request
 */
void new_request(host_t* host, char *image, char *type, char *name, write_callback cb, void* data);

void pickup_requests();

#endif /* ifndef _H_HTTP */
