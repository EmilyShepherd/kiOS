#include <stdlib.h>

#include "http/http.h"
#include "socket.h"

static void do_new_request(host_t *host, req_t *req);
static void pickup_request(host_t *conn, req_t *req);
static void new_connection(host_t *host);
static void read_http(conn_t *conn);

static PARSER_CB(to_eol);
static PARSER_CB(save_header);
static PARSER_CB(next_header);
static PARSER_CB(read_status);

host_t *hosts = NULL;

void open_http2(conn_t *conn);

/**
 * We currently just use the one context for the whole application
 */
WOLFSSL_CTX *ctx;

/**
 * Mostly sets up wolfSSL:
 *   - Sets up its internal memory
 *   - Creates a context which will use the highest TLS version possible
 *   - Loads the default CA certificates
 */
void init_http_client() {
  wolfSSL_Init();

  ctx = wolfSSL_CTX_new(wolfSSLv23_client_method());

  wolfSSL_CTX_load_verify_locations(ctx, "/etc/ssl/certs/ca-certificates.crt", NULL);
}

/**
 * Mark a connection as available again
 *
 * If there are any pending requests for the connection's host, the next
 * one will be picked up immediately for the connection to take over.
 * Otherwise the connection is added to the host's available pool.
 */
void mark_available(conn_t *conn) {
  conn->next = conn->host->available_conn;
  conn->host->available_conn = conn;
  if (conn->status != STATUS_OK) {
    conn->status = STATUS_OK;
    conn->host->pending_connections--;
  }
}

void pickup_requests() {
  host_t *host = hosts;

  while (host) {
    while (host->next_request) {
      if (host->available_conn) {
        pickup_request(host, host->next_request);
        host->next_request = host->next_request->next;
        host->pending_requests--;
      } else {
        if (host->type != HTTP1 && !host->pending_connections) {
          new_connection(host);
        } else while (host->pending_connections < host->pending_requests) {
          new_connection(host);
        }
        break;
      }
    }

    host = host->next;
  }
}

/**
 * Callback whenever the kernel detects a state change on the
 * connection's socket.
 *
 * If the connection is pending, it will attempt to continue
 * establishing an SSL connection. Otherwise it'll attempt to read an
 * HTTP response.
 */
void socket_cb(uint32_t event, conn_t *conn) {
  if (conn->status != STATUS_PENDING) {
    read_http(conn);
  } else if (wolfSSL_connect(conn->ssl_session) == SSL_SUCCESS) {
    char *protocol;
    unsigned short size;
    wolfSSL_ALPN_GetProtocol(conn->ssl_session, &protocol, &size);
    if (strcmp(protocol, "h2") != 0) {
      mark_available(conn);
    } else {
      conn->type = HTTP2;
      open_http2(conn);
    }
  }
}

/**
 * Creates a new connection for the given host
 */
static void new_connection(host_t *host) {
  conn_t *conn = malloc(sizeof(conn_t));
  conn->next = NULL;
  conn->host = host;
  conn->status = STATUS_PENDING;

  conn->socket = socket(host->addr->ai_family, SOCK_NONBLOCK | host->addr->ai_socktype, host->addr->ai_protocol);
  connect(conn->socket, host->addr->ai_addr, host->addr->ai_addrlen);

  conn->ssl_session = wolfSSL_new(ctx);
  wolfSSL_set_using_nonblock(conn->ssl_session, 1);
  wolfSSL_set_fd(conn->ssl_session, conn->socket);
  wolfSSL_UseSNI(conn->ssl_session, WOLFSSL_SNI_HOST_NAME, host->name, strlen(host->name));

  add_event_listener(conn->socket, (event_cb)&socket_cb, conn);
  host->pending_connections++;

  char *alpn_list = "h2,http/1.1";
  wolfSSL_UseALPN(conn->ssl_session, alpn_list, sizeof(alpn_list), WOLFSSL_ALPN_CONTINUE_ON_MISMATCH);
}

/**
 * Sets up a new host and initiates it to the correct defaults. It will
 * also perform a DNS lookup on the hostname and pre-emptively starts a
 * connection for it.
 */
host_t* new_host(char *hostname) {
  struct addrinfo hints;

  host_t *host = malloc(sizeof(host_t));
  strcpy(host->name, hostname);
  host->next_host_hint = NULL;
  host->type = HTTP1;
  host->pending_connections = 0;
  host->pending_requests = 0;
  host->available_conn = NULL;
  host->next_request = NULL;
  host->type = UNKNOWN;
  host->next = hosts;
  host->next_stream_id = 1;
  hosts = host;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;

  getaddrinfo(hostname, NULL, &hints, &host->addr);

  struct sockaddr *addr = host->addr->ai_addr;
  if (addr->sa_family == AF_INET) {
    ((struct sockaddr_in*)addr)->sin_port = htons(443);
  } else {
    ((struct sockaddr_in6*)addr)->sin6_port = htons(443);
  }

  new_connection(host);

  return host;
}

static PARSER_CB(slurp_until) {
  while (ptr[*i] != p->target) {
    p->str_target[p->str_i++] = ptr[*i]; // | 0x20;
    INC();
  }

  p->str_target[p->str_i] = 0;

  p->func = p->then;
  INC();
  return 0;
}

static PARSER_CB(follow_next) {
  conn_t *conn = (conn_t*)p->data;
  host_t *host = conn->host->next_host_hint;
  conn->status = STATUS_UNNEEDED;

  do_new_request(host, conn->req);

  p->func = &to_eol;
  return 0;
}

