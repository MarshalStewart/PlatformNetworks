#ifndef _SUPPORT_H_
#define _SUPPORT_H_

#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

// defined in time.h
// #define CLOCKS_PER_SEC sysconf(_SC_CLK_TCK)

#define SCHEDULER_INTERVAL_1_MS 1
#define SCHEDULER_INTERVAL_10_MS 10
#define SCHEDULER_INTERVAL_50_MS 50
#define SCHEDULER_INTERVAL_500_MS 500
#define SCHEDULER_INTERVAL_1000_MS 1000

#define TASK_SCHEDULER_1MS_RATE      (SCHEDULER_INTERVAL_1_MS     * (CLOCKS_PER_SEC / 1000))
#define TASK_SCHEDULER_10MS_RATE     (SCHEDULER_INTERVAL_10_MS    * (CLOCKS_PER_SEC / 1000))
#define TASK_SCHEDULER_50MS_RATE     (SCHEDULER_INTERVAL_50_MS    * (CLOCKS_PER_SEC / 1000))
#define TASK_SCHEDULER_500MS_RATE    (SCHEDULER_INTERVAL_500_MS   * (CLOCKS_PER_SEC / 1000))
#define TASK_SCHEDULER_1000MS_RATE   (SCHEDULER_INTERVAL_1000_MS  * (CLOCKS_PER_SEC / 1000))

#define CONVERT_MS_TO_S(x)  ( (sec_t)x  / (sec_t)1000);
#define CONVERT_US_TO_MS(x) ((msec_t)x  / (msec_t)1000);
#define CONVERT_MS_TO_US(x) ((usec_t)x  * (usec_t)1000);

typedef clock_t sec_t;
typedef clock_t msec_t;
typedef clock_t  usec_t;

extern void set_start_time( void );
extern bool check_elasped_time( msec_t elapsed_time );
extern msec_t get_elasped_time( void );
extern bool start_timer( void );
extern msec_t stop_timer( void );

/***************************************************************************//**
 * Delay in milliseconds 
 *
 * This function is blocking, will suspend for specified ms. 
 *
 * @param ms Amount of time to sleep in ms.
 ******************************************************************************/
extern void delay_ms(msec_t sleep_time);

#endif // _SUPPORT_H_