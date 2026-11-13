#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>

#include "central.h"
#include "common.h"

volatile sig_atomic_t keepRunning = 1;

void cleanupAndExit(int sig) {
    // const char *msg = "\ncleanupAndExit executed\n";
    // write(STDOUT_FILENO, msg, strlen(msg));
    
    // unlink (therefore delete after end of this process) our link file so future runs can run
    unlink(LOCK_FILE);
    _exit(sig);
}

void handleSignal(int sig) {
    // const char *msg = "\nhandleSignal executed";
    // write(STDOUT_FILENO, msg, strlen(msg));
    keepRunning = 0;
}

void setupSignalHandlers() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handleSignal;
    sigemptyset(&sa.sa_mask);
    // needed to clear the SA_RESTART flag which would outherwise restart fgets() aftetr CTRL+C interrupt
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction SIGINT failed");
        exit(1);
    }

    if (sigaction(SIGTERM, &sa, NULL) < 0) {
        perror("sigaction SIGTERM failed");
        exit(1);
    }
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

// === VALIDATION & INITIALIZATION ===
bool validateAndConvertTxtToPosNum(const char* txt, int* output) {
    char *endptr;
    *output = (int) strtol(txt, &endptr, 10);
    if(*endptr != '\0' || *output <= 0){
        return false;
    }
    return true;
}

void initializeState(central_state_t* state) {
    snprintf(state->bindir, sizeof(state->bindir), "%s", DEFAULT_BINDIR);
    state->maxEstafetas = DEFAULT_ESTAFETAS;
    state->acceptanceWindow = DEFAULT_WINDOW;

    state->totalReceived = 0;
    state->totalAssigned = 0;
    state->totalCompleted = 0;
    state->totalExpired = 0;
    state->totalLost = 0;
    state->totalValueCompleted = 0;

}

void enrichStateWithEnv(central_state_t* state) {
    char *centralPipe = getenv("CENTRAL_PIPE");
    if(centralPipe == NULL || strlen(centralPipe) == 0) {
        printf("[Error]: CENTRAL_PIPE environment variable either doesnt exist or is empty. Exiting...\n");
        cleanupAndExit(1);
    }
    snprintf(state->centralPipeName, sizeof(state->centralPipeName), "%s", centralPipe);

    char *bindir = getenv("BINDIR");
    if(bindir != NULL) {
        snprintf(state->bindir, sizeof(state->bindir), "%s", bindir);
    }

    char *maxEstafetas = getenv("MAXESTAFETAS");
     if(maxEstafetas != NULL) {
        int output = -1;
        if(validateAndConvertTxtToPosNum(maxEstafetas, &output)) {
            state->maxEstafetas = output;
        }
        else {
            printf("[Error]: Wrong value in MAXESTAFETAS env. var. value: non-number. Exiting...\n");
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
            printf("[Error]: Wrong value in PRAZOACEITA env. var. value: non-number. Exiting...\n");
            cleanupAndExit(1);
        }
    }
}

void enrichStateWithCmdLineArgs(central_state_t* state, int argc, char* argv[]) {
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-p") == 0 && i+1 < argc) {
            i++; // shift to its argument
            int output = -1;
            if(validateAndConvertTxtToPosNum(argv[i], &output)) {
                state->acceptanceWindow = output;
            }
            else {
                printf("[Error]: Value after -p could not be converted to a number. Exiting...\n");
                cleanupAndExit(1);
            }
        }
        else if(strcmp(argv[i], "-m") == 0 && i+1 < argc) {
            i++; // shift to its argument
            int output = -1;
            if(validateAndConvertTxtToPosNum(argv[i], &output)) {
                state->maxEstafetas = output;
            }
            else {
                printf("[Error]: Value after -m could not be converted to a number. Exiting...\n");
                cleanupAndExit(1);
            }
        }

        else {
            printf("[Error]: Unknown argument. Exiting...\n");
            cleanupAndExit(1);
        }
    }
}

void printStats(const central_state_t* state) {
    printf("Statistics:\n");
    printf("Total received: %d\n", state->totalReceived);
    printf("Total assigned: %d\n", state->totalAssigned);
    printf("Total completed: %d\n", state->totalCompleted);
    printf("Total Expired: %d\n", state->totalExpired);
    printf("Total lost: %d\n", state->totalLost);
    printf("Total value completed: %f\n", state->totalValueCompleted);
}

void tarifadorSetup(central_state_t* state, int* pipeToTarif, int* pipeFromTarif) {
    if(pipe(pipeToTarif) < 0) {
        printf("Creation of pipeToTarif failed. Exiting...\n");
        cleanupAndExit(1);
    }

    if(pipe(pipeFromTarif) < 0) {
        printf("Creation of pipeFromTarif failed. Exiting...\n");
        cleanupAndExit(1);
    }

    pid_t p = fork();
    // error
    if(p < 0) {
        perror("fork fail");
        cleanupAndExit(1);
    }
    // child
    else if(p == 0) {
        // redirecting stdin & stdout fds to point to the pipes instead of the terminal 
        dup2(pipeToTarif[0], STDIN_FILENO);
        dup2(pipeFromTarif[1], STDOUT_FILENO);
        
        // close the original pipe file descriptors in the child process
        // ensures the ref count is lowered so when central closes its ends, tarif receives EOF
        close(pipeToTarif[0]);
        close(pipeToTarif[1]);
        close(pipeFromTarif[0]);
        close(pipeFromTarif[1]);

        char tarifPath[300];
        snprintf(tarifPath, sizeof(tarifPath), "%s/tarifador", state->bindir);

        execl(tarifPath, "tarifador", (char*) NULL);

        perror("execl failed");
        _exit(1); // cleanup here doesnt make sense - parent does it

    }
    // parent
    else {
        // central wont use these ends - read end of toTarif and write end of fromTarif
        close(pipeToTarif[0]);
        close(pipeFromTarif[1]);

        // on the other hand we need these
        state->toTarifFd = pipeToTarif[1];
        state->fromTarifFd = pipeFromTarif[0];
        state->tarifPid = p;
    }
}

