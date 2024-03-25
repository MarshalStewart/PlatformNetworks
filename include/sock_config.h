#ifndef _SOCK_CONFIG_H_
#define _SOCK_CONFIG_H_

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/un.h>

#define CLIENT_SIDE 0
#define SERVER_SIDE 1

#define MESSAGE_BUF_SIZE 1000
#define MAX_SERVER_MESSAGE_SIZE 256 

#define MAX_NUM_OF_SOCKS 3
#define MAX_NUM_OF_CLIENTS 10

#define INET4_ADDRSIZE INET_ADDRSTRLEN * 4
#define INET6_ADDRSIZE INET6_ADDRSTRLEN * 4

#define LOCAL_PORT_NUM 9003
#define TCP_PORT_NUM 9004
#define UDP_PORT_NUM 9005
#define INET_PORT_NUM 9006

typedef struct sockaddr_un sockaddr_un_t;
typedef struct sockaddr_in sockaddr_in_t;
typedef struct sockaddr_in6 sockaddr_in6_t;
typedef struct sockaddr sockaddr_t;
typedef int sock_id_t;

typedef enum {
    SOCK_NOT_OK = -1,
    SOCK_OK,
    SOCK_RECONNECTED,
    SOCK_FAILED_TO_SEND
} E_SOCK_STATUS;

typedef enum {
    E_LOCAL_SOCK = 0,
    E_TCP_SOCK,
    E_UDP_SOCK,
} E_APP_SOCK_TYPE;

typedef enum {
    E_IPV4_SOCK = 0,
    E_IPV6_SOCK,
} E_DOMAIN_TYPE;

typedef struct {
    bool is_server;
    bool is_connected;
    E_APP_SOCK_TYPE app_type;
    E_DOMAIN_TYPE domain;
    
    int type;
    int status;

    char *response;
    
    int listen_fd;
    socklen_t listen_len;
    void *listen_addr;

    char *addr_str;
    int port;
    
    int conn_fd;
    socklen_t conn_addr_len;
    sockaddr_t *conn_addr;
    int conn_num_bytes;
    void *conn_buff;

    int listen_opt;
    
} sock_config_t;

/* Initialize Socket
 *
 * Opens a connection of type, on addr:port. Specifying is server impacts send/receive. If a socket
 * is a server it will be always listening. It will not connect to the socket on send as the 
 * connection is already open. This option is stored internally, to simplify the API. Both server and
 * clients can interface with APIs the same. 
 * 
 * NOTE: Local port initialization still requires param and port, however it's ignored. addr represents path.
 */
extern sock_id_t initialize_sock( E_APP_SOCK_TYPE type, const char *addr, int port, bool is_server );

/* Close Socket
 * 
 * Closes open connection and frees resources referenced by id.
 */
extern int close_sock( sock_id_t id );

/* Local APIs
 *
 * There are no network features enabled on a LOCAL socket.
 */
extern int await_local_receive( sock_id_t id, void *buffer, size_t len );
extern int await_local_send( sock_id_t *id, const void *buffer, size_t len );

/* TCP APIs
 * 
 * Supports both IP4 and IP6, whether IP4 or IP6 is used dependens on the type passed during 
 * initialization. On a failure to connect or accept, will re-initialize the socket at the same
 * id handler. This is a failure, and the send/receive must be attempted again. APIs return SOCK_OK on 
 * success, and SOCK_NOT_OK on failure. 
 */
extern int await_tcp_receive( sock_id_t id, void *buffer, size_t len );
extern int await_tcp_send( sock_id_t *id, const void *buffer, size_t len );


#endif // __SOCK_CONFIG_H_