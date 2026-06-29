#ifndef SCHEDULER_SHM_H
#define SCHEDULER_SHM_H

#include <sys/types.h>

#define MAX_JOBS 100

typedef struct 
{
    pid_t pid;
    char name[256];
    int state;   
    int run_time_slices;//tslices process has run
    int wait_time_slices;//tslices process has waited
} Job;

typedef struct 
{
    Job jobs[MAX_JOBS];
    int job_count;
    int ncpu;
    int tslice;//in ms
    int scheduler_running;//flag to control loop
} SharedData;

#define SHM_KEY 0x1234

#endif
