#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "mod-fis.h"
#include "logger.h"

#define SOCK_NAME "openfz.sock"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

char* commands[] = {
    "help"
};

int CPU_QTY;
int NEXT_SLOT = 0;
pthread_mutex_t new_fuzzy_mtx;

static void daemonize();

void sigint_handler (int signo);

void* openfz_cli (void* arg);

void spawn_fis (char* input, int* slot, pthread_t* fuzzy_slots, MOD_FIS_ARGS* loaddedfis, int fixed_slot);

int main(int argc, char* argv[])
{

    unsigned char daemon = 0;
    CPU_QTY = sysconf(_SC_NPROCESSORS_ONLN);
    CPU_QTY = 4;
    signal(SIGINT, sigint_handler);

    pthread_t fuzzy_slots[CPU_QTY];
    MOD_FIS_ARGS loaddedfis[CPU_QTY];
    MOD_FIS_ARGS aux_args;

    char *input;
    char sentence[LOGGER_BUFFER_SIZE];
    int cmd_sz = 0;

    int server_sockfd, client_sockfd;
    socklen_t server_len, client_len;
    struct sockaddr_un server_address;
    struct sockaddr_un client_address;
    int it;
    int aux;
    int empty_slot = 0;

    if (argc > 1) {
        if (strcmp(argv[1], "-d") == 0) {
            daemon = 1;
            daemonize();
            unlink("openfz.log");
            mkfifo("openfz.log", 0666);
            input = (char*) malloc(sizeof(char)*CPU_QTY);

            unlink(SOCK_NAME);
            server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

            server_address.sun_family = AF_UNIX;
            strcpy(server_address.sun_path, SOCK_NAME);
            server_len = sizeof(server_address);
            bind(server_sockfd, (struct sockaddr *) &server_address, server_len);

            listen(server_sockfd, 5);
        }
    } else {
        printf("%s\n", banner());
        rl_bind_key('\t', rl_complete);
    }

    logger(INFO, "OpenFZ Started.");

    for (it = 0; it < CPU_QTY; it++) {
        loaddedfis[it]  = (MOD_FIS_ARGS) {NULL, 0, NULL};
    }

    while(1) {
        prompt();
        if (daemon) {
            client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_address, &client_len);
            read(client_sockfd, input, LOGGER_BUFFER_SIZE);
        } else {
            input = readline(NULL);
        }

        cmd_sz = sscanf(input, "%s", sentence);
        if (!cmd_sz) {
            continue;
        }

        if(strcmp(sentence, "help") == 0) {
            printf("%s\n", help());
            continue;
        }

        if (strcmp(sentence, "loadfis") == 0) {
            spawn_fis(input, &empty_slot, fuzzy_slots, loaddedfis, -1);
            continue;
        }

        if (strcmp(sentence, "unloadfis") == 0) {
            cmd_sz = sscanf(input, "%*s %i", &aux);
            if (cmd_sz >= 0) {
                pthread_cancel(fuzzy_slots[aux]);
            } /* FIXME else {
                cmd_sz = sscanf(input, "%*s %s", sentence);
                if (strcmp(sentence, "all") == 0) {
                    for (it = 0; it <  CPU_QTY; it++) {
                        if (!loaddedfis[it].mpool) continue;
                        pthread_cancel(fuzzy_slots[it]);
                    }
                }
            } */
            continue;
        }

        if (strcmp(sentence, "reloadfis") == 0) {
            cmd_sz = sscanf(input, "%*s %i", &aux);
            if (cmd_sz >= 0) {
                aux_args = loaddedfis[aux];
                pthread_cancel(fuzzy_slots[aux]);
                /*  FIXME: when reload it sockets given connection refused */
                spawn_fis(aux_args.cmd, &empty_slot, fuzzy_slots, loaddedfis, aux_args.thread_slot);
            }
            continue;
        }

        if (strcmp(sentence, "summary") == 0) {
            cmd_sz = sscanf(input, "%*s %i", &aux);
            if (cmd_sz >= 0) {
                if (!loaddedfis[aux].mpool) continue;
                summary(loaddedfis[aux].mpool);
                continue;
            }
            printf("Summary of allocated fuzzy machines\n\n");
            printf("\tSlot Port Name Type\n");
            printf("---------------------------------------\n");
            aux = 0;
            for (it = 0; it < CPU_QTY; it++) {
                if(!loaddedfis[it].mpool) continue;
                if (loaddedfis[it].mpool->config & DIRTY) continue;

                printf("\t%i %i %s %s\n", loaddedfis[it].mpool->slot, \
                    loaddedfis[it].mpool->port, \
                    loaddedfis[it].mpool->name, \
                    loaddedfis[it].mpool->type_name);
                aux++;
            }
            if(!aux)
                printf("There`s no fuzzy\n");
            printf("---------------------------------------\n");
            continue;
        }

        if (strcmp(sentence, "shutdown") == 0) {
            break;
        }

        logger(ERR, "Command not implemented");

    }

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

/*
 * FIXME: Set this function work
 */
 static void daemonize()
 {
    freopen( "openfz.log", "w", stdout);
}

void sigint_handler (int signo) {
    logger(WARN, "To exit type shutdown");
    prompt();
}
