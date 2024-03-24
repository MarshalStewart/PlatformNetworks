#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "server.h"
#include "sock_config.h"
#include "support.h"
#include "threads_config.h"

static sock_id_t id;
static pid_t child_pid;
static pipe_id_t parent_to_child;
static pipe_id_t child_to_parent;

const char my_sock[] = "/tmp/my_socket";

void int_handler(int __attribute__((unused)) sigType) {
    if (child_pid > 0) {
        fprintf(stderr, "Closing server\n");
        (void)close_sock(id);
    }
    exit(EXIT_FAILURE);
}

/* Child process isn't blocked waiting for socket, can perform background tasks */
void child_process() {
    int counter = 0;

    // if ((id = initialize_sock(E_INET_SOCK, UDP_PORT_NUM, SERVER_SIDE)) < 0) {
    //     printf("Failed to start socket\n");
    //     return -1;
    // }

    /* TODO: can turn a child process into a a port handler, aka a UDP server. The parent can be responsible for 
     * managing the application layer. So what does the server actually do based on the connection. So the child 
     * send hey we have an open connection here, and wait for the parent to respond. Basically be the gatekeep, while
     * the actually application memory and code is in an entirely different process.
     */

    for (;;) {

        /* Maintains execution rate */
        if (check_elasped_time(TASK_SCHEDULER_1000MS_RATE)) {

            if ((write_pipe(child_to_parent, (void *)&counter, sizeof(counter))) > 0) {
                // printf("Child to parent: %d\n", counter);
            }
            
            set_start_time(); // Reset scheduler
        }

        counter++;

        fflush(stdout); // Flush the output buffer

    }    
}

int main( int argc, char *argv[] )
{
    char buffer[128] = "";
    int rc;
    int ipc_buffer;

    signal(SIGINT, int_handler);
    
    /* This can cause weird behavior as SIGPIPE is used in sockets */
    // signal(SIGPIPE, sigpipe_handler);

    if ((parent_to_child = create_pipe()) < 0) {
        printf("Failed to create parent_to_child pipe\n");
        return -1;
    }

    if ((child_to_parent = create_pipe()) < 0) {
        printf("Failed to create child_to_parent pipe\n");
        return -1;
    }

    if ((child_pid = fork()) == -1) {
        printf("Failed to fork process.\n");
        return -1;
    } else if (child_pid == 0) {
        child_process();
        return 0;
    } else {
        // Parent continue ...
    }    
        
    // if ((id = initialize_sock(E_LOCAL_SOCK, my_sock, 0, SERVER_SIDE)) < 0) {
    //     printf("Failed to start socket\n");
    //     return -1;
    // }
    
    if ((id = initialize_sock(E_IP4_SOCK, "127.0.0.1", LOCAL_PORT_NUM, SERVER_SIDE)) < 0) {
        printf("Failed to start socket\n");
        return -1;
    }

   /* Initialize scheduler */ 
    set_start_time();

    /* Task Scheduler */
    for (;;) {

        if (check_elasped_time(TASK_SCHEDULER_1000MS_RATE)) {

            // if ((rc = await_local_receive(id, buffer, sizeof(buffer))) >= 0) {
            //     printf("Buffer: %s\n", buffer);
            // }

            if ((rc = await_tcp_receive(id, buffer, sizeof(buffer))) >= 0) {
                printf("Buffer: %s\n", buffer);
            }            
            
            if ((read_pipe(child_to_parent, (void *)&ipc_buffer, sizeof(ipc_buffer))) > 0) {
                //printf("Parent read from child: %d\n", ipc_buffer);
            }

            set_start_time(); // Reset scheduler
        }
            
        fflush(stdout); // Flush the output buffer

    }

    return 0;
}
