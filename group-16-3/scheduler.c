// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <signal.h>
// #include <sys/shm.h>
// #include <sys/wait.h>
// #include <errno.h>
// #include <time.h>
// #include <string.h>
// #include "scheduler_shm.h"

// void ms_sleep(int ms) {
//     usleep(ms * 1000);
// }

// int main() {
//     int shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
//     if (shmid < 0) { perror("shmget"); exit(1); }

//     SharedData *shm = (SharedData *)shmat(shmid, NULL, 0);
//     if (shm == (void *)-1) { perror("shmat"); exit(1); }

//     printf("SimpleScheduler running...\n");

//     int ready_queue[MAX_JOBS];
//     int queue_len = 0;

//     while (shm->scheduler_running || queue_len > 0) {
//         // Rebuild ready queue from all jobs with state 0
//         queue_len = 0;
//         for (int i = 0; i < shm->job_count; i++)
//             if (shm->jobs[i].state == 0)
//                 ready_queue[queue_len++] = i;

//         if (queue_len == 0) 
//         {
//             ms_sleep(10);
//             continue;
//         }

//         int running = queue_len < shm->ncpu ? queue_len : shm->ncpu;

//         for (int i = 0; i < running; i++) 
//         {
//             int idx = ready_queue[i];
//             kill(shm->jobs[idx].pid, SIGCONT);
//             shm->jobs[idx].state = 1;
//         }

//         // Run for quantum
//         ms_sleep(shm->tslice);

//         // Stop jobs and rotate unfinished jobs
//         int new_queue[MAX_JOBS], new_len = 0;
//         for (int i = 0; i < running; i++) {
//             int idx = ready_queue[i];
//             int status;
//             pid_t ret = waitpid(shm->jobs[idx].pid, &status, WNOHANG);

//             if (ret == shm->jobs[idx].pid || !shm->scheduler_running) {
//                 shm->jobs[idx].state = 2;
//                 if (!shm->scheduler_running) kill(shm->jobs[idx].pid, SIGKILL);
//             } else {
//                 kill(shm->jobs[idx].pid, SIGSTOP);
//                 shm->jobs[idx].run_time_slices++;
//                 shm->jobs[idx].state = 0; // back to ready
//                 new_queue[new_len++] = idx;
//             }
//         }

//         // Add remaining jobs in queue (they weren’t run this quantum)
//         for (int i = running; i < queue_len; i++)
//             new_queue[new_len++] = ready_queue[i];

//         memcpy(ready_queue, new_queue, sizeof(int) * new_len);
//         queue_len = new_len;
//     }

//     printf("All jobs completed. Scheduler exiting.\n");
//     shmdt(shm);
//     return 0;
// }






#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include "scheduler_shm.h"

void ms_sleep(int ms) 
{
    usleep(ms * 1000);
}

int main() 
{
    int shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
    if (shmid < 0) { perror("shmget"); exit(1); }

    SharedData *shm = (SharedData *)shmat(shmid, NULL, 0);
    if (shm == (void *)-1) { perror("shmat"); exit(1); }

    printf("SimpleScheduler running...\n");

    int ready_queue[MAX_JOBS];
    int queue_len = 0;

    while (shm->scheduler_running || queue_len > 0) {
        // Rebuild ready queue from all jobs with state 0
        queue_len = 0;
        for (int i = 0; i < shm->job_count; i++)
            if (shm->jobs[i].state == 0)
                ready_queue[queue_len++] = i;

        if (queue_len == 0) {
            ms_sleep(10);
            continue;
        }

        // Pick up to NCPU jobs
        int running = queue_len < shm->ncpu ? queue_len : shm->ncpu;

        for (int i = 0; i < running; i++) {
            int idx = ready_queue[i];
            kill(shm->jobs[idx].pid, SIGCONT);
            shm->jobs[idx].state = 1;
        }

        // Run for quantum
        ms_sleep(shm->tslice);

        // Stop jobs and rotate unfinished jobs
        int new_queue[MAX_JOBS], new_len = 0;
        for (int i = 0; i < running; i++) {
            int idx = ready_queue[i];
            int status;
            pid_t ret = waitpid(shm->jobs[idx].pid, &status, WNOHANG);

            if (ret == shm->jobs[idx].pid || !shm->scheduler_running) {
                shm->jobs[idx].state = 2;
                if (!shm->scheduler_running) kill(shm->jobs[idx].pid, SIGKILL);
            } else {
                kill(shm->jobs[idx].pid, SIGSTOP);
                shm->jobs[idx].run_time_slices++;
                shm->jobs[idx].state = 0; // back to ready
                new_queue[new_len++] = idx;
            }
        }

        // Add remaining jobs in queue (they weren’t run this quantum)
        for (int i = running; i < queue_len; i++)
            new_queue[new_len++] = ready_queue[i];

        memcpy(ready_queue, new_queue, sizeof(int) * new_len);
        queue_len = new_len;
    }

    printf("All jobs completed. Scheduler exiting.\n");
    shmdt(shm);
    return 0;
}
