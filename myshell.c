/**
 * CS2106 AY22/23 Semester 1 - Lab 2
 *
 * This file contains function definitions. Your implementation should go in
 * this file.
 */


#include "myshell.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define EXITED 1
#define RUNNING 2
#define TERMINATING 3
#define STOPPED 4
#define NOT_EXITED -1
#define BG_TOKEN "&"

typedef struct PCBTable PCBTable;

PCBTable processes[MAX_PROCESSES];
// invariant: next index to add at = num_processes
int num_processes = 0;

void print_tokens(size_t num_tokens, char** tokens) {
    printf("tokens excl. NULL: %ld\n", num_tokens - 1);
    if (num_tokens >= 1) {
        printf("command: %s\n", tokens[0]);
        for (size_t i = 1; i < num_tokens - 1;i++) {
            printf("tokens[%ld]: %s\n", i, tokens[i]);
        }
    }
}

// add a new running process
void add_process(pid_t pid) {
    PCBTable proc = { pid, RUNNING, NOT_EXITED };
    processes[num_processes] = proc;
    num_processes += 1;
    return;
}

// print details for one process
void print_process(PCBTable *proc) {
    pid_t pid = proc->pid;
    int status = proc->status;
    int exitCode = proc->exitCode;

    char *with_code = "[%d] %s %d\n";
    char *without_code = "[%d] %s\n";

    switch (status) {
        case 1:
        case 4:
            printf(with_code, pid, "Exited", exitCode);
            break;
        case 2:
            printf(without_code, pid, "Running");
            break;
        case 3:
            printf(without_code, pid, "Terminating");
            break;
    }
}

// print details of all processes
void print_all_processes() {
    for (int i = 0; i < num_processes; i++) {
        print_process(&processes[i]);
    }
}

// print process count for the given status
void print_process_count(int status) {
    int total = 0;
    for (int i = 0; i < num_processes; i++) {
        PCBTable *proc = &processes[i];
        if (proc->status == status) {
            total += 1;
        }
    }

    switch (status) {
        case EXITED:
            printf("Total exited process: %d\n", total);
            break;
        case RUNNING:
            printf("Total running process: %d\n", total);
            break;
        case TERMINATING:
            printf("Total terminating process: %d\n", total);
            break;
        case STOPPED:
            printf("Total stopped process: %d\n", total);
            break;
    }
}

// option is numeric numbers +ve,-ve
// options: 0,1,2,3
void info_handler(size_t num_tokens, char** tokens) {
    //print_tokens(num_tokens, tokens);

    // num_tokens includes NULL
    if (num_tokens - 1 > 1) {
        int option = atoi(tokens[1]);

        switch (option) {
            case 0:
                print_all_processes();                
                break;
            case EXITED:
            case RUNNING:
            case TERMINATING:
            case STOPPED:
                print_process_count(option);
                break;
            default:
                fprintf(stderr, "Wrong command\n");
                break;
        }
    } else {
        fprintf(stderr, "Wrong command\n");
    }
    
}

void my_init(void) {
    // Initialize what you need here
    //signal(SIGCHLD, child_handler);
 
}

// blocking wait and state change for a specific child process
void blocking_wait_on_process(pid_t pid) {
    for (int i = 0; i < num_processes; i++) {
        PCBTable *proc = &processes[i];
        int status;

        if (proc->pid != pid) {
            continue;
        }

        int res = waitpid(proc->pid, &status, 0);

        if (WIFEXITED(status)) {
            proc->status = EXITED;
            // get actual exit status using macro
            proc->exitCode = WEXITSTATUS(status);
        }
    }
}

// num_tokens: length of src including NULL
// assumption: dest created with size num_tokens-1
void remove_bg_token(char** src, char** dest, size_t num_tokens) {
    size_t index = 0;
    for (size_t i = 0; i < num_tokens; i++) {
        if (src[i] == NULL) {
            continue;
        }

        if (strcmp(BG_TOKEN, src[i]) == 0) {
            continue;
        }
        dest[index] = src[i];
        index++;
    }
    dest[num_tokens-2] = NULL;
}


// tokens: array of strings, last is NULL
void my_process_command(size_t num_tokens, char **tokens) {
    // Your code here, refer to the lab document for a description of the
    // arguments
    //print_tokens(num_tokens, tokens);

    // background: don't blocking wait
    int blocking = 1;

    if (strcmp(tokens[num_tokens-2], BG_TOKEN) == 0) {
        blocking = 0;
        char* new_tokens[num_tokens-1];
        remove_bg_token(tokens, new_tokens, num_tokens);
        //print_tokens(num_tokens - 1, new_tokens);
    }

    char *first = tokens[0];

    // check for command names
    if (strcmp(first, "info") == 0) {
        info_handler(num_tokens, tokens);
        return;
    }

    // check if file accessible and executable
    if (access(first, F_OK | R_OK | X_OK) == -1) {
        fprintf(stderr, "%s not found\n", first);
        return;
    }

    pid_t child_id;

    // inside child
    if ((child_id = fork()) == 0) {
        // this branch should never run under specified cond
        if (execv(first, tokens) == -1) {
            fprintf(stderr, "%s not found\n", first);
            _Exit(1);
        } 
    } else if (child_id > 0){
        // fork success
        add_process(child_id);
    } else {
        // fork failed
        fprintf(stderr, "%s not found\n", first);
    }

    if (blocking) {
        blocking_wait_on_process(child_id);
    }
}



void my_quit(void) {

    // Clean up function, called after "quit" is entered as a user command



    printf("\nGoodbye\n");
}
