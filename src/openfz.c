#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "mod-fis.h"
#include "logger.h"
#include "request.h"

char* commands[] = {
    "help"
};

int CPU_QTY;

void sigint_handler (int signo);

void spawn_fis (char* input, int* slot, pthread_t* fuzzy_slots, MOD_FIS_ARGS* loaddedfis, int fixed_slot);

int main(int argc, char* argv[])
{

    unsigned char daemon = 0;
    unsigned char _run = 1;
    CPU_QTY = 4;
    signal(SIGINT, sigint_handler);

    pthread_t fuzzy_slots[CPU_QTY];
    MOD_FIS_ARGS loaddedfis[CPU_QTY];
    MOD_FIS_ARGS aux_args;

    char *input;
    struct request_payload response;
    char sentence[REQ_BUFFER_SIZE];
    char log[LOGGER_BUFFER_SIZE];
    int cmd_sz = 0;

    int server_sockfd, client_sockfd;
    socklen_t server_len, client_len;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    int it;
    int aux;
    int empty_slot = 0;

    printf("%s\n", banner());

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
    freopen( "openfz.log", "w", stdout);

    logger(INFO, "OpenFZ Started.");

    for (it = 0; it < CPU_QTY; it++) {
        loaddedfis[it]  = (MOD_FIS_ARGS) {NULL, 0, NULL};
    }
    listen(server_sockfd, 5);

    client_len = sizeof(client_address);
    client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_address, &client_len);

    input = (char*) malloc(sizeof(char)*REQ_BUFFER_SIZE);
    while(_run) {
        response.status = 400;
        read(client_sockfd, input, REQ_BUFFER_SIZE);
        sprintf(response.client_inet4, "%s", inet_ntoa(client_address.sin_addr));
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

//        if (strcmp(sentence, "summary") == 0) {
//            cmd_sz = sscanf(input, "%*s %i", &aux);
//            if (cmd_sz >= 0) {
//                if (!loaddedfis[aux].mpool) continue;
//                if (loaddedfis[aux].mpool->config & DIRTY) continue;
//                response.status = 200;
//                strcpy(response.msg, summary(loaddedfis[aux].mpool));
//            } else {
//                sprintf(log, "Summary of allocated fuzzy machines\n\n"
//                        "\tSlot Port Name Type\n"
//                        "---------------------------------------\n");
//                aux = 0;
//                for (it = 0; it < CPU_QTY; it++) {
//                    if (!loaddedfis[it].mpool) continue;
//                    if (loaddedfis[it].mpool->config & DIRTY) continue;
//
//                    sprintf("\t%i %i %s %s\n", loaddedfis[it].mpool->slot,
//                           loaddedfis[it].mpool->port,
//                           loaddedfis[it].mpool->name,
//                           loaddedfis[it].mpool->type_name);
//                    aux++;
//                }
//                if (!aux)
//                    printf("There`s no fuzzy\n");
//                printf("---------------------------------------\n");
//                continue;
//            }
//        }

        else if (strcmp(sentence, "shutdown") == 0) {
            sprintf(response.msg, "END");
            response.status = 200;
            _run = 0;
        }

        if (response.status == 400) {
            sprintf(response.msg, "Command not implemented");
            logger(ERR, response.msg);
        }

        sendto(server_sockfd, &response, sizeof(struct request_payload) ,0,(struct sockaddr *) &client_address,sizeof(struct sockaddr));
        write(client_sockfd, &response, sizeof(struct request_payload));
        sprintf(log, "Response %s with status %i", response.client_inet4, response.status);
        logger(LOG, log);
    }

    close(client_sockfd);
    close(server_sockfd);

    free(&response);

    logger(INFO, "The system will shutdown now");
    free(input);
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

void sigint_handler (int signo) {
    logger(WARN, "To exit type shutdown");
    prompt();
}
