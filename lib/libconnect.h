
#ifndef LIB_CONNECT_H_
#define LIB_CONNECT_H_

#define BACK_LOG (15)

#define CONN_FAMILY (AF_INET)
#define CONN_PROTOCOL (SOCK_STREAM)

/*Safe Calls*/
#define SAFE_CALL_EXIT (SAFE_CALL_EXIT_FALSE)
#define SAFE_CALL_EXIT_TRUE (1)
#define SAFE_CALL_EXIT_FALSE (0)

#define SAFE_CALL_PERROR (SAFE_CALL_PERROR_TRUE)
#define SAFE_CALL_PERROR_TRUE (1)
#define SAFE_CALL_PERROR_FALSE (0)

#define SETUP_SERVER_ERR (-1)
#define SETUP_SERVER_SUCCESS (1)

typedef struct {
  int fd;
  struct sockaddr_in addr;
}sock;


int send_pkt(int sock, void *datagram, int len);
int setup_server(sock *Sock, char *ip_address, int port, log *Log);

connection *ssl_connect(void);
void ssl_disconnect(connection *c);
void ssl_write(connection *c, void *text, int len);

int s_send(int sockfd, void *datagram, int len, int flags);
int s_socket(int domain, int type, int protocol);
int s_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int s_inet_pton(int af, char *src, void *dst);
int s_getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int s_bind(int sockfd, const struct sockaddr *addr,
	   socklen_t addrlen);
int s_listen(int sockfd, int backlog);

#endif
