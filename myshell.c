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

#define EXITED 1
#define RUNNING 2
#define TERMINATING 3
#define STOPPED 4
#define NOT_EXITED -1

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
    printf("added proc %d\n", pid);
    return;
}

void print_process(PCBTable *proc) {
    pid_t pid = proc->pid;
    int status = proc->status;
    int exitCode = proc->exitCode;

    char *with_code = "[%d] %s %d";
    char *without_code = "[%d] %s";

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

// option is numeric numbers +ve,-ve
// options: 0,1,2,3
void info_handler(size_t num_tokens, char** tokens) {
    //print_tokens(num_tokens, tokens);
    if (num_tokens - 1 > 1) {
        int option = atoi(tokens[1]);

        switch (option) {
            case 0:
                puts("opt 0");
                break;
            case 1:
                puts("opt 1");
                break;
            case 2:
                puts("opt 2");
                break;
            case 3:
                puts("opt 3");
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

 
}



// tokens: array of strings, last is NULL
void my_process_command(size_t num_tokens, char **tokens) {
    // Your code here, refer to the lab document for a description of the
    // arguments
    //print_tokens(num_tokens, tokens);
    char *first = tokens[0];

    if (strcmp(first, "info") == 0) {
        info_handler(num_tokens, tokens);
        return;
    }

    if (access(first, F_OK | R_OK) == -1) {
        fprintf(stderr, "%s not found\n", first);
        return;
    }

    pid_t child_id;
    int status;

    // inside child
    if ((child_id = fork()) == 0) {
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

    waitpid(child_id, &status, 0);

    
}



void my_quit(void) {

    // Clean up function, called after "quit" is entered as a user command



    printf("\nGoodbye\n");
}
