#include "sock_config.h"

static sock_config_t *sock_configs[MAX_NUM_OF_SOCKS];

/* Static Functions */
static sock_id_t _initialize_tcp_sock( int domain, int type, const char *addr, int port, bool is_server);
static sock_id_t _initialize_local_sock( int domain, int type, const char *path, bool is_server);
static int _find_open_sock( void );

/* Initialize a sock connection, configuration
 *
 * This is the configuration handler for intializing a socket, function will handle setting proper 
 * domains, families, etc. Performs a sanity check on the provided address. Address must be
 * of the exact requirements of that socket type. On error returns SOCK_NOT_OK, on success returns
 * id of socket to be used as a handle for the socket.
 * 
 * NOTE: When using a LOCAL socket, address and port are ignored.
 */
sock_id_t initialize_sock(E_APP_SOCK_TYPE app_type, const char *addr, int port, bool is_server) {
    sock_id_t id = SOCK_NOT_OK;
    switch (app_type) {
        case E_LOCAL_SOCK:
            if ((id = _initialize_local_sock(AF_LOCAL, SOCK_STREAM, addr, is_server)) < 0) {
                return SOCK_NOT_OK;
            }
            sock_configs[id]->app_type = app_type;
            break;
        case E_IP4_SOCK:
            if ((id = _initialize_tcp_sock(AF_INET, SOCK_STREAM, addr, port, is_server)) < 0) {
                return SOCK_NOT_OK;
            }
            sock_configs[id]->app_type = app_type;
            break;
        case E_IP6_SOCK:
            if ((id = _initialize_tcp_sock(AF_INET6, SOCK_STREAM, addr, port, is_server)) < 0) {
                return SOCK_NOT_OK;
            }
            sock_configs[id]->app_type = app_type;
            break;
        case E_UDP_SOCK:
        default:
            break;
    }

    return id;
}

/* Initialize a sock connection
 * 
 * The following will initialize a sock with the given domain, type, family, address, and port. Functionality
 * will vary depending on if the sock is of type server or client. 
 * 
 * To accept connections, the following steps are performed
 * 1. A sock is created with socket()
 * 2. The sock is bound to a local address using bind(), so that other sockets may be connect()ed to it.
 * 3. A willingness to accept incoming connections and a queue limit for incoming connections are specified 
 *      with listen().
 * 4. Connections are accepted with accept()
 */
