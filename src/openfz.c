#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>

#include "mod-fis.h"
#include "logger.h"
#include "request.h"

#define MAX_CONNECTION 10
#define CONNECTION_TIMEOUT 60

int CPU_QTY = 4;
pthread_mutex_t con_mtx, new_mtx;
pthread_t* fuzzy_slots;
pthread_t* clients;
MOD_FIS_ARGS* loaddedfis;
MOD_FIS_ARGS aux_args;
unsigned char _exit_ = 0;
int connection_count = 0;

char* commands[] = {
    "help"
};

typedef struct daemon_args
{
    int sockfd;
    struct sockaddr_in client;

} DAEMON_ARGS;

DAEMON_ARGS connections[MAX_CONNECTION];

void sigint_handler (int signo);

void spawn_fis (char* input, int* slot, pthread_t* fuzzy_slots, MOD_FIS_ARGS* loaddedfis, int fixed_slot);
void* work (void* args);

int main(int argc, char* argv[])
{
    CPU_QTY = 4;
//    signal(SIGINT, sigint_handler);

    char log[LOGGER_BUFFER_SIZE];

    int server_sockfd, client_sockfd;
    socklen_t server_len, client_len;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    int it;
    int aux;

    pthread_t c_monitor;

    loaddedfis = (MOD_FIS_ARGS*)malloc(sizeof(MOD_FIS_ARGS)*CPU_QTY);
    fuzzy_slots = (pthread_t*) malloc(sizeof(pthread_mutex_t)*CPU_QTY);
    clients = (pthread_t*) malloc(sizeof(pthread_t)*MAX_CONNECTION);

    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(1337);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_len = sizeof(server_address);
    aux = bind(server_sockfd, (struct sockaddr *) &server_address, server_len);
    if (aux  == -1) {
        perror("ERR");
        exit(1);
    }

    printf("%s\n", banner());
    logger(INFO, "OpenFZ Started.");
    freopen( "openfz.log", "w", stdout);

    for (it = 0; it < CPU_QTY; it++) {
        loaddedfis[it]  = (MOD_FIS_ARGS) {NULL, 0, NULL};
    }
    listen(server_sockfd, 5);

    /* Starting Daemon */
    while(!_exit_) {
        client_len = sizeof(client_address);
        client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_address, &client_len);
        connections[connection_count].client = client_address;
        connections[connection_count].sockfd = client_sockfd;
        pthread_create(&clients[connection_count], NULL, work, (void*)&connections[connection_count]);
        pthread_mutex_lock(&con_mtx);
        connection_count++;
        pthread_mutex_unlock(&con_mtx);
        sprintf(log, "New connection from %s", inet_ntoa(client_address.sin_addr));
        logger(INFO, log);
    }

    logger(INFO, "The system will shutdown now");
    return 0;
}

void spawn_fis (char* input, int* slot, pthread_t* fuzzy_slots, MOD_FIS_ARGS* loaddedfis, int fixed_slot)
{
    int it;
    int empty_slot = *slot;
    if (fixed_slot >= 0) {
        empty_slot = fixed_slot;
    } else {
        for (it  = 0; it < CPU_QTY; it++) {
            if(!loaddedfis[it].mpool) continue;
            if (loaddedfis[it].mpool->config & DIRTY) {
                empty_slot = it;
            }
        }
    }

    if (empty_slot >= CPU_QTY) {
        logger(WARN, "There`s no empty slots, type summary to show loadded fis");
        return;
    }

    loaddedfis[empty_slot] = (MOD_FIS_ARGS){
        input,
        empty_slot,
        NULL
    };

    pthread_create(&fuzzy_slots[empty_slot], NULL, runtime, &loaddedfis[empty_slot]);
    if (fixed_slot < 0)
        (*slot)++;
}

