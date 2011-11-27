#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "libservice.h"
#include "libconnect.h"
#include "liblog.h"

extern int testing;

int send_pkt(int sockfd, void *datagram, int len)
{
   if(!testing)
     return (s_send(sockfd, (void *)datagram, len, ) == -1) ? 1 : 1; //why return 1 for both sit
   return 0;
}

/* SSL Connection Stuff */
connection *ssl_connect(void)
{
   connection *c;

   c = malloc(sizeof(connection));
   c->sslHandle = NULL;
   c->sslContext = NULL;

   if((c->socket = tcpConnect()))
   {
      SSL_load_error_strings();
      SSL_library_init();

      c->sslContext = SSL_CTX_new(SSLv23_client_method());
      
      if(c->sslContext == NULL)
         ERR_print_errors_fp(stderr);

      c->sslHandle = SSL_new(c->sslContext);
      if(c->sslHandle == NULL)
         ERR_print_errors_fp(stderr);

      if(!SSL_set_fd(c->sslHandle, c->socket))
         ERR_print_errors_fp(stderr);

      if(SSL_connect(c->sslHandle) != 1)
         ERR_print_errors_fp(stderr);
   }
   else
      perror("Connect failed");

   return c;
}

void ssl_disconnect(connection *c)
{
   if(c->socket)
      close(c->socket);
   
   if(c->sslHandle)
   {
      SSL_shutdown(c->sslHandle);
      SSL_free(c->sslHandle);
   }

   if(c->sslContext)
      SSL_CTX_free(c->sslContext);

   free(c);
}

void ssl_write(connection *c, void *text, int len)
{
   if(c)
      SSL_write(c->sslHandle, text, len);
}

int s_send(int sockfd, void *datagram, int len, int flags)
{
  int chars_sent;
  if ((chars_sent = send(sockfd, datagram, len, flags)) == -1)
    {
      if (SAFE_CALL_PERROR)
	perror("s_send_pkt");
      if (SAFE_CALL_EXIT)
	exit(EXIT_FAILURE);
    }
  return chars_sent;
}

int s_socket(int domain, int type, int protocol)
{
  int sockfd;
  if((sockfd = socket(domain, type, protocol)) == -1)
    {
      if (SAFE_CALL_PERROR)
	perror("s_socket");
      if (SAFE_CALL_EXIT)
	exit(EXIT_FAILURE);
    }
  return sockfd;
}

int s_connect(int sockfd, const struct sockaddr *addr,
	      socklen_t addrlen)
{
  int ret_val = 0;
  if((ret_val = connect(sockfd, addr, addrlen)) == -1)
    {
      if (SAFE_CALL_PERROR)
	perror("s_connect error:");
      if (SAFE_CALL_EXIT)
	exit(EXIT_FAILURE);
    }
  return ret_val;
}

int s_inet_pton(int af, char *src, void *dst)
{
  int ret_val;
  if((ret_val = inet_pton(af, (const char *)src, dst)) <= 0)
    {
      if (ret_val == -1)
	{
	  if (SAFE_CALL_PERROR)
	    perror("s_connect error:");
	  if (SAFE_CALL_EXIT)
	    exit(EXIT_FAILURE);
	}
    }
  return ret_val;
}

int setup_server(sock *Sock, char *ip_address, int port, log *Log)
{
  socklen_t sock_len;
  bzero(&(Sock->addr), sizeof(Sock->addr));
  
  /* needs safe call wrapper*/
  if(s_inet_pton(CONN_FAMILY, ip_address, &(Sock->addr.sin_addr)) > 0)
    {
      Sock->addr.sin_port = port;
      Sock->addr.sin_family = CONN_FAMILY;
    }
  else
    {
      write_log(Log, "Bad IP Address");
      return SETUP_SERVER_ERR;
    }
  
  /* Get Socket FD from Kernel */
  if((Sock->fd = s_socket(CONN_FAMILY, CONN_PROTOCOL, 0)) == -1) //why zero at end ?
    {
      write_log(Log, "Socket Error");
      return SETUP_SERVER_ERR;
    }
  
  /* Bind Socket */
  if(s_bind(Sock->fd, (struct sockaddr *)&(Sock->addr), sizeof(Sock->addr)) != 0)
    {
      write_log(Log, "Socket Binding Error");
      return SETUP_SERVER_ERR;
    }
  
  /* Get the Service port assigned by kernel */
   sock_len = sizeof(Sock->addr);
   s_getsockname(Sock->fd, (struct sockaddr *)&(Sock->addr), &sock_len);
   Sock->addr.sin_port = htons(Sock->addr.sin_port);
   
   /* Start Listening on Port */
   if(s_listen(Sock->fd, BACK_LOG) == -1)
     {
       write_log(Log, "Listening Error");
       return SETUP_SERVER_ERR;
     }

   return SETUP_SERVER_SUCCESS;
}


int s_listen(int sockfd, int backlog)
{
  int ret_val;
  if ((ret_val = listen(sockfd, backlog)) == 0)
    {
      return ret_val;
    }
  if (SAFE_CALL_PERROR)
    perror("s_listen error");
  if (SAFE_CALL_EXIT)
    exit(EXIT_FAILURE);

  return ret_val;
}

int s_bind(int sockfd, const struct sockaddr *addr,
	 socklen_t addrlen)
{
  int ret_val;
  if ((ret_val = bind(sockfd, addr, addrlen)) == 0)
    {
      return ret_val;
    }
  if (SAFE_CALL_PERROR)
    perror("s_listen error");
  if (SAFE_CALL_EXIT)
    exit(EXIT_FAILURE);

  return ret_val;
}

int s_getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  int ret_val;
  if ((ret_val = getsockname(sockfd, addr, addrlen)) == 0)
    {
      return ret_val;
    }
  if (SAFE_CALL_PERROR)
	perror("s_getsockname error");
  if (SAFE_CALL_EXIT)
    exit(EXIT_FAILURE);
  return ret_val;
}
