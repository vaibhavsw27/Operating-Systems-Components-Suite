//Vaibhav 2024597 OS BONUS PROJECT

#pragma once

#include "egos.h"
#include "syscall.h"

enum proc_status {
    PROC_UNUSED,
    PROC_LOADING,
    PROC_READY,
    PROC_RUNNING,
    PROC_RUNNABLE,
    PROC_PENDING_SYSCALL
};

#define MAX_NPROCESS        16
#define SAVED_REGISTER_NUM  32
#define SAVED_REGISTER_SIZE SAVED_REGISTER_NUM * 4
#define SAVED_REGISTER_ADDR (void*)(EGOS_STACK_TOP - SAVED_REGISTER_SIZE)

struct process {
    int pid;
    struct syscall syscall;
    enum proc_status status;
    uint mepc, saved_registers[SAVED_REGISTER_NUM];
    /* Student's code goes here (Preemptive Scheduler | System Call). */

    /* Add new fields for lifecycle statistics, MLFQ or process sleep. */

    //MLFQ
    int mlfq_level;
    ulonglong remainingt_onlevel;
    ulonglong current_level_runtime;
    ulonglong total_runtime;

    //process sleep
    ulonglong sleep_until_time;

    //Lifecycle statistics
    ulonglong time_created;
    ulonglong time_terminated;
    ulonglong cpu_runtime;
    int no_interrupts;
    ulonglong time_first_scheduled;
    ulonglong time_last_scheduled;


    /* Student's code ends here. */
};

ulonglong mtime_get();

int proc_alloc();
void proc_free(int);
void proc_set_ready(int);
void proc_set_running(int);
void proc_set_runnable(int);
void proc_set_pending(int);

void mlfq_reset_level();
void mlfq_update_level(struct process* p, ulonglong runtime);
void proc_sleep(int pid, uint usec);
void proc_coresinfo();

extern uint core_to_proc_idx[NCORES];
