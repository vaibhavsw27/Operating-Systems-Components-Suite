#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define MAX_INPUT_SIZE 1000
#define MAX_HISTORY_SIZE 500

char *history[MAX_HISTORY_SIZE];
pid_t process_pid[MAX_HISTORY_SIZE];
int pid_count = 0;
time_t start_time[MAX_HISTORY_SIZE];
int start_count = 0;
double execution_time[MAX_HISTORY_SIZE];
int exec_time = 0;
int history_count = 0;

char* trim(char* str) {
    while(*str==' ') str++;
    char* end = str + strlen(str) - 1;
    while(end > str && (*end==' ' || *end=='\t' || *end=='\n' || *end=='\r')) *end-- = '\0';
    return str;
}

void addToHistory(char *command){
    if (history_count < MAX_HISTORY_SIZE){
        history[history_count] = strdup(command);
        history_count+=1;
    } else{
        free(history[0]);
        for (int i = 0; i < MAX_HISTORY_SIZE - 1; i++){
            history[i] = history[i + 1];
            process_pid[i] = process_pid[i + 1];
            start_time[i] = start_time[i + 1];
            execution_time[i] = execution_time[i + 1];
        }
        history[MAX_HISTORY_SIZE - 1] = strdup(command);
    }
}

void parseInput(char *input, char *parameters[], int *counter) {
    *counter = 0;
    char *token = strtok(input, " ");

    while (token != NULL) {
        parameters[(*counter)] = token;
        (*counter)++;

        token = strtok(NULL, " ");
    }
}

