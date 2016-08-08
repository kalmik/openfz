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

int main(int argc, char* argv[])
{	

	unsigned char daemon = 0;
	CPU_QTY = sysconf(_SC_NPROCESSORS_ONLN);
	signal(SIGINT, sigint_handler);
	
	pthread_t fuzzy_slots[CPU_QTY];
	MOD_FIS_ARGS loaddedfis[CPU_QTY];

	char input[LOGGER_BUFFER_SIZE];
	char sentence[LOGGER_BUFFER_SIZE];
	int cmd_sz = 0;

	int server_sockfd, client_sockfd;
	socklen_t server_len, client_len;
	struct sockaddr_un server_address;
	struct sockaddr_un client_address;
	int it;
	int aux;

	if (argc > 1) {
		if (strcmp(argv[1], "-d") == 0) {
			daemon = 1;
			//daemonize();
			unlink("openfz.log");
			mkfifo("openfz.log", 0666);

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
	
	while(1) {
		prompt();
		if (daemon) {
			client_sockfd = accept(server_sockfd, (struct sockaddr *) &client_address, &client_len);
			read(client_sockfd, input, LOGGER_BUFFER_SIZE);
		} else {
			scanf("%[aA-zZ0-9 ./-_]+", input);
			fseek(stdin, 0, SEEK_END);
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
			if (NEXT_SLOT >= CPU_QTY) {
				logger(WARN, "There`s no empty slots, type summary to show loadded fis");
				continue;
			}

			loaddedfis[NEXT_SLOT] = (MOD_FIS_ARGS){
				input,
				&NEXT_SLOT,
				new_fuzzy_mtx,
				NULL
			};

			pthread_create(&fuzzy_slots[NEXT_SLOT], NULL, runtime, &loaddedfis[NEXT_SLOT]);
			continue;
		}

		if (strcmp(sentence, "unloadfis") == 0) {
			cmd_sz = sscanf(input, "%*s %i", &aux);
			if (cmd_sz >= 0) {
				pthread_kill(fuzzy_slots[aux], SIGTERM);
			}
			continue;
		}

		if (strcmp(sentence, "summary") == 0) {
			cmd_sz = sscanf(input, "%*s %i", &aux);
			if (cmd_sz >= 0) {
				if (aux >= NEXT_SLOT) continue;
				summary(loaddedfis[aux].mpool);
				continue;
			}
			printf("Summary of allocated fuzzy machines\n\n");
			printf("Slot Port Name Type\n");
			printf("---------------------------------------\n");
			for (it = 0; it < NEXT_SLOT; it++) {
				printf("%i %i %s %s\n", loaddedfis[it].mpool->slot, \
										loaddedfis[it].mpool->port, \
										loaddedfis[it].mpool->name, \
					                    loaddedfis[it].mpool->type_name);
			}
			if(!NEXT_SLOT)
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
	return 0;
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
