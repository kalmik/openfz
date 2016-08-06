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
#include <readline/readline.h>
#include <readline/history.h>

#include "mod-fis.h"
#include "logger.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

char* commands[] = {
	"help"
};

int CPU_QTY;
int NEXT_SLOT = 0;
pthread_mutex_t new_fuzzy_mtx;

static void daemonize(const char* lockfile);

void sigint_handler (int signo);

void* openfz_cli (void* arg);

int main(int argc, char* argv[])
{	
	CPU_QTY = sysconf(_SC_NPROCESSORS_ONLN);
	signal(SIGINT, sigint_handler);
	printf("%s\n", banner());

	pthread_t fuzzy_slots[CPU_QTY];
	char* input, shell_prompt[100];
	char sentence[100];
	int cmd_sz = 0;
	logger(INFO, "OpenFZ_cli Started.");

	// rl_attempted_completion_function = my_completion;
	rl_bind_key('\t', rl_complete);
	while(1) {
		prompt();
		input = readline(shell_prompt);
		add_history(input);

		cmd_sz = sscanf(input, "%s", sentence);
		if (!cmd_sz) {
			continue;
		}

		if(strcmp(sentence, "help")== 0) {
			printf("%s\n", help());
			continue;
		}

		if (strcmp(sentence, "loadfis") == 0) {
			MOD_FIS_ARGS args = {
				input,
				NEXT_SLOT
			};

			NEXT_SLOT++;
			pthread_create(&fuzzy_slots[NEXT_SLOT], NULL, runtime, &args);
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
static void daemonize(const char* lockfile)
{
    /* Redirect standard files to /dev/null */
    freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
    freopen( "/dev/null", "w", stderr);
}

void sigint_handler (int signo) {
	logger(WARN, "To exit type shutdown");
	prompt();
}