bool validateNoArgCmd(char* lineBuf) {
    char command[32];
    char garbage[32];
    if (sscanf(lineBuf, "%31s %31s", command, garbage) != 1) {
        return false;
    }
    return true;
}

int main(int argc, char *argv[]) {
    setupSignalHandlers();

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

    int pipeToTarif[2];
    int pipeFromTarif[2];
    tarifadorSetup(&state, pipeToTarif, pipeFromTarif);

    while(keepRunning) {
        char lineBuf[256];
        if(fgets(lineBuf, sizeof(lineBuf), stdin) == NULL) {
            // ctrl+D should just break to still end tarifador and prevent zombie 
            if (feof(stdin)) {
                break;
            }
            // after SIGINT, let the loop loop around bcs keepRunning is already 0
            if(errno == EINTR) {
                continue;
            }
            perror("fgets error");
            cleanupAndExit(1);
        }
        char command[32];
        // skip invalids
        if(sscanf(lineBuf, "%31s", command) != 1) {
            continue;
        }

        // debug cmd req: <origin> <destination> <weight>
        if(strcmp(command, "request") == 0) {
            char cmd[32];
            char origin[32];
            char destination[32];
            float weight;
            char garbage[32];
            if(sscanf(lineBuf, "%31s %31s %31s %f %31s", cmd, origin, destination, &weight, garbage) != 4) {
                printf("[Error]: Incorrect arguments used after Request command.\n Usage: request <origin> <destination> <weight>\n");
                continue;
            }
            if(weight <= 0) {
                printf("[Error]: weight argument after request cmd has non-positive value.\n Usage: request <origin> <destination> <weight>\n");
                continue;
            }
            if(strlen(origin) >= MAX_ORIGIN_DEST || strlen(destination) >= MAX_ORIGIN_DEST) {
                printf("[Error]: origin or destination argument after request cmd has exceeded 15 chars limit.\n Usage: request <origin> <destination> <weight>\n");
                continue;
            }

            char toSend[128];
            snprintf(toSend, sizeof(toSend), "%s %s %f\n", origin, destination, weight);

            if(write(state.toTarifFd, toSend, strlen(toSend)) < 0) {
                printf("Writing to tarifador pipe failed\n");
                continue;
            }

            // read the response and capture number of bytes read for manual null termination
            char toRead[256];
            ssize_t bytesRead = read(state.fromTarifFd, toRead, sizeof(toRead) - 1);
            if(bytesRead <= 0) {
                printf("Reading from tarifador pipe failed\n");
                continue;
            }
            toRead[bytesRead] = '\0';
            
            float distance;
            float valueEuros;
            if(sscanf(toRead, "%f %f", &distance, &valueEuros) == 2) {
                printf("Calculated distance: %f\nCalculated value in Euros: %f\n", distance, valueEuros);
                state.totalReceived++;
            }
        }

        else if(strcmp(command, "stats") == 0){
            if(validateNoArgCmd(lineBuf)) {
                printStats(&state);
            }
            else {
                printf("[Error]: stats command accepts no arguments. Try again\n");
            }
        }

        else if(strcmp(command, "shutdown") == 0) {
            if(validateNoArgCmd(lineBuf)) {
                printf("Shutdown command received. Shutting down...\n");
                keepRunning = 0;
            }
            else {
                printf("[Error]: shutdown command accepts no arguments. Try again\n");
            }
            
        }

        else if(strcmp(command, "requests") == 0) {
            if(validateNoArgCmd(lineBuf)) {
                printf("No active requests\n");
            }
            else {
                printf("[Error]: requests command accepts no arguments. Try again\n");
            }
        }

        else if(strcmp(command, "couriers") == 0) {
            if(validateNoArgCmd(lineBuf)) {
                printf("No connected couriers\n");
            }
            else {
                printf("[Error]: couriers command accepts no arguments. Try again\n");
            }
        }
        
        else if(strcmp(command, "window") == 0) {
            char cmd[32];
            char arg[32];
            char garbage[32];

            if(sscanf(lineBuf, "%31s %31s %31s", cmd, arg, garbage) != 2) {
                printf("[Error]: Incorrect arguments used after Window command.\n Usage: Window <n>\n");
                continue;
            }

            int output = -1;
            if(validateAndConvertTxtToPosNum(arg, &output)) {
                state.acceptanceWindow = output;
                printf("Window command processed, acceptance window has been changed to %d seconds\n", output);
            }
            else {
                printf("[Error]: Could not convert to number the argument after the Window command. Try again\n");
                continue;
            }
        }
        else {
            printf("[Error]: Unknown command\n");
        }
    }

    printf("Central shutting down\n");

    // closing the tarif pipe resulting in tarifador stdin getting an EOF 
    // as there are no write ends anymore
    close(state.toTarifFd);

    // wait until tarifador subprocess dies - prevention of creating a zombie process
    waitpid(state.tarifPid, NULL, 0);

    // kernel cleanup closes fromTarifFd write end on tarifador side
    // but we still need to close the read end on central side 
    close(state.fromTarifFd);

    cleanupAndExit(0);
    return 0;
}

/*

Unknown commands, or commands
with invalid parameters, produce an error message and do not affect the operation of the system. Every command
must give feedback on the terminal indicating whether the operation succeeded or failed.
*/