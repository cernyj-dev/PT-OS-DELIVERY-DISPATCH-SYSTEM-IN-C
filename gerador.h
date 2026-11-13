#ifndef GERADOR_H
#define GERADOR_H

#include "common.h"

#define DEFAULT_INTMIN 2
#define DEFAULT_INTMAX 10

// gerador settings
typedef struct {
    int my_id;
    int intmin;
    int intmax;
    char centralPipeName[CENTRAL_PIPE_NAME_SIZE];
} gerador_settings_t;

#endif /* GERADOR_H */
