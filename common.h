#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>


// some shared constants
#define TYPE_HEADER_SIZE 12
#define BODY_PAYLOAD_SIZE 50
#define MAX_ORIGIN_DEST 16 // +1 for \0
#define PIPE_NAME_SIZE 64
#define CENTRAL_PIPE_NAME_SIZE 256
#define ENV_CENTRAL_PIPE "CENTRAL_PIPE"

// msg type ids
#define MSG_TYPE_NEW_ORDER "pedido"
#define MSG_TYPE_RIDER_REGISTRATION "RegistoEst"
#define MSG_TYPE_ANNOUNCEMENT "anuncio"
#define MSG_TYPE_RIDER_ACCEPTS "aceitar"
#define MSG_TYPE_COMMAND "comando"
#define MSG_TYPE_RESPONSE "resposta"

// cmds for msg_cmd_estafeta_t
enum EstCmd {
    CMD_DELIVERED,
    CMD_STATUS,
    CMD_EARNINGS,
    CMD_QUIT
};

// msg reading is gonna be in two parts: reading the header and then reading the rest based on the type
// this way writing is simply just sending the structured msg
typedef struct {
    char type[TYPE_HEADER_SIZE];
} msg_header_t;

// [comms] gerador -> central - creation of new request/order by gerador (generator) to central pipe of central
typedef struct {
    char type[TYPE_HEADER_SIZE]; // "pedido" = MSG_TYPE_NEW_ORDER
    char body[BODY_PAYLOAD_SIZE];
} msg_gerador_t;

// ESTAFETA <-> CENTRAL COMMS
// [comms] rider registering to central pipe of central
typedef struct {
    char type[TYPE_HEADER_SIZE]; // "RegistoEst" = MSG_TYPE_RIDER_REGISTRATION
    int estafeta_id;
    char name[30];
    char pipeCentraltoEstafeta[PIPE_NAME_SIZE];
    char pipeEstafetatoCentral[PIPE_NAME_SIZE];
} msg_registro_estafeta_t;

// [comms] central -> estafeta - announcement/broadcast
typedef struct {
    char type[TYPE_HEADER_SIZE]; // "anuncio" = MSG_TYPE_ANNOUNCEMENT
    int request_id;
    char origin[MAX_ORIGIN_DEST];
    char destination[MAX_ORIGIN_DEST];
    double weight;
    double price;
    int time_left;
} msg_anuncio_t;

// [comms] estafeta -> central - to accept broadcasted request ... accept <id> 
typedef struct {
    char type[TYPE_HEADER_SIZE]; // "aceitar" = MSG_TYPE_RIDER_ACCEPTS
    int request_id;
} msg_accept_t;

// [comms] estafeta -> central - to send cmds (w/o parametres) like delivered / status / earnings / quit
typedef struct {
    char type[TYPE_HEADER_SIZE]; // "comando" = MSG_TYPE_COMMAND
    enum EstCmd command;
    int estafeta_id;
} msg_cmd_estafeta_t;

// [comms] central -> estafeta (command reply / shutdown notice)
typedef struct {
    char type[TYPE_HEADER_SIZE]; // "resposta" = MSG_TYPE_RESPONSE
    int success;
    char message[256];
} msg_resposta_t;

#endif /* COMMON_H */
