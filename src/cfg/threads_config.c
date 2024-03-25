#include "threads_config.h"

/* Number of allocated pipes */
static int num_allocated_pipes;

/* Number of dead pipes that exist in the linked-list */
static int num_dead_pipes;

/* Linked-List of pipefd[] */
static node_t *head;

/* Locking mechanism to prevent potential forks applications from getting out of sync */
static bool pipes_locked;

/* Allocate a pipe and return id
 * 
 * Creates a pipe in the stored linked-list. The linked-list is ordered, and must be
 * sorted after every deletion. This function assumes that the linked-list is already organized. 
 * Will only allocate up to MAX_NUM_OF_PIPES pipefd. Returns THREAD_NOT_OK if unable to create a new
 * pipefd. Returns pipe_id_t of newly created pipefd on success.
 */
pipe_id_t create_pipe( void ) {
    pipe_id_t id = 0;
    node_t *cur;
    node_t *prv;

    if (pipes_locked) { return THREAD_NOT_OK; }

    if (head) {
        /* Linked-list isn't empty */
        prv = head;
        cur = head->nxt;

        /* Search for next open pipefd */
        for (id=1; id < MAX_NUM_OF_PIPES; id++) {
            if (cur) {

                /* Dead pipes
                 * 
                 * When a pipe is deleted, the linked-list doesn't free the node unless that node
                 * exists at the end of the linked-list. This results in "dead pipes", these nodes 
                 * do not have allocated pipes, but are still required to maintain app IDs and navigating 
                 * the one directional linked-list. When a dead pipe is encounted, that already allocated 
                 * node will be utilized.
                 */
                if (!cur->pipfd) {
                    if (pipe(cur->pipfd) == -1) {
                        /* Inform app that a pipe wasn't created */
                        id = THREAD_NOT_OK;
                        return id;
                    } else {
                        num_dead_pipes--;
                        return id;
                    }                    
                } else {
                    /* alive pipe */
                    prv = cur;
                    cur = cur->nxt;
                }

            } else {
                /* Found open pipefd at the end of the list. Free node */
                cur = malloc(sizeof(node_t));

                prv->nxt = cur;

                if (pipe(cur->pipfd) == -1) {
                    id = THREAD_NOT_OK;
                    free(cur);                
                } else {
                    num_allocated_pipes++;
                }

                return id;
            }
        }

        /* Not possible */
        return THREAD_NOT_OK;

    } else {
        /* Linked-list is empty */
        head = malloc(sizeof(node_t));

        /* Create pipe
         *
         * Creates a pipe, a unidirectional data channel that can be used for interprocess communication (IPC). The array pipefd is used to return two file descriptors referring to the ends of the pipe. pipfd[2] refres to the read end of the pipe. pipefd[1] refers to the write end of the pipe. Data written to the write end of the pipe is buffered by the kernel until it is read from the read end of the pipe. Returns 0 on success, pipe() doesn't modify pipefd on failure. On failure returns -1.
         *
         */
        if (pipe(head->pipfd) == -1) {
            /* Inform app that a pipe wasn't created */
            id = THREAD_NOT_OK;

            /* Free allocated memory */
            free(head);

        } else {
            num_allocated_pipes++;
        }

        return id;
    }
}

/* Free pipe from linked-list
 *
 * Frees pipefd at id if pipefd exists. Re-organizes linked-list after a pipefd is freed. Will
 * re-organize up to MAX_NUM_OF_PIPES times. This is a slow process and isn't intended for pipes 
 * to be freed very often. Returns THREAD_NOT_OK on failure, returns THREAD_OK on success. 
 */
int free_pipe ( pipe_id_t id ) {
    pipe_id_t cur_id;

    /* Pointers required for re-organizing linked-list */
    node_t *cur;
    //node_t *prv;
    node_t *nxt;
    
    if (pipes_locked) { return THREAD_NOT_OK; }

    /* ID cannot be larger than the number of allocated pipes */
    if (id >= num_allocated_pipes) { return THREAD_NOT_OK; }
    if (!head) { return THREAD_NOT_OK; }

    //prv = NULL;    
    cur = head;
    nxt = head->nxt;

    /* Navigate linked-list 
     * 
     * Navigate linked-list, always iterates (id) iterations. If nxt is ever NULL, this means
     * that the linked-list is broken. When the linked-list is broken, this is reported to the 
     * application. This is a fatal error, as all threads APIs will not leave the linked-list in 
     * this state. If id==0, head is the node to be deleted, therefore this for loop with never iterate. 
     */
    for (cur_id=1; cur_id <= id; cur_id++ ) {

        if (nxt) {
            //prv = cur;
            cur = nxt;
            nxt = nxt->nxt;
        } else {
            /* nxt will never be NULL entering an iteration of this for-loop. nxt can be null exiting the 
             * loop, through the first conditional branch. 
             * TODO: ERROR handling when linked-list is broken
             */
            return THREAD_NOT_OK;
        }
    }

    /* Organizing the linked-list
     * 
     * Expect nxt to be NULL, if end of list. Broken linked-list detected during navigating linked-list. cur 
     * will hold the ptr to the node represented by id. If nxt is not NULL, then all elements after ID must 
     * be shifted down in the list. Since id is used in the application, and won't be updated, a node removed
     * from the middle of list, will result in a DEAD node. This node still holds pts to the next member of
     * the linked-list. Only the last member of a linked-list can be removed.
     */
    if (nxt) {
        close(cur->pipfd[READ_END_OF_PIPE]);
        close(cur->pipfd[WRITE_END_OF_PIPE]);
        num_dead_pipes++;

    } else {
        /* ID is the end of the list, therefore we can free the resource and decrement the number of allocated
         * linked-lists. Must close pipefd before free, or else the pipes will still exist until the process is
         * complete. */
        close(cur->pipfd[READ_END_OF_PIPE]);
        close(cur->pipfd[WRITE_END_OF_PIPE]);
        free(cur);
        num_allocated_pipes--;
    }    
    
    return THREAD_OK;
}