static sock_id_t _initialize_tcp_sock( int domain, int type, const char *addr, int port, bool is_server ) {
    sock_config_t *sock_cfg;
    sockaddr_in_t *listen_addr;
    
    int open_sock_id = SOCK_NOT_OK;
    int status = SOCK_NOT_OK;

    if ((open_sock_id = _find_open_sock()) == SOCK_NOT_OK) {
        return SOCK_NOT_OK;
    }
    
    sock_cfg = sock_configs[open_sock_id] = malloc(sizeof(sock_config_t));
    sock_cfg->conn_addr = malloc(sizeof(sockaddr_t));
    sock_cfg->conn_addr_len = sizeof(sock_cfg->conn_addr);
    sock_cfg->conn_buff = malloc(MAX_SERVER_MESSAGE_SIZE * sizeof(char));
    
    sock_cfg->is_server = is_server;
    sock_cfg->type = type;

    sock_cfg->listen_addr = malloc(sizeof(sockaddr_in_t));
    listen_addr = (sockaddr_in_t *)sock_cfg->listen_addr;
    sock_cfg->listen_len = sizeof(*listen_addr);
    listen_addr->sin_family = domain;
    listen_addr->sin_port = htons(port);
    // sock_cfg->listen_addr->sin_addr.s_addr = s_addr; 
    sock_cfg->listen_opt = 1;

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(domain, addr, &listen_addr->sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        printf("addr: %s\n", addr);
        return SOCK_NOT_OK;
    }        

     /* Create an endpoint for communication
      * 
      * Creates an endpoint for communication and returns a fd that is referred to as 
      * the endpoint. The fd will be lowest-numbered fd not currrently open for the 
      * process. The domain argument specifies a communication domain, this selects the 
      * protocol family. The sock has an indicated type, which specifies the 
      * communication semantics. Returns a fd for the new socket. On error, -1 is returned.
      * 
      */
    sock_cfg->listen_fd  = socket(domain, type, 0);
    if (sock_cfg->listen_fd < 0) {
        printf("Failed to get socket\n");
        close_sock(open_sock_id);
        return SOCK_NOT_OK;
    }

    if (is_server) {

        /* Bind a name to a sock
         * 
         * When a sock is created with socket(), it exists in a namespace, however no address
         * has been assigned to it. bind() assigns the address specified by addr to the sock 
         * referred to by the fd. Traditionally, this operation is called "assigning a name to
         * a sock". This is necessary before the socket can receive a connection. Rules of 
         * binding are dependent on the address families.
         */
        if ((status = bind(sock_cfg->listen_fd, 
                (const struct sockaddr *)listen_addr, sock_cfg->listen_len)) < 0) {

            printf("Failed to bind socket\n");
            close_sock(open_sock_id);
            return SOCK_NOT_OK;
        }
        
        /* Socket Options (Reuse Address)
         * To manipulate options at the socks API level, level is specified as SOL_SOCK. The
         * option SO_REUSEADDR will bypass the restrictions of the OS for an address already in use.
         * This is useful during development, as you don't want to wait for the OS to release the
         * address after restarting the server. This option should be removed in a production setting.
         */
        if ((status = setsockopt(sock_cfg->listen_fd, 
                SOL_SOCKET, SO_REUSEADDR, &sock_cfg->listen_opt, sizeof(sock_cfg->listen_opt))) < 0) {

            printf("Failed to set socket options\n");
            close_sock(open_sock_id);
            return SOCK_NOT_OK;
        }

        /* Listen for connection on a sock
         *
         * Marks the sock referred to by the fs, as a passive socket, that is, as a socket that
         * will be used to accept incoming connection requests using accept(). The backlog argument
         * defines the maximum length to which the queue of pending connections for fs may grow. If a
         * connection request arrives when the queue is full, the client may recieve an error that the
         * connection was refused. This behavior is dependent on the underlying protocol. Some may 
         * support retransmission. 
         * 
         */
        if ((status = listen(sock_cfg->listen_fd, MAX_NUM_OF_CLIENTS)) < 0){

            printf("Failed to listen on socket\n");
            close_sock(open_sock_id);
            return SOCK_NOT_OK;
        }

    }
 
    return open_sock_id;
}

/* Initialize Local Socket
 *
 * Initialize local socket to listen on any address. 
 */
static sock_id_t _initialize_local_sock( int domain, int type, const char* path, bool is_server ) {
    sock_config_t *sock_cfg;
    sockaddr_un_t *listen_addr;
    
    int open_sock_id = SOCK_NOT_OK;
    int status = SOCK_NOT_OK;
    size_t len;

    if (!path) { return SOCK_NOT_OK; }

    if ((open_sock_id = _find_open_sock()) == SOCK_NOT_OK) {
        return SOCK_NOT_OK;
    }
    
    sock_cfg = sock_configs[open_sock_id] = malloc(sizeof(sock_config_t));
    
    sock_cfg->conn_addr = malloc(sizeof(sockaddr_t));
    sock_cfg->conn_addr_len = sizeof(sock_cfg->conn_addr);
    sock_cfg->conn_buff = malloc(MAX_SERVER_MESSAGE_SIZE * sizeof(char));
    
    sock_cfg->is_server = is_server;
    sock_cfg->type = type;
    
    sock_cfg->listen_addr = malloc(sizeof(sockaddr_un_t));
    listen_addr = (sockaddr_un_t *)sock_cfg->listen_addr; 
    sock_cfg->listen_len = sizeof(*listen_addr);

    listen_addr->sun_family = domain;
    len = strlen(path) * sizeof(char);
    strncpy(listen_addr->sun_path, path, len);

    /* Enable options for sock descriptor */
    sock_cfg->listen_opt = 1;

    sock_cfg->listen_fd  = socket(domain, type, 0);
    if (sock_cfg->listen_fd < 0) {
        printf("Failed to get socket\n");
        close_sock(open_sock_id);
        return SOCK_NOT_OK;
    }

    if (is_server) {

        if ((status = bind(sock_cfg->listen_fd, 
                (struct sockaddr *)listen_addr, sock_cfg->listen_len)) < 0) {

            printf("Failed to bind socket\n");
            close_sock(open_sock_id);
            return SOCK_NOT_OK;
        }
        
        if ((status = setsockopt(sock_cfg->listen_fd, 
                SOL_SOCKET, (SO_REUSEADDR | SO_REUSEPORT), &sock_cfg->listen_opt, sizeof(int))) < 0) {

            printf("Failed to set socket options\n");
            close_sock(open_sock_id);
            return SOCK_NOT_OK;
        }

        if ((status = listen(sock_cfg->listen_fd, MAX_NUM_OF_CLIENTS)) < 0){

            printf("Failed to listen on socket\n");
            close_sock(open_sock_id);
            return SOCK_NOT_OK;
        }

    }
 
    return open_sock_id;
}