int launch(char *command, char *args[]) {
    int pid;
    time_t start, end;

    pid = fork();


    if (pid < 0) {
        perror("Forking error");
    } 
    else if (pid == 0) {
        if (execvp(command, args) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } 
    else {
        
        process_pid[pid_count++] = pid;
        time(&start);

        waitpid(pid, NULL, 0);

        time(&end);
        execution_time[exec_time++] = difftime(end, start);
        start_time[start_count++] = start; 
    }

    return 0;
}

int launch_pipe(char* command1[], char* command2[]) {
    int pid;
    time_t start, end;
    int fd[2];

    if (pipe(fd) < 0) {
        perror("Piping error");
        exit(EXIT_FAILURE);
    }

    int pid1 = fork();

    if (pid1 < 0) {
        perror("Forking error 1");
        exit(EXIT_FAILURE);
    }

    else if (pid1 == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        if (execvp(command1[0], command1) == -1) {
            perror("execvp command1");
            exit(EXIT_FAILURE);
        }
    }
    else{
        time(&start);
    }

    int pid2 = fork();

    if (pid2 < 0) {
        perror("Forking error 2");
        exit(EXIT_FAILURE);
    }

    else if(pid2 == 0) {
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        close(fd[1]);
        if (execvp(command2[0], command2) == -1) {
            perror("execvp command2");
            exit(EXIT_FAILURE);
        }
    }
    else{
        process_pid[pid_count++] = pid2;
        time(&end);
        execution_time[exec_time++] = difftime(end, start);
        start_time[start_count++] = start; 
    }

    close(fd[0]);
    close(fd[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 1;
}

int launch_pip2(char* command1[], char* command2[], char* command3[]) {
    int pid;
    time_t start, end;
    int pipe1[2], pipe2[2]; 

    
    if (pipe(pipe1) <0 || pipe(pipe2) <0) {
        perror("Pipe creation error");
        exit(EXIT_FAILURE);
    }

    int pid1 = fork();
    if (pid1 < 0) {
        perror("Forking error 1");
        exit(EXIT_FAILURE);
    } 
    else if (pid1 == 0) {
        close(pipe1[0]); 
        close(pipe2[0]); 
        close(pipe2[1]); 
        dup2(pipe1[1], STDOUT_FILENO); 
        close(pipe1[1]); 
        if(execvp(command1[0], command1) < 0){;
            perror("execvp command1");
            exit(EXIT_FAILURE);
        }
    }
    else{
        time(&start);
    }

    int pid2 = fork();
    if (pid2 < 0) {
        perror("Forking error 2");
        exit(EXIT_FAILURE);
    } else if (pid2 == 0) {
        
        close(pipe1[1]); 
        close(pipe2[0]);
        dup2(pipe1[0], STDIN_FILENO); 
        dup2(pipe2[1], STDOUT_FILENO); 
        close(pipe1[0]); 
        close(pipe2[1]); 

        if(execvp(command2[0], command2) < 0){;
            perror("execvp command1");
            exit(EXIT_FAILURE);
        }
    }

    int pid3 = fork();
    if (pid3 < 0) {
        perror("Forking error 3");
        exit(EXIT_FAILURE);
    } else if (pid3 == 0) {
        
        close(pipe1[0]);
        close(pipe1[1]); 
        close(pipe2[1]); 
        dup2(pipe2[0], STDIN_FILENO); 
        close(pipe2[0]); 

        if(execvp(command3[0], command3) < 0){;
            perror("execvp command1");
            exit(EXIT_FAILURE);
        }
    }
    else{
        time(&end);
        execution_time[exec_time++] = difftime(end, start);
        start_time[start_count++] = start;      
    }

    
    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);

    return 0;
}

int main(){
    char input[MAX_INPUT_SIZE];
    char *parameters[MAX_INPUT_SIZE];
    int counter;

    while (true){
        bool flag = false;
        int count = 0;

        printf("SimpleShell~/Group-16$ ");
        if (fgets(input, sizeof(input), stdin) == NULL){
            printf("\nExiting...\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) == 0) continue;

        if (strcmp(input, "exit") == 0){
            for (int i = 0; i < history_count; i++){
                printf("%d: %s\n", i + 1, history[i]);
                printf("PID: %d\n", process_pid[i]);
                printf("Start time: %s", ctime(&start_time[i]));
                printf("Execution time: %.2f s\n", execution_time[i]);
            }
            break;
        }

        if (strcmp(input, "history") == 0){
            for (int i = 0; i < history_count; i++){
                printf("%d: %s\n", i + 1, history[i]);
            }
            continue;
        }

        if (strlen(input) > 0){
            addToHistory(input);
        }

        for (int i = 0; input[i] != '\0';i++){
            if (input[i] == '|'){
            flag= true;
            count++;       
            }
        }

        parseInput(input, parameters,&counter);
        parameters[counter++] = NULL;

        if (flag){
            if (count == 1){
                char* command1[MAX_INPUT_SIZE];
                char* command2[MAX_INPUT_SIZE];
                int counter1= 0;
                int counter2 = 0;
                int temp = 0;
                while (strcmp(parameters[temp],"|") != 0){
                    command1[counter1++] = parameters[temp++];
                }
                command1[counter1] = NULL;
                temp++;
                
                while(parameters[temp] != NULL){
                    command2[counter2++] = parameters[temp++];
                }
                command2[counter] = NULL;
                launch_pipe(command1,command2);

            }
            else if (count == 2){
                char* command1[MAX_INPUT_SIZE];
                char* command2[MAX_INPUT_SIZE];
                char* command3[MAX_INPUT_SIZE];
                int counter1= 0;
                int counter2 = 0;
                int counter3 = 0;
                int temp = 0;
                while (strcmp(parameters[temp],"|") != 0){
                    command1[counter1++] = parameters[temp++];
                }
                command1[counter1] = NULL;
                temp++;

                while(strcmp(parameters[temp],"|") != 0){
                    command2[counter2++] = parameters[temp++];
                }
                command2[counter] = NULL;
                temp++;
                while(parameters[temp] != NULL){
                    command3[counter3++] = parameters[temp++];
                }
                command3[counter] = NULL;
                launch_pip2(command1,command2,command3);
            }
            else{
                printf("Shell cannot handle more than 2 layers of piping\n");
            }
            }
            else{
                launch(parameters[0],parameters);
            }

        }

    for (int i = 0; i < history_count; i++){
        free(history[i]);
    }

    return 0;
}