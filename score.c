#include <stdio.h>
#include <time.h>
#include <math.h>
#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif
#include "limits.h"
#include "types.h"
#include "winning_engine.c"
//#include "engine.c"
#include "score_feed.h"

void feed(unsigned begin, unsigned end);
long long timediff(struct timespec start, struct timespec end);
void print_cpuaffinity();

/* This script provides indicative unofficial score. 
   Lowest score wins.

   Run Ubuntu 10.10 with the kernel option isocpus=1 
   and match "Detailed information about judging 
   platform", on the challenge webpage, to make this as 
   close to the judging environment as possible.
*/

void current_utc_time(struct timespec *ts) {
#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts->tv_sec = mts.tv_sec;
    ts->tv_nsec = mts.tv_nsec;
#else
    clock_gettime(CLOCK_MONOTONIC_RAW, ts);
#endif
}


void execution(t_execution exec) {};

int main(int argc, char *argv[]) {
    int msg_batch_size = 200;
    int replays = 2000;

    if (argc > 1) {
        msg_batch_size = atoi(argv[1]);
    }
    if (argc > 2) {
        replays = atoi(argv[2]);
    }
    int raw_feed_len = sizeof(raw_feed)/sizeof(t_order);
    int nbatches = raw_feed_len/msg_batch_size;
    int samples = replays * nbatches;

    printf("batches: %d\n", nbatches);
    printf("batch size: %d\n", msg_batch_size);
    printf("replays: %d\n", replays);
    printf("raw feed length: %d\n", raw_feed_len);
    printf("samples: %d\n", samples);

    long long sum = 0;
    long long squares = 0;

    struct timespec begin;
    struct timespec end;

    for (int j = 0; j < replays; j++) {
        init();
        for (int i = msg_batch_size; i < raw_feed_len; i += msg_batch_size) {
            current_utc_time(&begin);

            feed(i-msg_batch_size, i);

            current_utc_time(&end);

            long long dt = timediff(begin, end);
            sum += dt;
            squares += dt*dt;
        }
        destroy();
    }

    double latency_mean = sum / (double)samples; // mean latency per batch
    double latency_sd = sqrt(squares / (double)samples - latency_mean * latency_mean); // latency stddev per batch
    double score = 0.5 * (latency_mean + latency_sd) * nbatches;
    printf("Mean:     %5.1f μs\n", 1e-3 * latency_mean * nbatches);
    printf("Stddev:   %5.1f μs\n", 1e-3 * latency_sd * nbatches); // σ
    printf("QC score: %ld\n", (long long)score);
    printf("avg/tx:   %.2f ns\n", latency_mean / (double)msg_batch_size);
    //printf("avg/tx:   %.2f ns\n", latency_mean * nbatches / (double) raw_feed_len);
}

void feed(unsigned begin, unsigned end) {
    for(int i = begin; i < end; i++) {
        if (raw_feed[i].price == 0) {
            cancel(raw_feed[i].size);
        } else {
            limit(raw_feed[i]);
        }
    }
}

long long timediff(struct timespec start, struct timespec end) {
    struct timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return (temp.tv_sec*1000000000) + temp.tv_nsec;
}


// #define _GNU_SOURCE
// #include <sched.h>
//
// void print_cpuaffinity() {
// #ifndef __MACH__
//     cpu_set_t mask; /* processor 1 (0-indexed) */
//     unsigned int len = sizeof(mask);
//
//     if (sched_getaffinity(0, len, &mask) < 0) {
//         perror("sched_getaffinity");
//     }
//     printf("active cores: ");
//     for (int i = 0; i < 32; ++i)
//         if (CPU_ISSET(i, &mask))
//             printf("%d ", i);
//     printf("\n");
// #endif
// }
