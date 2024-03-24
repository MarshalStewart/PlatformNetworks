#ifndef _FORK_CFG_H_
#define _FORK_CFG_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>

#define READ_END_OF_PIPE 0
#define WRITE_END_OF_PIPE 1

#define MAX_NUM_OF_PIPES 5

typedef int pipe_id_t;

typedef enum {
    THREAD_NOT_OK = -1,
    THREAD_OK,
    THRED_BROKEN_LINKED_LIST
} E_THREAD_STATUS;

typedef struct  {
    int pipfd[2];
    void *nxt;
    pipe_id_t id;
} node_t;

extern pipe_id_t create_pipe( void );
extern int free_pipe ( pipe_id_t id );
extern int write_pipe( pipe_id_t id, void *buffer, size_t len );
extern int read_pipe ( pipe_id_t id, void *buffer, size_t len );
extern int lock_pipes ( void );
extern int unlock_pipes ( void );

#endif // _FORK_CFG_H_