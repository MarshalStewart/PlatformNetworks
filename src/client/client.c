#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "server.h"
#include "sock_config.h"
#include "support.h"
#include "threads_config.h"

static sock_id_t id;

const char my_sock[] = "/tmp/my_socket";

void int_handler(int __attribute__((unused)) sigType) {
    fprintf(stderr, "Closing client\n");
    close_sock(id);
    exit(EXIT_FAILURE);
}

/* Application Client Tasks */
void __attribute__((weak)) appClientTask10Ms( void ) {
    //printf("running app task\n");
}

static int server_service( void ) {

    char message[8] = "marsh";
    int rc;

    if ((rc = await_local_send(&id, message, 8)) < 0) {
        printf("Failed to send data to server\n");
        return -1;
    }

    // if ((rc = await_tcp_send(&id, message, 8)) < 0) {
    //     printf("Failed to send data to server\n");
    //     return -1;
    // }    

    return 0;
}

int main( int argc, char *agv[] ) {

    signal(SIGINT, int_handler);

    if ((id = initialize_sock(E_LOCAL_SOCK, my_sock, 0, CLIENT_SIDE)) < 0) {
        printf("Failed to get a socket.\n");
        return -1;
    }    

    // if ((id = initialize_sock(E_TCP_SOCK, "0.0.0.0", 9003, CLIENT_SIDE)) < 0) {
    //    printf("Failed to get a socket.\n");
    //    return -1;
    // }      
    
    // if ((id = initialize_sock(E_TCP_SOCK, "54.80.21.43", 9003, CLIENT_SIDE)) < 0) {
    //     printf("Failed to get a socket.\n");
    //     return -1;
    // }      

    // if ((id = initialize_sock(E_TCP_SOCK, "::", 9003, CLIENT_SIDE)) < 0) {
    //     printf("Failed to get a socket.\n");
    //     return -1;
    // }   

    // if ((id = initialize_sock(E_UDP_SOCK, "::", 9003, CLIENT_SIDE)) < 0) {
    //     printf("Failed to get a socket.\n");
    //     return -1;
    // }   

    /* Setup App Client Metrics */

    /* Initialize scheduler */ 
    set_start_time();

    msec_t task_counter_10ms    = 0;
    msec_t task_counter_500ms   = 0;
    msec_t task_counter_1000ms      = 0;

    /* Task Scheduler */
    for (;;) {

        // printf("Current elasped time: %ld\n", get_elasped_time());
        
        if (task_counter_10ms >= SCHEDULER_INTERVAL_10_MS) {
            // TODO: Record app code exection time
            // start_timer(); 
            appClientTask10Ms();
            // printf("Elasped time: %lu\n", stop_timer());
            task_counter_10ms = 0;
        }

        if (task_counter_500ms >= SCHEDULER_INTERVAL_500_MS) {
            (void)server_service();
            task_counter_500ms = 0;
        }

        if (task_counter_1000ms >= SCHEDULER_INTERVAL_1000_MS) {
            // printf("1s task\n");
            task_counter_1000ms = 0;
        }
        
        /* Maintains execution rate */
        if (check_elasped_time(TASK_SCHEDULER_1MS_RATE)) {
            // increment counters
            task_counter_10ms++;
            task_counter_500ms++; 
            task_counter_1000ms++;
            set_start_time(); // Reset scheduler
        }

        fflush(stdout); // Flush the output buffer
    }

    return 0;
}
