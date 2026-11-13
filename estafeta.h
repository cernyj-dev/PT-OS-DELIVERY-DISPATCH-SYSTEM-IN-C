#ifndef ESTAFETA_H
#define ESTAFETA_H

#include "common.h"

#define ESTAFETA_NAME_SIZE 30
#define PIPE_PREFIX "estafeta_" // for later pipe comm

// settings only, no state
typedef struct {
    char name[ESTAFETA_NAME_SIZE]; // cannot be duplicities
    char centralPipeName[CENTRAL_PIPE_NAME_SIZE];
    char pipeCentraltoEstafeta[PIPE_NAME_SIZE];
    char pipeEstafetatoCentral[PIPE_NAME_SIZE];
    int centralFd; // central pipe
    int inPipeFd; //pipeCentraltoEstafeta (read)
    int outPipeFd; // pipeEstafetatoCentral (write)
} estafeta_settings_t;

#endif /* ESTAFETA_H */