void* work (void* args)
{

    unsigned char _run = 1;
    struct request_payload request;
    struct request_payload response;
    char log[LOGGER_BUFFER_SIZE];
    char logaux[LOGGER_BUFFER_SIZE];
    char sentence[REQ_BUFFER_SIZE];
    char *input;

    int client_sockfd = ((DAEMON_ARGS*)(args))->sockfd;
    struct sockaddr_in client_address = ((DAEMON_ARGS*)(args))->client;

    int aux, it;
    int cmd_sz = 0;
    int empty_slot = 0;
    struct timeval timeout = {CONNECTION_TIMEOUT, 0};
    ssize_t rcv;

    setsockopt (client_sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
    input = (char*) malloc(sizeof(char)*REQ_BUFFER_SIZE);
    while(_run) {
        response.status = 400;
        rcv = read(client_sockfd, &request, sizeof(struct request_payload));
        if (rcv == -1) {
            logger(INFO, "Connection timeout, exiting");
            break;
        }

        if (request.status == 101) {
            continue;
        }
        sprintf(response.client_inet4, inet_ntoa(client_address.sin_addr));
        sprintf(input, request.msg);
        sprintf(log, "recive %s from %s", input, response.client_inet4);
        logger(LOG, log);

        cmd_sz = sscanf(input, "%s", sentence);
        if (!cmd_sz) {
            continue;
        }

        else if(strcmp(sentence, "help") == 0) {
            response.status = 200;
            sprintf(response.msg, help());
        }

        else if (strcmp(sentence, "loadfis") == 0) {
            spawn_fis(input, &empty_slot, fuzzy_slots, loaddedfis, -1);
            response.status = 200;
            sprintf(response.msg, "Ok");
        }

        else if (strcmp(sentence, "unloadfis") == 0) {
            cmd_sz = sscanf(input, "%*s %i", &aux);
            if (cmd_sz >= 0) {
                pthread_cancel(fuzzy_slots[aux]);
                response.status = 200;
                sprintf(response.msg, "Ok");
            } /* FIXME UNLOAD ALL*/
        }

        else if (strcmp(sentence, "reloadfis") == 0) {
            cmd_sz = sscanf(input, "%*s %i", &aux);
            if (cmd_sz >= 0) {
                aux_args = loaddedfis[aux];
                pthread_cancel(fuzzy_slots[aux]);
                /*  FIXME: when reload it sockets given connection refused */
                spawn_fis(aux_args.cmd, &empty_slot, fuzzy_slots, loaddedfis, aux_args.thread_slot);
                response.status = 200;
                sprintf(response.msg, "Ok");
            }
        }

        if (strcmp(sentence, "summary") == 0) {
            cmd_sz = sscanf(input, "%*s %i", &aux);
            if (cmd_sz >= 0) {
                if (!loaddedfis[aux].mpool) continue;
                if (loaddedfis[aux].mpool->config & DIRTY) continue;
                response.status = 200;
                sprintf(response.msg, "%s\n", summary(loaddedfis[aux].mpool));
            }
            else {
                sprintf(log, "Summary of allocated fuzzy machines\n\n"
                        "\tSlot Port Name Type\n"
                        "---------------------------------------\n");
                aux = 0;
                for (it = 0; it < CPU_QTY; it++) {
                    if (!loaddedfis[it].mpool) continue;
                    if (loaddedfis[it].mpool->config & DIRTY) continue;

                    sprintf(logaux, "\t%i %i %s %s\n", loaddedfis[it].mpool->slot,
                           loaddedfis[it].mpool->port,
                           loaddedfis[it].mpool->name,
                           loaddedfis[it].mpool->type_name);
                    strcat(log, logaux);
                    aux++;
                }
                if (!aux) {
                    sprintf(logaux, "There`s no fuzzy\n");
                    strcat(log, logaux);
                }
                sprintf(logaux, "---------------------------------------\n");
                strcat(log, logaux);
                sprintf(response.msg, log);
                response.status = 200;
            }
        }

        else if (strcmp(sentence, "shutdown") == 0) {
            sprintf(response.msg, "END");
            response.status = 200;
            _run = 0;
        }

        if (response.status == 400) {
            sprintf(response.msg, "Command not implemented");
            logger(ERR, response.msg);
        }

        write(client_sockfd, &response, sizeof(struct request_payload));
        sprintf(log, "Response %s with status %i", response.client_inet4, response.status);
        logger(LOG, log);
    }

    pthread_mutex_lock(&con_mtx);
    connection_count--;
    pthread_mutex_unlock(&con_mtx);
    close(client_sockfd);
}

void sigint_handler (int signo) {
    logger(WARN, "To exit type shutdown");
    prompt();
}