/* Write data to pipe (non-blocking) 
 *
 * Will navigate linked-list for pipefd, then write to the pipefd. This is a non-blocking process, 
 * It's possible that zero bytes are written. On error returns THREAD_NO_OK, on success will return
 *  the number of bytes written.
 */
int write_pipe( pipe_id_t id, void *buffer, size_t len ) {

    pipe_id_t cur_id;
    node_t *cur;
    int write_pipefd;
    size_t num_bytes;
    
    if (!buffer) { return THREAD_NOT_OK; }

    /* ID cannot be larger than the number of allocated pipes */
    if (id >= num_allocated_pipes) { return THREAD_NOT_OK; }
    if (!head) { return THREAD_NOT_OK; }

    /* linked-list isn't empty */
    cur = head;

    for ( cur_id=0; cur_id < id; cur_id++ ) {
        if (cur->nxt) {
            cur = cur->nxt;
        } else {
            return THREAD_NOT_OK;
        }
    }

    write_pipefd = cur->pipfd[WRITE_END_OF_PIPE];

    // Set write end of pipe to non-blocking mode
    fcntl(write_pipefd, F_SETFL, O_NONBLOCK);

    if ((num_bytes = write(write_pipefd, buffer, len)) < 0) {
        return THREAD_NOT_OK;
    }

    return num_bytes;
}

/* Read data from pipe (non-blocking) 
 *
 * Will navigate linked-list for pipefd, then read from the pipefd. If the
 * data recieved is larger than the len provided, only len will be written. 
 * If the number of bytes received is less than len, only the number of bytes
 * received will be written. This is a non-blocking process, if the other process
 * hasn't updated the write side of the pipefd, then the data previously stored in 
 * the pipe will be read. It's possible for zero bytes are read. On error returns
 * THREAD_NO_OK, on success will return the number of bytes read.
 * 
 * NOTE: For applications that buffer is of type char[], it's possible for a null-terminating
 * character '\0' to not be written, if len < number of bytes read.
 */
int read_pipe ( pipe_id_t id, void *buffer, size_t len ) {

    pipe_id_t cur_id;
    node_t *cur;
    void *temp_buffer;
    int read_pipefd;
    size_t num_bytes;

    if (!buffer) { return THREAD_NOT_OK; }

    /* ID cannot be larger than the number of allocated pipes */
    if (id >= num_allocated_pipes) { return THREAD_NOT_OK; }
    if (!head) { return THREAD_NOT_OK; }

    /* linked-list isn't empty */
    cur = head;

    for ( cur_id=0; cur_id < id; cur_id++ ) {
        if (cur->nxt) {
            cur = cur->nxt;
        } else {
            /* TODO: error handling when linked-list is broken. cur->nxt cannot be NULL. */
            return THREAD_NOT_OK;
        }
    }
    
    read_pipefd = cur->pipfd[READ_END_OF_PIPE];

    // Set read end of pipe to non-blocking mode
    fcntl(read_pipefd, F_SETFL, O_NONBLOCK);
    
    if ((temp_buffer = malloc(len)) == NULL) {
        return THREAD_NOT_OK;
    }
    
    if ((num_bytes = read(read_pipefd, temp_buffer, len)) < 0) {
        free(temp_buffer);
        return THREAD_NOT_OK;
    }
    
    /* Update application buffer */
    if (num_bytes > len) {
        (void)memcpy(buffer, temp_buffer, len);
    } else {
        (void)memcpy(buffer, temp_buffer, num_bytes);
    }
    
    free(temp_buffer);

    return num_bytes;
}

/* Lock protection of linked-list
 *
 * If the APIs are used with features such as fork(), then the child and parent process will
 * be out of sync if the linked-list is modified. lock_pipes() prevents any further modifications 
 * to the linked-list until unlock_pipes() is called. pipefd are still functional during a r/w. After
 * a fork() process is complete, the linked-list can be unlocked.
 */
int lock_pipes ( void ) {
    pipes_locked = true;
    return THREAD_OK;
}

int unlock_pipes ( void ) {
    pipes_locked = false;
    return THREAD_OK;
}


