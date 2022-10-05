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
#include <errno.h>

#define EXITED 1
#define RUNNING 2
#define TERMINATING 3
#define STOPPED 4
#define NOT_EXITED -1
#define BG_TOKEN "&"
#define CHAIN_TOKEN ";"
#define INPUT_TOKEN "<"
#define OUTPUT_TOKEN ">"
#define ERR_TOKEN "2>"
#define INPUT_INDEX 0
#define OUTPUT_INDEX 1
#define ERR_INDEX 2
#define FILE_TOKENS_LENGTH 3

typedef struct PCBTable PCBTable;

PCBTable processes[MAX_PROCESSES];
// invariant: next index to add at = num_processes
int num_processes = 0;

/* PRINTING */

void print_tokens(size_t num_tokens, char** tokens) {
    printf("tokens excl. NULL: %ld\n", num_tokens - 1);
    if (num_tokens >= 1) {
        printf("command: %s\n", tokens[0]);
        for (size_t i = 1; i < num_tokens;i++) {
            printf("tokens[%ld]: %s\n", i, tokens[i]);
        }
    }
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

void my_init(void) {
    // Initialize what you need here
    //signal(SIGCHLD, child_handler);
 
}

/* PROCESS UPDATES */

// add a new running process
void add_process(pid_t pid) {
    PCBTable proc = { pid, RUNNING, NOT_EXITED };
    processes[num_processes] = proc;
    num_processes += 1;
    return;
}

// Updates process at &proc according to status returned from wait*
void update_process_state(PCBTable *proc, int status) {
    if (WIFEXITED(status)) {
            proc->status = EXITED;
            // get actual exit status using macro
            proc->exitCode = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        proc->status = EXITED;
        proc->exitCode = WTERMSIG(status);
    }
}

// wait on a process given pointer to the process
void wait_on_proc(PCBTable *proc) {
    int status;
    waitpid(proc->pid, &status, 0);
    update_process_state(proc, status);
}

// blocking wait and state change for a specific child process
void blocking_wait_on_process(pid_t pid) {
    for (int i = 0; i < num_processes; i++) {
        PCBTable *proc = &processes[i];

        if (proc->pid != pid) {
            continue;
        }

        wait_on_proc(proc);
    }
}

// state change on any processes that have not been waited on already
void update_processes() {
    for (int i = 0; i < num_processes; i++) {
        PCBTable *proc = &processes[i];
        int status;

        int res = waitpid(proc->pid, &status, WNOHANG);

        // either waited already or no state change
        if (res <= 0) {
            continue;
        }
        
        update_process_state(proc, status);
    }
}

/* PARSING AND COMMAND HANDLERS */

// assumption: each token appears only once
// find first index of <,> or 2> indicating re-direction
// return -1 if no such token, else the index of the first token
// actual_tokens: tokens without BG_TOKEN if any
// indices: array length 3, already created
size_t first_index_of_file_token(size_t length, char ** actual_tokens, size_t *indices) {
    size_t NO_VAL = -1;
    size_t min_index = -1;
    for (size_t i = 0; i < length; i++) {
        if (actual_tokens[i] == NULL) {
            continue;
        }

        size_t curr = -1;

        if (strcmp(actual_tokens[i], INPUT_TOKEN) == 0) {
            indices[INPUT_INDEX] = i;
            curr = i;
        } else if (strcmp(actual_tokens[i], OUTPUT_TOKEN) == 0) {
            indices[OUTPUT_INDEX] = i;
            curr = i;
        } else if (strcmp(actual_tokens[i], ERR_TOKEN) == 0) {
            indices[ERR_INDEX] = i;
            curr = i;
        }

        if (curr != NO_VAL && min_index == NO_VAL) {
            min_index = curr;
        }
    }

    return min_index;

}   

// buffer = original[0:index], index exclusive
// buffer already created with length index+1 to accomodate NULL at end
void slice_excl(char** original, char** buffer, size_t index) {
    for (size_t i = 0; i < index; i++) {
        buffer[i] = original[i];
    }
    buffer[index] = NULL;
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

// wait command
void wait_handler(char**tokens) {
    //print_tokens(num_tokens, tokens);
    pid_t pid = atoi(tokens[1]);
    update_processes();

    for (int i = 0; i < num_processes; i++) {
        PCBTable *proc = &processes[i];
        if (proc->pid != pid) {
            continue;
        }

        if (proc->status == RUNNING) {
            wait_on_proc(proc);
        }
    }
}

// terminate PID
void terminate_handler(char**tokens) {
    pid_t pid = atoi(tokens[1]);
    update_processes();

    for (int i = 0; i < num_processes; i++) {
        PCBTable *proc = &processes[i];
        if (proc->pid != pid) {
            continue;
        }

        if (proc->status == RUNNING) {
            proc->status = TERMINATING;
            // just send the signal: process can intercept and do what it wants
            kill(proc->pid, SIGTERM);
        }
    }
}

// FILE REDIRECT HANDLERS

// return -1 if in_file not accessible, else call dup2
// return 0 if input token didn't exist or success
int handle_in(char** actual_tokens, size_t *indices) {
    if (indices[INPUT_INDEX] == (size_t)-1) {
        return 0;
    }

    char *file_name = actual_tokens[indices[INPUT_INDEX] + 1];
    FILE *in_file = fopen(file_name, "r");
    if (in_file == NULL) {
        return -1;
    }

    int res = dup2(fileno(in_file), STDIN_FILENO);
    if (res == -1) {
        return res;
    }

    return 0;
}

void handle_out(char** actual_tokens, size_t *indices) {
    if (indices[OUTPUT_INDEX] == (size_t)-1) {
        return;
    }

    char *file_name = actual_tokens[indices[OUTPUT_INDEX] + 1];
    FILE *out_file = fopen(file_name, "w");
    dup2(fileno(out_file), STDOUT_FILENO);
}

void handle_err(char** actual_tokens, size_t *indices) {
    if (indices[ERR_INDEX] == (size_t)-1) {
        return;
    }

    char *file_name = actual_tokens[indices[ERR_INDEX] + 1];
    FILE *err_file = fopen(file_name, "w");
    dup2(fileno(err_file), STDERR_FILENO);
}


// num_tokens and ** tokens are for one command only
// may include &, won't include semicolon. tokens is NULL terminated
void process_one_command(size_t num_tokens, char **tokens) {
    update_processes();
    char *first = tokens[0];

    // COMMAND : check for command names
    if (strcmp(first, "info") == 0) {
        info_handler(num_tokens, tokens);
        return;
    } 

    if (strcmp(first, "wait") == 0) {
        wait_handler(tokens);
        return;
    }

    if (strcmp(first, "terminate") == 0) {
        terminate_handler(tokens);
        return;
    }

    // PROGRAM

    // check if file accessible and executable
    if (access(first, F_OK | R_OK | X_OK) == -1) {
        fprintf(stderr, "%s not found\n", first);
        return;
    }



    // blocking = 1 means normal, blocking = 0 means &
    int blocking = 1;

    // if BG_TOKEN at end, filtered tokens will be tokens without BG_TOKEN
    char* filtered_tokens[num_tokens-1];

    // check if bg proc
    if (strcmp(tokens[num_tokens-2], BG_TOKEN) == 0) {
        blocking = 0;
        remove_bg_token(tokens, filtered_tokens, num_tokens);
        //print_tokens(num_tokens - 1, filtered_tokens);
    }

    pid_t child_id;

    // inside child
    if ((child_id = fork()) == 0) {
        char** actual_tokens = tokens;
        size_t length = num_tokens;

        

        // chose original if blocking, filtered if nonblocking
        if (blocking) {
            actual_tokens = tokens;
            length = num_tokens;
        } else {
            actual_tokens = filtered_tokens;
            length = num_tokens-1;
        }

        // print_tokens(length, actual_tokens);

        // slice actual_tokens to before first file token if needed
        size_t indices[3] = {-1,-1,-1};
        size_t first_index = first_index_of_file_token(length, actual_tokens, indices);
        char *sliced[first_index+1];
        char **actual_tokens_after_slice = actual_tokens;
    
        // redirection token exists
        if (first_index != (size_t) -1) {
            
            slice_excl(actual_tokens, sliced, first_index);
            actual_tokens_after_slice = sliced;

            // printf("first ind: %ld\n", first_index);
            // for (int k = 0; k < 3; k++) {
            //     printf("ind[%d]: %ld\n", k, indices[k]);
            // }

            if (handle_in(actual_tokens, indices) == -1) {
                fprintf(stderr, "%s does not exist\n", actual_tokens[indices[INPUT_INDEX] + 1]);
                _Exit(1);
            }

            handle_out(actual_tokens, indices);
            handle_err(actual_tokens, indices);
            //print_tokens(first_index+1, actual_tokens_after_slice);
        }
       

        //print_tokens(length, actual_tokens);
        if (execv(first, actual_tokens_after_slice) == -1) {
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




    // run blocking or async
    if (blocking) {
        blocking_wait_on_process(child_id);
    } else {
        printf("Child [%d] in background\n", child_id);
    }
}


// tokens: array of strings, last is NULL
void my_process_command(size_t num_tokens, char **tokens) {
    // Your code here, refer to the lab document for a description of the
    // arguments
    // print_tokens(num_tokens, tokens);
    update_processes();
    
    // start_index of current command, length of the cmd excluding NULL/;
    size_t start_index = 0;
    size_t length = 0;

    // create new token arrays for each command to execute and pass individually
    // tokens includes all args and NULL - so that we can pass into execv directly
    for (size_t i = 0; i < num_tokens; i++) {
        if (length > 0 && (tokens[i] == NULL || strcmp(tokens[i], CHAIN_TOKEN) == 0)) {
            // execute using process
            char *current_tokens[length+1];

            for (size_t j = start_index; j < start_index + length; j++) {
                current_tokens[j - start_index] = tokens[j];
            }
            // terminate with NULL
            current_tokens[length] = NULL;


            process_one_command(length + 1, current_tokens);

            start_index=i+1;
            length = 0;            
        } else {
            length++;
        }
    }

    // TODO: split into multiple commands of num_tokens, tokens when semicolon there,
    // then pass into process_one one by one
    //process_one_command(num_tokens, tokens);
}



void my_quit(void) {

    // Clean up function, called after "quit" is entered as a user command
    update_processes();
    for (int i = 0; i < num_processes; i++) {
        PCBTable *proc = &processes[i];
        
        if (proc->status == RUNNING) {
            printf("Killing [%d]\n", proc->pid);
            kill(proc->pid, SIGTERM);
        }
    }

    printf("\nGoodbye\n");
}
