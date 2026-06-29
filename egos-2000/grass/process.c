/*
 * (C) 2025, Cornell University
 * All rights reserved.
 *
 * Description: helper functions for process management
 */

 //Vaibhav 2024597 OS BONUS PROJECT

#include "process.h"

#define MLFQ_NLEVELS          5
#define MLFQ_RESET_PERIOD     10000000         /* 10 seconds */
#define MLFQ_LEVEL_RUNTIME(x) (x + 1) * 100000 /* e.g., 100ms for level 0 */
extern struct process proc_set[MAX_NPROCESS + 1];

static void proc_set_status(int pid, enum proc_status status) {
    for (uint i = 0; i < MAX_NPROCESS; i++)
        if (proc_set[i].pid == pid) proc_set[i].status = status;
}

void proc_set_ready(int pid) { proc_set_status(pid, PROC_READY); }
void proc_set_running(int pid) { proc_set_status(pid, PROC_RUNNING); }
void proc_set_runnable(int pid) { proc_set_status(pid, PROC_RUNNABLE); }
void proc_set_pending(int pid) { proc_set_status(pid, PROC_PENDING_SYSCALL); }

int proc_alloc() {
    static uint curr_pid = 0;
    for (uint i = 1; i <= MAX_NPROCESS; i++)
        if (proc_set[i].status == PROC_UNUSED) {
            proc_set[i].pid    = ++curr_pid;
            proc_set[i].status = PROC_LOADING;
            /* Student's code goes here (Preemptive Scheduler | System Call). */

            /* Initialization of lifecycle statistics, MLFQ or process sleep. */

            //MLFQ
            proc_set[i].mlfq_level=0;
            proc_set[i].remainingt_onlevel=MLFQ_LEVEL_RUNTIME(0);
            proc_set[i].current_level_runtime=0;
            proc_set[i].total_runtime=0;


            //process sleep
            proc_set[i].sleep_until_time=0;

            //Lifecycle statistics
            proc_set[i].time_created=mtime_get(); //current time
            proc_set[i].time_terminated=0;
            proc_set[i].cpu_runtime=0;
            proc_set[i].no_interrupts=0;
            proc_set[i].time_first_scheduled=0;
            proc_set[i].time_last_scheduled=0;


            /* Student's code ends here. */
            return curr_pid;
        }

    FATAL("proc_alloc: reach the limit of %d processes", MAX_NPROCESS);
}

void proc_free(int pid) 
{
    /* Student's code goes here (Preemptive Scheduler). */

    /* Print the lifecycle statistics of the terminated process or processes. */
    if (pid != GPID_ALL) 
    {

        for (uint i = 0; i < MAX_NPROCESS; i++)
            if (proc_set[i].pid >= GPID_USER_START && proc_set[i].status != PROC_UNUSED) 
            {
                ulonglong turnaround_time=0;
                ulonglong response_time=0;

                proc_set[i].time_terminated=mtime_get();


                if (proc_set[i].time_terminated>proc_set[i].time_created) //error
                {
                    turnaround_time=proc_set[i].time_terminated-proc_set[i].time_created; //the time between process creation and termination
                }
                if (proc_set[i].time_first_scheduled>0) //error
                {
                    response_time=proc_set[i].time_first_scheduled-proc_set[i].time_created;//the time between process creation and the first time scheduled

                }
                
                INFO("Process %d has been terminated-", proc_set[i].pid);
                INFO("Turnaround time- %d x 10^-6 seconds",(int)turnaround_time);
                INFO("Response time- %d x 10^-6 seconds",(int)response_time);
                INFO("CPU time- %d x 10^-6 seconds",(int)proc_set[i].cpu_runtime);
                INFO("No of timer interrupts- %d",proc_set[i].no_interrupts);

                earth->mmu_free(pid);
                proc_set_status(pid, PROC_UNUSED);
                break;
            }
            } 
            else
            {
                /* Free all user processes. */
                for (uint i = 0; i < MAX_NPROCESS; i++)
                    if (proc_set[i].pid >= GPID_USER_START && proc_set[i].status != PROC_UNUSED) 
                    {
                        ulonglong turnaround_time=0;
                        ulonglong response_time=0;

                        proc_set[i].time_terminated=mtime_get();


                        if (proc_set[i].time_terminated>proc_set[i].time_created) //error
                        {
                            turnaround_time=proc_set[i].time_terminated-proc_set[i].time_created; //the time between process creation and termination
                        }
                        if (proc_set[i].time_first_scheduled>0) //error
                        {
                            response_time=proc_set[i].time_first_scheduled-proc_set[i].time_created;//the time between process creation and the first time scheduled

                        }
                        
                        INFO("Process %d has been terminated-", proc_set[i].pid);
                        INFO("Turnaround time- %d x 10^-6 seconds",(int)turnaround_time);
                        INFO("Response time- %d x 10^-6 seconds",(int)response_time);
                        INFO("CPU time- %d x 10^-6 seconds",(int)proc_set[i].cpu_runtime);
                        INFO("No of timer interrupts- %d",proc_set[i].no_interrupts);

                        earth->mmu_free(proc_set[i].pid);
                        proc_set[i].status = PROC_UNUSED;

            }
    }
    /* Student's code ends here. */
}




void mlfq_update_level(struct process* p, ulonglong runtime) {
    /* Student's code goes here (Preemptive Scheduler). */

    /* Update the MLFQ-related fields in struct process* p after this
     * process has run on the CPU for another runtime x 10^-6 seconds. */

    p->total_runtime+=runtime;
    p->current_level_runtime+=runtime;
    p->cpu_runtime+=runtime;
    p->remainingt_onlevel-=runtime;

    if (p->remainingt_onlevel<=0) //time over for this level
    {
        if (p->mlfq_level<MLFQ_NLEVELS-1) //not yet at lowest level
        {
            p->mlfq_level+=1;
        }

        p->remainingt_onlevel=MLFQ_LEVEL_RUNTIME(p->mlfq_level);
        p->current_level_runtime=0;
    }

    /* Student's code ends here. */
}

void mlfq_reset_level() 
{
    /* Student's code goes here (Preemptive Scheduler). */
    if (!earth->tty_input_empty()) 
    {
        /* Reset the level of GPID_SHELL if there is pending keyboard input. */
        int count=0;
        while (count<MAX_NPROCESS) 
        {
            if (proc_set[count].pid==GPID_SHELL) 
            {
                proc_set[count].mlfq_level=0;
                break;
            }
            count++;
        }
        
    }

    static ulonglong MLFQ_last_reset_time = 0;
    /* Reset the level of all processes every MLFQ_RESET_PERIOD microseconds */

    ulonglong current_time=mtime_get();
    if (MLFQ_last_reset_time==0) 
    {
        MLFQ_last_reset_time=current_time;
    }

    //10 seconds
    if ((current_time-MLFQ_last_reset_time)>=MLFQ_RESET_PERIOD) 
    {
        for (int i=0;i<MAX_NPROCESS;i++) 
        {
            if (proc_set[i].status!=PROC_UNUSED) 
            {
                proc_set[i].mlfq_level=0;
            }
        }
        MLFQ_last_reset_time=current_time;
    }

    /* Student's code ends here. */
}





// assingment dint ask for this
void proc_sleep(int pid, uint usec) {
    /* Student's code goes here (System Call & Protection). */

    /* Update the sleep-related fields in the struct process for process pid. */

    /* Student's code ends here. */
}

void proc_coresinfo() {
    /* Student's code goes here (Multicore & Locks). */

    /* Print out the pid of the process running on each CPU core. */

    /* Student's code ends here. */
}