/* Await TCP Receive 
 *
 * Checks if there is already an open connection, if not will create a new connection via
 * accept(). When a message is received, will write to buffer, if the number of bytes received is
 * greater than len, only len bytes are written. On success will return SOCK_OK, on error will return 
 * SOCK_NOT_OK.
 * 
 * TODO: Mechanism to detect whether more bytes where received than can be written.
 */
int await_tcp_receive(sock_id_t id, void *buffer, size_t len) {
    sock_config_t *sock_cfg;
    
    if (id > MAX_NUM_OF_SOCKS) { return SOCK_NOT_OK; }
    if (buffer == NULL) { return SOCK_NOT_OK; }

    sock_cfg  = sock_configs[id];
    
    if (sock_cfg == NULL) { return SOCK_NOT_OK; }
    if ((sock_cfg->app_type != E_IP4_SOCK) && (sock_cfg->app_type != E_IP6_SOCK)) { return SOCK_NOT_OK; } 

    /* Clear buffer */ 
    memset(buffer, 0, len);

    /* Only perform this check after a valid connection has been made. Otherwise, 
    there will not be an error and no connection will be made. */
    if ( sock_cfg->is_connected ) {
        int error = 0;
        socklen_t len = sizeof(error);

        /* Get options on socket 
        *
        * The purpose of this check is to determine if there is already on open connection
        * on the socket. If connect() or accept() are called consecutively on the same socket,
        * the process will get a negative return code.
        */
        if (getsockopt(sock_cfg->listen_fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
            return SOCK_NOT_OK;
        }

        if (error == 0) {
            // sock_cfg->is_connected = 1;
        } else {
            sock_cfg->is_connected = 0;
        }        
    }

    if ( !sock_cfg->is_connected ) {
        /* Accepting new connection
        * Extracts the first connection request on the queue. Creates a new connected sock, returns fd
        * for that sock. Original socket is unaffected. The newly created socket is not in the listening
        * state. Requires listening sock, addrlen is size of incoming address, must be initialized with
        * size of addr.
        * 
        * If there are no pending connections present in the queue, will be blocking, unless the sock
        * was marked as non-blocking. If marked as non-blocking and no pending connections, will fail.
        * 
        */
        sock_cfg->conn_fd = accept(sock_cfg->listen_fd, sock_cfg->conn_addr, &sock_cfg->conn_addr_len);

        if (sock_cfg->conn_fd < 0) {
            printf("Failed to accept connection.\n");
            return -1;
        }

        sock_cfg->is_connected = true;
    }

    /* Receive a message from a sock 
     *
     * Used to receive messages from a sock. They may be used to recieve data on both connectionless and
     * connection-oriented socks. Return the length of the message on successful completion. If no messages are 
     * available at the sock, the receive calls wait for a message to arrive, unless the socket is nonblocking, in 
     * which case the value -1 is returned and errno is updated. The receive calls normally return any data 
     * available, up to the requested amount, rather than waiting fo receipt of the full amount requested. Returns 
     * the number of bytes received, or -1 if an error occured.
     * 
     * An application can use select(), poll(), or epoll() to determine when more data arrives on a sock.
     * 
     * There exists recv(), recvmsg(), and recvfrom(). recv() works in this simple application
     * 
     * The only difference between recv() and read() is the presence of flags. 
     *
     */
    sock_cfg->conn_num_bytes = recv(sock_cfg->conn_fd, sock_cfg->conn_buff, sizeof(sock_cfg->conn_buff), 0);
    
    if (sock_cfg->conn_num_bytes > 0) {

        // printf("Number of bytes receieved: %d\n", sock_cfg->conn_num_bytes);
        // printf("Received buffer : %s\n", sock_cfg->conn_buff);
        
        /* Update application buffer */
        if (sock_cfg->conn_num_bytes > len) {
            (void)memcpy(buffer, sock_cfg->conn_buff, len);
        } else {
            (void)memcpy(buffer, sock_cfg->conn_buff, sock_cfg->conn_num_bytes);
        }

        return SOCK_OK;

    } else {
        return SOCK_NOT_OK;
    }

}

/* Await local receive
 *
 * There is no diffrerence between local and tcp receive APIs
 */
int await_local_receive(sock_id_t id, void *buffer, size_t len) {
    sock_config_t *sock_cfg;
    
    if (id > MAX_NUM_OF_SOCKS) { return SOCK_NOT_OK; }
    if (buffer == NULL) { return SOCK_NOT_OK; }

    sock_cfg  = sock_configs[id];
    
    if (sock_cfg == NULL) { return SOCK_NOT_OK; }
    if ((sock_cfg->app_type != E_LOCAL_SOCK)) { return SOCK_NOT_OK; }

    /* Clear buffer */ 
    memset(buffer, 0, len);

    if ( sock_cfg->is_connected ) {
        int error = 0;
        socklen_t len = sizeof(error);

        if (getsockopt(sock_cfg->listen_fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
            return SOCK_NOT_OK;
        }

        if (error == 0) {
            // sock_cfg->is_connected = 1;
        } else {
            sock_cfg->is_connected = 0;
        }        
    }

    if ( !sock_cfg->is_connected ) {
        sock_cfg->conn_fd = accept(sock_cfg->listen_fd, sock_cfg->conn_addr, &sock_cfg->conn_addr_len);

        if (sock_cfg->conn_fd < 0) {
            printf("Failed to accept connection.\n");
            return -1;
        }

        sock_cfg->is_connected = true;
    }

    sock_cfg->conn_num_bytes = recv(sock_cfg->conn_fd, sock_cfg->conn_buff, sizeof(sock_cfg->conn_buff), 0);
    
    if (sock_cfg->conn_num_bytes > 0) {

        /* Update application buffer */
        if (sock_cfg->conn_num_bytes > len) {
            (void)memcpy(buffer, sock_cfg->conn_buff, len);
        } else {
            (void)memcpy(buffer, sock_cfg->conn_buff, sock_cfg->conn_num_bytes);
        }

        return SOCK_OK;

    } else {
        return SOCK_NOT_OK;
    }
}

int await_tcp_send(sock_id_t *id, const void *buffer, size_t len) {
    sock_config_t *sock_cfg;
    sockaddr_in_t *listen_addr;
    
    if (id == NULL) { return SOCK_NOT_OK; }
    if (*id > MAX_NUM_OF_SOCKS) { return SOCK_NOT_OK; }
    if (buffer == NULL) { return SOCK_NOT_OK; }
    
    sock_cfg = sock_configs[*id];

    if (sock_cfg == NULL) { return SOCK_NOT_OK; }
    if ((sock_cfg->app_type != E_IP4_SOCK) && (sock_cfg->app_type != E_IP6_SOCK)) { return SOCK_NOT_OK; }

    /* Server side doesn't connect to socket, as it's already passively listening. */
    if ((!sock_cfg->is_server)) {
        
        /* Only perform this check after a valid connection has been made. Otherwise, 
        there will not be an error and no connection will be made. */
        if ( sock_cfg->is_connected ) {
            int error = 0;
            socklen_t len = sizeof(error);

            /* Get options on socket 
            *
            * The purpose of this check is to determine if there is already on open connection
            * on the socket. If connect() or accept() are called consecutively on the same socket,
            * the process will get a negative return code.
            */
            if (getsockopt(sock_cfg->listen_fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
                printf("Failed to get socket options\n");
                return SOCK_NOT_OK;
            }

            if (error == 0) {
                // sock_cfg->is_connected = 1;
            } else {
                sock_cfg->is_connected = 0;
            }        
        }
 
        /* Initiate a connection on a sock 
        *
        * Connects the sock referred to by the fs to the address specified by addr. The format
        * of the address is determined by the configuration of the sock. If the connection succeeds, 
        * zero is returned. On error, -1 is returned. If connection fails, consider the state of the sock
        * as unspecified. Protable applications should close the sock and crete a new one for reconnecting.
        * 
        */
        if (!sock_cfg->is_connected) {
            if ((sock_cfg->status = connect(sock_cfg->listen_fd, (const sockaddr_t *)sock_cfg->listen_addr, sizeof(sockaddr_t))) < 0) {

                listen_addr = (sockaddr_in_t *)sock_cfg->listen_addr;

                /* TODO: useful standard printout message with what happened, ID, addr, port, etc */
                printf("Socket ID(%d) failed to connect. \n", *id);
                
                E_APP_SOCK_TYPE app_type = sock_cfg->app_type;
                bool is_server = sock_cfg->is_server;
                /* Port is stored in network bytes order, app provides host byte order */
                int port = ntohs(listen_addr->sin_port);
                
                int af;
                socklen_t size;
                char *addr_str;
                
                /* Get current address */
                switch(sock_cfg->app_type) {
                    case E_IP4_SOCK:
                        af = AF_INET;
                        size = INET_ADDRSTRLEN;
                        break;
                    case E_IP6_SOCK:
                        af = AF_INET6;
                        size = INET6_ADDRSTRLEN;
                        break;
                    case E_UDP_SOCK:
                    default:
                        /* Not supported */
                        return SOCK_NOT_OK;
                }
                        
                addr_str = malloc(size * sizeof(char));

                /* Convert IPv4 and IPv6 addresses from binary to text form */
                if(inet_ntop(af, &listen_addr->sin_addr, addr_str, size) <= 0) {
                    printf("\nInvalid address/ Address not supported \n");
                    return SOCK_NOT_OK;
                }        

                /* Failed to connect to sock, closing socket, and reconnecting */
                close_sock(*id);

                /* Application is given update id for sock, informed that socket was reconnected */
                *id = initialize_sock(app_type, addr_str, port, is_server);

                free(addr_str);
            
                sock_cfg->is_connected = false;
                
                return SOCK_NOT_OK;
            }

            sock_cfg->is_connected = true;
        }
    }

    /* Send a message on a sock 
     *
     * Transmit a message to another sock. send() can only be used hwen the socket is in a connected state (so that the intended 
     * recient is known). The only difference between send() and write() is the presence of flags. fd is of the sending sock. The 
     * message to send is found in buf and has length len. On success, returns the number of bytes sent. On error, -1 is returned. If
     * the message does not fit into the send buffer of the sock, send() blocks. Unless the socket is in nonblocking I/O mode.
     */
    if ((sock_cfg->conn_num_bytes = send(sock_cfg->listen_fd, buffer, len, 0)) < 0) {
        printf("Failed to send\n");
        return SOCK_NOT_OK;
    }

    printf("Sent %d bytes\n", sock_cfg->conn_num_bytes);
        
    return SOCK_OK;
}

/* Await Local Send
 * 
 */
int await_local_send( sock_id_t *id, const void *buffer, size_t len ) {
    sock_config_t *sock_cfg;
    sockaddr_un_t *listen_addr;
    char *path;
    
    if (id == NULL) { return SOCK_NOT_OK; }
    if (*id > MAX_NUM_OF_SOCKS) { return SOCK_NOT_OK; }
    if (buffer == NULL) { return SOCK_NOT_OK; }
    
    sock_cfg = sock_configs[*id];

    if (sock_cfg == NULL) { return SOCK_NOT_OK; }
    if ((sock_cfg->app_type != E_LOCAL_SOCK)) { return SOCK_NOT_OK; }

    /* Server side doesn't connect to socket, as it's already passively listening. */
    if ((!sock_cfg->is_server)) {
                
        listen_addr = (sockaddr_un_t *)sock_cfg->listen_addr;
        
        if ( sock_cfg->is_connected ) {
            int error = 0;
            socklen_t len = sizeof(error);

            if (getsockopt(sock_cfg->listen_fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
                printf("Failed to get local socket options\n");
                return SOCK_NOT_OK;
            }

            if (error == 0) {
                // sock_cfg->is_connected = 1;
            } else {
                sock_cfg->is_connected = 0;
            }        
        }
 
        if (!sock_cfg->is_connected) {
            if ((sock_cfg->status = connect(sock_cfg->listen_fd, 
                (const sockaddr_t *)listen_addr, sock_cfg->listen_len)) < 0) {

                size_t len;
                bool is_server = sock_cfg->is_server;

                len = strlen(listen_addr->sun_path) * sizeof(char);
                path = malloc(len);
                strncpy(path, listen_addr->sun_path, len);

                /* TODO: useful standard printout message with what happened, ID, addr, port, etc */
                printf("Local socket ID(%d) failed to connect. \n", *id);
                
                /* Failed to connect to sock, closing socket, and reconnecting */
                close_sock(*id);

                /* Application is given update id for sock, informed that socket was reconnected */
                *id = initialize_sock(E_LOCAL_SOCK, path, 0, is_server);
            
                sock_cfg->is_connected = false;
                free(path);
                
                return SOCK_NOT_OK;
            }

            sock_cfg->is_connected = true;
        }
    }

    if ((sock_cfg->conn_num_bytes = send(sock_cfg->listen_fd, buffer, len, 0)) < 0) {
        printf("Failed to send\n");
        return SOCK_NOT_OK;
    }

    printf("num bytes: %d\n", sock_cfg->conn_num_bytes);

    return SOCK_OK;
}

/* Close a socket
 *
 * Performs check if id is valid. Will always attempt to close the socket. A failure of close() is
 * ignored. All resources in sock_cfg are freed. Returns SOCK_OK on success, and SOCK_NOT_OK on failure.
 */
int close_sock(sock_id_t id) {
    sock_config_t *sock_cfg;
    size_t len;
    char *path;
    
    if (id > MAX_NUM_OF_SOCKS) { return SOCK_NOT_OK; }
    
    sock_cfg = sock_configs[id];

    if (sock_cfg == NULL) { return SOCK_NOT_OK; }

    if (sock_cfg->app_type == E_LOCAL_SOCK) {
        len = strlen(((sockaddr_un_t *)sock_cfg->listen_addr)->sun_path) * sizeof(char);
        path = malloc(len);
        strncpy(path, ((sockaddr_un_t *)sock_cfg->listen_addr)->sun_path, len);
    }

    /* Close a file descriptor (fd)
     *
     * Closes a fd, so that it no longer refers to any file and may be reused. Any record locks held on
     * the file it was associated with, and owned by the process, are removed. Regardless of the fd that 
     * was used to obtain the lock.
     * 
     * If fd is the last fd referring to the underlying open fd, the resources associated with the open fd are freed; 
     * if the fd was the last reference to a file which has been removed using unlink(), the file is deleted.
     *
     * TODO: Assuming that failures are because that socket closed and path already unlinked. Can check errno to 
     * determine if this is the case. However, this isn't that important.
     */
    if (close(sock_cfg->listen_fd) < 0) {
        // Don't care since we want to close anyways
        //printf("Failed to close socket id: %d\n", id);
    }

    /*
     * Ensure the server is running: The server must be running and have already created the Unix domain socket file  
     * at the specified path before the client attempts to connect. If the server hasn't started or hasn't created the 
     * socket file yet, the client will receive an ENOENT error.
     */
    if ((sock_cfg->app_type == E_LOCAL_SOCK) && sock_cfg->is_server ) {
        if (unlink(path) < 0) {
            // Don't care since we want to de-link anyways
            // printf("Failed to unlink path: %s\n", path);
        }
        free(path);
    }

    if (sock_cfg->conn_buff)   { free(sock_cfg->conn_buff);   } 
    if (sock_cfg->conn_addr)   { free(sock_cfg->conn_addr);   }
    if (sock_cfg->listen_addr) { free(sock_cfg->listen_addr); }
    free(sock_cfg);

    sock_configs[id] = NULL;

    return SOCK_OK;
}

/* Searches available socks for an open buffer */
static int _find_open_sock( void ){
    for (int i=0; i<MAX_NUM_OF_SOCKS; i++) {
        if (sock_configs[i] == NULL) {
            return i;
        }
    }
    return SOCK_NOT_OK;
}