static PARSER_CB(read_new_path) {
  conn_t *conn = (conn_t*)p->data;
  host_t *host = conn->host;

  if (!host->next_host_hint || strcmp(host->next_host_hint->name, p->str_buffer) != 0) {
    host->next_host_hint = new_host(p->str_buffer);
  }

  p->str_i = 1;
  p->func = &slurp_until;
  p->then = &follow_next;
  p->target = '\r';
  p->str_target = conn->req->path;
  conn->req->path[0] = '/';

  return 0;
}

static PARSER_CB(read_new_host) {
  p->str_i = 0;
  p->func = &slurp_until;
  p->then = &read_new_path;
  p->target = '/';
  p->str_target = p->str_buffer;

  return 0;
}

static PARSER_CB(start_location) {
  strcpy(p->str_buffer, "https://");
  p->str_i = 0;
  p->func = &expect_token;
  p->then = &read_new_host;

  return 0;
}

static PARSER_CB(save_length) {
  ((conn_t*)p->data)->expected_length = p->target;
  p->func = &to_eol;

  return 0;
}

static PARSER_CB(read_length) {
  p->func = &read_int;
  p->target = 0;
  p->then = &save_length;

  return 0;
}

static PARSER_CB(save_header) {
  while (ptr[*i] != ':') {
    p->str_target[p->str_i++] = ptr[*i] | 0x20;
    INC();
  }

  p->str_target[p->str_i] = 0;
  p->func = &to_eol;

  if (strcasecmp(p->str_target, "location") == 0) {
    p->func = &skip_whitespace;
    p->then = &start_location;
    INC();
  } else if (strcasecmp(p->str_target, "content-type") == 0) {
    //
  } else if (strcasecmp(p->str_target, "content-length") == 0) {
    p->func = &skip_whitespace;
    p->then = &read_length;
    INC();
  }

  return 0;
}

static PARSER_CB(body) {
  int bytes = max - *i + 1;
  conn_t *conn = (conn_t*)p->data;
  conn->read_length += bytes;

  if (conn->status != STATUS_UNNEEDED) {
    int ret = conn->req->body_cb(&ptr[*i], bytes, conn->req->data);

    if (ret == 0) {
      conn->status = STATUS_UNNEEDED;
    }
  }

  return 1;
}

static PARSER_CB(start_body) {
  p->func = &body;
  EXPECT('\n');
  return 0;
}

static PARSER_CB(next_header) {
  if (ptr[*i] == '\r') {
    p->func = &start_body;
    INC();
  } else {
    p->func = &save_header;
    p->str_i = 0;
    p->str_target = p->str_buffer;
  }

  return 0;
}

static PARSER_CB(to_eol) {
  while (ptr[*i] != '\n') {
    INC();
  }

  p->func = &next_header;
  INC();
  return 0;
}

static PARSER_CB(read_status) {
  p->target = 0;
  p->func = &read_int;
  p->then = &to_eol;

  return 0;
}

/**
 * Queues up a request to be performed.
 *
 * If there is an available connection in the host's pool, it will
 * assign it straight away. Otherwise it'll add it to the host's
 * pending_requests. If there aren't enough pending connections to cover
 * it, it will request more.
 */
static void do_new_request(host_t *host, req_t *req) {
  if (host->available_conn) {
    pickup_request(host, req);
  } else {
    req->conn = NULL;
    host->pending_requests++;
    if (host->next_request == NULL) {
      host->next_request = req;
    } else {
      host->last_request->next = req;
    }
    host->last_request = req;
  }
}

void do_http2_req(conn_t *conn, req_t *req);

/**
 * Sends an HTTP request on the given connection
 */
static void pickup_request(host_t *host, req_t *req) {
  conn_t *conn = host->available_conn;

  conn->expected_length = 0;
  conn->read_length = 0;
  conn->req = req;
  req->conn = conn;

  printf("REQ\n");

  switch (conn->type) {
    case HTTP1:
      memset(&conn->p, 0, sizeof(Parser));
      conn->p.func = &expect_token;
      conn->p.str_i = 0;
      conn->p.then = &read_status;
      conn->p.data = conn;
      strcpy(conn->p.str_buffer, "HTTP/1.1 ");

      // Remove this connection from the available pool.
      host->available_conn = conn->next;

      char buff[2000] = "GET ";
      strcat(buff, req->path);
      strcat(buff, " HTTP/1.1\nhost: ");
      strcat(buff, conn->host->name);
      strcat(buff, "\nconnection: keep-alive\naccept: application/vnd.docker.distribution.manifest.v2+json, application/vnd.docker.distribution.manifest.list.v2+json\n\n");
      wolfSSL_write(conn->ssl_session, buff, strlen(buff));
      break;
    case HTTP2:
      do_http2_req(conn, req);
      break;
  }
}

/**
 * Convenience function to create a new artifact request in the correct
 * format.
 */
void new_request(host_t *host, char *image, char *type, char *name, write_callback cb, void* data) {
  req_t *req = malloc(sizeof(req_t));
  req->body_cb = cb;
  req->data = data;
  req->next = NULL;
  sprintf(req->path, "/v2/%s/%ss/%s", image, type, name);

  do_new_request(host, req);
}

/**
 * Reads available data from a connection's socket and passes them to
 * the HTTP parser.
 */
static void read_http(conn_t *conn) {
  unsigned char buff[32000];
  int ret, tot = 0;

  while ((ret = wolfSSL_read(conn->ssl_session, buff, sizeof(buff))) > 0) {
    tot += ret;
    parse_block(buff, ret, &conn->p);
  }

  if (!tot) return;

  if (conn->expected_length && conn->expected_length == conn->read_length) {
    if (conn->req->conn == conn) {
      free(conn->req);
    }
    mark_available(conn);
  }
}
