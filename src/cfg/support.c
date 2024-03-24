#include "support.h"

static usec_t start_time;

static bool timer_lock = false;
static usec_t timer;

void set_start_time( void ) {
    start_time = clock();
}

bool check_elasped_time( msec_t elapsed_time ) {
   
    usec_t curr_time = clock();
    usec_t delta_time = 0;

    /* Overflow protection */
    if ( start_time > curr_time ) {
        delta_time = (usec_t)-1 - start_time + curr_time + 1;
    } else {
        delta_time = curr_time - start_time;
    }

    return (delta_time > elapsed_time);
}

msec_t get_elasped_time( void ) {
    usec_t curr_time = clock();
    usec_t delta_time = 0;
    
    /* Overflow protection */
    if ( start_time > curr_time ) {
        delta_time = (usec_t)-1 - start_time + curr_time + 1;
    } else {
        delta_time = curr_time - start_time;
    }

    return delta_time;    
}

/* TODO: something weird is going on with the elasped time tracking */
bool start_timer( void ) {
    if (timer_lock == true) {return false;}
    timer = clock();
    return true;
}

msec_t stop_timer( void ) {
    clock_t end_time = clock();
    double cpu_time_used = ((double)(end_time - timer)) / CLOCKS_PER_SEC;

    printf("CPU time used: %.2f seconds\n", cpu_time_used);

    return 0;
}

void delay_ms(msec_t sleep_time) {
    struct timespec ts;
    ts.tv_sec = sleep_time / 1000;
    ts.tv_nsec = (sleep_time % 1000) * 1000000;
    nanosleep(&ts, NULL);
}
