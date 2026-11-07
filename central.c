#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>

#include "common.h"

#define LOCK_FILE "./central.lock"

volatile sig_atomic_t keepRunning = 1;

void cleanupAndExit(int sig) {
    const char *msg = "\ncleanupAndExit executed\n";
    write(STDOUT_FILENO, msg, strlen(msg));
    
    // unlink (therefore delete after end of this process) our link file so future runs can run
    unlink(LOCK_FILE);
    _exit(sig);
}

void handleSignal(int sig) {
    const char *msg = "\nhandleSignal executed";
    write(STDOUT_FILENO, msg, strlen(msg));
    keepRunning = 0;
    cleanupAndExit(0);
}

// checking based on a lock file
bool isAlreadyRunning() {
    // O_CREAT | O_EXCL -> file creation atomic
    int fd = open(LOCK_FILE, O_WRONLY | O_CREAT | O_EXCL, 0644);
    
    if (fd < 0) {
        // fails with EEXIST if the file exists already
        if (errno == EEXIST) {
            return true;
        }
        // some other error in errno
        perror("Failed to create lock file");
        exit(1);
    }

    // write my own pid to the lock file
    char pid_str[16];
    int len = snprintf(pid_str, sizeof(pid_str), "%d\n", getpid());
    write(fd, pid_str, len);
    close(fd);

    return false;
}

bool validateAndConvertTxtToPosNum(const char* txt, int* output) {
    char *endptr;
    *output = (int) strtol(txt, &endptr, 10);
    if(*endptr != '\0'){
        return false;
    }
    else if(*output <= 0) {
        return false;
    }
    return true;
}

void initializeState(central_state_t* state) {
    strcpy(state->bindir, DEFAULT_BINDIR);
    state->maxEstafetas = DEFAULT_ESTAFETAS;
    state->acceptanceWindow = DEFAULT_WINDOW;
}

void enrichStateWithEnv(central_state_t* state) {
    char *centralPipe = getenv("CENTRAL_PIPE");
    if(centralPipe == NULL || strlen(centralPipe) == 0) {
        printf("CENTRAL_PIPE environment variable either doesnt exist or is empty. Exiting...\n");
        cleanupAndExit(1);
    }
    strcpy(state->centralPipeName, centralPipe);

    char *bindir = getenv("BINDIR");
    if(bindir != NULL) {
        strcpy(state->bindir, bindir);
    }

    char *maxEstafetas = getenv("MAXESTAFETAS");
     if(maxEstafetas != NULL) {
        int output = -1;
        if(validateAndConvertTxtToPosNum(maxEstafetas, &output)) {
            state->maxEstafetas = output;
        }
        else {
            printf("Wrong value in MAXESTAFETAS env. var. value: non-number. Exiting...\n");
            cleanupAndExit(1);
        }
    }

    char *window = getenv("PRAZOACEITA");
     if(window != NULL) {
        int output = -1;
        if(validateAndConvertTxtToPosNum(window, &output)) {
            state->acceptanceWindow = output;
        }
        else {
            printf("Wrong value in PRAZOACEITA env. var. value: non-number. Exiting...\n");
            cleanupAndExit(1);
        }
    }
}



void enrichStateWithCmdLineArgs(central_state_t* state, int argc, char* argv[]) {
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-p") == 0 && i+1 < argc) {
            int output = -1;
            if(validateAndConvertTxtToPosNum(argv[i+1], &output)) {
                state->acceptanceWindow = output;
            }
        }

        if(strcmp(argv[i], "-m") == 0 && i+1 < argc) {
            int output = -1;
            if(validateAndConvertTxtToPosNum(argv[i+1], &output)) {
                state->maxEstafetas = output;
            }
        }
    }
}


int main(int argc, char *argv[]) {
    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);

    if(isAlreadyRunning()) {
        printf("Another central instance is already running. Closing...");
        return 1;
    }

    central_state_t state;
    initializeState(&state);
    enrichStateWithEnv(&state);
    enrichStateWithCmdLineArgs(&state, argc, argv);

    printf("Central instance started successfully. Press Ctrl+C to stop.\n");
    printf("Max estafetas: %d\n", state.maxEstafetas);
    while(keepRunning) {
        sleep(1);
    }
    cleanupAndExit(0);
    return 0;
}