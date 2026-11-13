#ifndef CENTRAL_H
#define CENTRAL_H

#include <time.h>
#include "common.h"

#define DEFAULT_ESTAFETAS 10
#define MAX_ESTAFETAS 20
#define DEFAULT_WINDOW 30
#define MAX_PENDING_REQ 30 // request arriving when > 30 -> discarded with a record on the administrators terminal
#define DEFAULT_BINDIR "."
#define LOCK_FILE "./central.lock" // to ensure no two centrals running at the same time

// estafeta struct to track its properties, including the name, pipes and status
typedef struct {
    int id;
    char name[30];
    char pipeCentraltoEstafeta[PIPE_NAME_SIZE];
    char pipeEstafetatoCentral[PIPE_NAME_SIZE];
    int isActive;
    int currentRequestsHandled;
    double totalEarned;
} estafeta_t;

// status enum for request_t
enum Status {
    UNASSIGNED,
    IN_PROGRESS,
    COMPLETED,
    EXPIRED,
    LOST
};

// request struct to track its properties
typedef struct {
    int request_id;
    char origin[MAX_ORIGIN_DEST];
    char destination[MAX_ORIGIN_DEST];
    double weight;
    double price;
    enum Status status;
    time_t timestamp;
} request_t;

// overall central state
typedef struct {
    int maxEstafetas;
    int acceptanceWindow;
    char bindir[256];
    char centralPipeName[CENTRAL_PIPE_NAME_SIZE];

    pid_t tarifPid;
    int toTarifFd;
    int fromTarifFd;

    int totalReceived;
    int totalAssigned;
    int totalCompleted;
    int totalExpired;
    int totalLost;
    double totalValueCompleted;

    request_t requests[MAX_PENDING_REQ];
    estafeta_t couriers[MAX_ESTAFETAS];
} central_state_t;

#endif /* CENTRAL_H */
