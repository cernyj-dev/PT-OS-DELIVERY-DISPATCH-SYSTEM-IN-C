#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>


#define INTMIN 2
#define INTMAX 10
#define DEFAULT_ESTAFETAS 10
#define DEFAULT_WINDOW 30
#define DEFAULT_BINDIR "."

#define MAX_ESTAFETAS 20
#define MAX_PENDING_REQ 30 // request arriving when > 30 -> discarded with a record on the administrators terminal

#define TYPE_HEADER_SIZE 12
#define BODY_PAYLOAD_SIZE 50

typedef struct {
    char type[TYPE_HEADER_SIZE];
} msg_header_t;

typedef struct {
    char type[TYPE_HEADER_SIZE];
    char body[BODY_PAYLOAD_SIZE];
} msg_gerador_t;

typedef struct {
    char type[TYPE_HEADER_SIZE];
    int estafeta_id;
    char name[30];
    char pipeCentraltoEstafeta[64];
    char pipeEstafetatoCentral[64];
} msg_registro_estafeta_t;

typedef struct {
    int id;
    char name[30];
    char pipeCentraltoEstafeta[64];
    char pipeEstafetatoCentral[64];
    int isActive;
    int currentRequestsHandled;
    double totalEarned;
} estafeta_t;

enum Status {
    PENDING,
    ASSIGNED,
    COMPLETED,
    EXPIRED,
    LOST
};

typedef struct {
    int request_id;
    char origin[20];
    char destination[20];
    double weight;
    double price;
    enum Status status;
    time_t timestamp;
} request_t;

typedef struct {
    int maxEstafetas;
    int acceptanceWindow;
    char bindir[256];
    char centralPipeName[256];

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

#endif /* COMMON_H */
