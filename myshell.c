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


struct PCBTable processes[MAX_PROCESSES];

void my_init(void) {

    // Initialize what you need here

 
}

void print_tokens(size_t num_tokens, char** tokens) {
    printf("tokens excl. NULL: %ld\n", num_tokens - 1);
    if (num_tokens >= 1) {
        printf("command: %s\n", tokens[0]);
        for (size_t i = 1; i < num_tokens - 1;i++) {
            printf("tokens[%ld]: %s\n", i, tokens[i]);
        }
    }
}

// tokens: array of strings, last is NULL
void my_process_command(size_t num_tokens, char **tokens) {
    // Your code here, refer to the lab document for a description of the
    // arguments
    //print_tokens(num_tokens, tokens);
    char *first = tokens[0];

    
    
    if (access(first, F_OK | R_OK) == -1) {
        fprintf(stderr, "%s not found\n", first);
        return;
    }

    pid_t child_id;
    int status;

    if ((child_id = fork()) == 0) {
        if (execv(first, tokens) == -1) {
            fprintf(stderr, "%s not found\n", first);
            _Exit(1);
        } 
    }

    waitpid(child_id, &status, 0);

    
}



void my_quit(void) {

    // Clean up function, called after "quit" is entered as a user command



    printf("\nGoodbye\n");
}
