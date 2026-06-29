// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <string.h>
// #include <sys/wait.h>
// #include <sys/shm.h>
// #include <signal.h>
// #include "scheduler_shm.h"

// int main(int argc, char *argv[])
//  {
//     if (argc != 3) 
//     {
//         fprintf(stderr,"Usage- %s NCPU TSLICE(ms)\n", argv[0]);
//         exit(1);
//     }

//     int ncpu = atoi(argv[1]);
//     int tslice = atoi(argv[2]);

//     //creating shared mem
//     int shmid = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
//     if (shmid < 0) 
//     {
//         perror("shmget");
//         exit(1);
//     }

//     SharedData *shm = (SharedData *)shmat(shmid, NULL, 0);
//     if (shm == (void *)-1) {
//         perror("shmat");
//         exit(1);
//     }

//     //initializing shared memory
//     shm->job_count = 0;
//     shm->ncpu = ncpu;
//     shm->tslice = tslice;
//     shm->scheduler_running = 1;

//     //start scheduler
//     pid_t schedulerpid = fork();
//     if (schedulerpid == 0) 
//     {
//         execlp("./SimpleScheduler","./SimpleScheduler", NULL);
//         perror("execlp");
//         exit(1);
//     }

//     printf("SimpleShell started...Scheduler PID: %d\n", schedulerpid);

//     char input[256];
//     while (1) 
//     {
//         printf("SimpleShell$ ");
//         fflush(stdout);

//         if (fgets(input, sizeof(input), stdin) == NULL)
//             continue;

//         input[strcspn(input, "\n")] = '\0';

//         if (strncmp(input, "submit ", 7) == 0) //7 spaces to compare submit
//         {
//             char *prog = input + 7;
//             if (shm->job_count >= MAX_JOBS) 
//             {
//                 printf("Reached max job limit\n");
//                 continue;
//             }

//             pid_t pid = fork();
//             if (pid == 0) 
//             {
//                 // raise(SIGSTOP);
//                 execlp(prog, prog, NULL);
//                 perror("execlp");
//                 exit(1);
//             }

//             // Add job to shared memory
//             Job *job = &shm->jobs[shm->job_count++];
//             job->pid = pid;
//             strncpy(job->name, prog, sizeof(job->name));
//             job->state = 0; // ready
//             job->run_time_slices = 0;
//             job->wait_time_slices = 0;

//             printf("Submitted job %s (PID %d)\n", prog, pid);
//         } 

//         else if (strcmp(input, "exit") == 0) 
//         {
//             printf("Terminating scheduler and shell...\n");
//             shm->scheduler_running = 0;

//             waitpid(schedulerpid, NULL, 0);

//             printf("\n=== Job Summary (in multiples of %d ms) ===\n", tslice);
//             for (int i = 0; i < shm->job_count; i++)
//             {
//                 Job *job = &shm->jobs[i];
//                 printf("Job: %-15s PID: %-6d Run: %-3d Wait: %-3d\n",job->name, job->pid, job->run_time_slices, job->wait_time_slices);
//             }

//             shmdt(shm);//detaching the shared memory segment from the process’s address space.
//             shmctl(shmid, IPC_RMID, NULL);//removing shared memory segment
//             break;
//         } 
//         else {
//             printf("Unknown command. Use: submit <executable> | exit\n");
//         }
//     }

//     return 0;
// }





#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <signal.h>
#include "scheduler_shm.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <NCPU> <TSLICE(ms)>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int ncpu = atoi(argv[1]);
    int tslice = atoi(argv[2]);

    // Create shared memory
    int shmid = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    SharedData *shm = (SharedData *)shmat(shmid, NULL, 0);
    if (shm == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Initialize shared memory
    shm->job_count = 0;
    shm->ncpu = ncpu;
    shm->tslice = tslice;
    shm->scheduler_running = 1;

    // Start scheduler as a daemon process
    pid_t sched_pid = fork();
    if (sched_pid == 0) {
        execlp("./SimpleScheduler", "./SimpleScheduler", NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }

    printf("SimpleShell started. Scheduler PID: %d\n", sched_pid);

    char input[256];
    while (1) {
        printf("SimpleShell$ ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL)
            continue;

        input[strcspn(input, "\n")] = '\0'; // remove newline

        if (strncmp(input, "submit ", 7) == 0) {
            char *prog = input + 7;
            if (shm->job_count >= MAX_JOBS) {
                printf("Max job limit reached.\n");
                continue;
            }

            pid_t pid = fork();
            if (pid == 0) {
                // Child waits for scheduler signal
                // raise(SIGSTOP);
                execlp(prog, prog, NULL);
                perror("execlp");
                exit(EXIT_FAILURE);
            }

            // Add job to shared memory
            Job *job = &shm->jobs[shm->job_count++];
            job->pid = pid;
            strncpy(job->name, prog, sizeof(job->name));
            job->state = 0; // ready
            job->run_time_slices = 0;
            job->wait_time_slices = 0;

            printf("Submitted job %s (PID %d)\n", prog, pid);
        } 
        else if (strcmp(input, "exit") == 0) {
            printf("Terminating scheduler and shell...\n");
            shm->scheduler_running = 0;

            waitpid(sched_pid, NULL, 0);

            printf("\n=== Job Summary (in multiples of %d ms) ===\n", tslice);
            for (int i = 0; i < shm->job_count; i++) {
                Job *job = &shm->jobs[i];
                printf("Job: %-15s PID: %-6d Run: %-3d Wait: %-3d\n",
                       job->name, job->pid, job->run_time_slices, job->wait_time_slices);
            }

            shmdt(shm);
            shmctl(shmid, IPC_RMID, NULL);
            break;
        } 
        else {
            printf("Unknown command. Use: submit <executable> | exit\n");
        }
    }

    return 0;
}