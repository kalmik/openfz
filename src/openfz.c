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
#include <time.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "fuzzy-loader.h"
#include "fuzzy-core.h"

#include "logger.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

char* commands[] = {
	"help"
};

typedef struct fz_m_pool
{
	int numInputs;
	int numOutputs;
	int numRules;
	float** inputRange;
	float** outputRange;
	int* rules;
	char* type_name;
	char* name;

} FZ_M_POOL;

int CPU_QTY;
int NEXT_SLOT = 0;
pthread_mutex_t new_fuzzy_mtx;

static void daemonize(const char* lockfile);
// void logger (int type, char* msg);
// const char* banner ();
// const char* help ();
// void prompt ();

void sigint_handler (int signo);

void* openfz_cli (void* arg);
void* runtime (void* arg);

double* eval_fis(FZ_M_POOL* mpool, double* in);
FZ_M_POOL* load_fis (char* cmd);

int main(int argc, char* argv[])
{	
	CPU_QTY = sysconf(_SC_NPROCESSORS_ONLN);
	signal(SIGINT, sigint_handler);
	int* cli_status = malloc(sizeof(int));
	*cli_status = 0;
	printf("%s\n", banner());

	pthread_t openfz_cli_handler;
	
	pthread_create(&openfz_cli_handler, NULL, openfz_cli, NULL);

	pthread_join(openfz_cli_handler, (void**)&cli_status);
	if( *cli_status == 1) {
		logger(INFO, "The system will shutdown now");
		return 0;
	}
	daemonize("openfzd");


	return 0;
}

/* @openfz_cli */
void* openfz_cli (void* arg)
{
	pthread_t fuzzy_slots[CPU_QTY];
	char* input, shell_prompt[100];
	char sentence[100];
	int* status = malloc(sizeof(int));
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
			pthread_create(&fuzzy_slots[NEXT_SLOT], NULL, runtime, input);
			continue;
		}

		/*
		 * FIXME: Demonize()
		if (strcmp(sentence, "quit") == 0) {
			logger(INFO, "The system run in background now");
			*status = 0;
			break;
		}
		 */

		if (strcmp(sentence, "shutdown") == 0) {
			*status = 1;
			break;
		}

		logger(ERR, "Command not implemented");

	}

	pthread_exit((void*)status);
}

FZ_M_POOL* load_fis (char* cmd)
{
	char path[100];
	int argc, i;
	char log[400];
	FILE* fp;

	FZ_M_POOL* mpool;
	mpool = (struct fz_m_pool*) malloc(sizeof(struct fz_m_pool));

	argc = sscanf(cmd, "%*s %s", path);
	if (argc < 1) {
		logger(ERR, "usage: loadfis <filename>. type help to see more informations");
		return NULL;
	}

	fp = fopen(path, "r");
	if (!fp) {
		sprintf(log, "%s, no such file", path);
		logger(ERR, log);
		return NULL;
	}
	sprintf(log, "<%s> Loading", path);
	logger(LOG, log);

	mpool->name = (char*) malloc(sizeof(char)*100);
	get_name(fp, mpool->name);

	mpool->type_name = (char*) malloc(sizeof(char)*10);
	get_type(fp, mpool->type_name);

	sprintf(log, "<%s> Fuzzy system type %s", path, mpool->type_name);
	logger(LOG, log);

	mpool->numInputs = get_numInputs(fp);
	mpool->numOutputs = get_numOutputs(fp);
	mpool->numRules = get_numRules(fp);
	
	sprintf(log, "<%s> Number of Inputs %i", path, mpool->numInputs);
	logger(LOG, log);
	
	sprintf(log, "<%s> Number of Outputs %i", path, mpool->numOutputs);
	logger(LOG, log);
	
	sprintf(log, "<%s> Number of Rules %i", path, mpool->numRules);
	logger(LOG, log);

	mpool->inputRange = (float**)malloc(sizeof(float*)*mpool->numInputs);
	for(i = 0; i < mpool->numInputs; i++){
		mpool->inputRange[i] = load_input(fp, NULL);
	}

	mpool->outputRange = (float**)malloc(sizeof(float*)*mpool->numOutputs);
	for(i = 0; i < mpool->numOutputs; i++)
		mpool->outputRange[i] = load_output(fp, NULL);

	mpool->rules = load_rules(fp, mpool->numInputs, mpool->numOutputs, mpool->numRules, NULL);
	
	fclose(fp);
	
	sprintf(log, "<%s> Closing file", path);
	logger(LOG, log);

	return mpool;
}

double* eval_fis(FZ_M_POOL* mpool, double* in)
{

	int i, j, k, jump, cur_MF_sz;
	float* cur_MF;
	double *fuzzyValues, *outValues, *cur_value_v, *out;

	int rules_sz_ln = 1 + mpool->numInputs + mpool->numOutputs;

	fuzzyValues = (double*)malloc(sizeof(double)*mpool->numRules);
	outValues = (double*)malloc(sizeof(double)*mpool->numOutputs*2);
	cur_value_v = (double*)malloc(sizeof(double)*mpool->numInputs);

	out = (double*)malloc(sizeof(double)*mpool->numOutputs);

	for(i = 0; i < mpool->numOutputs*2; i++){
		outValues[i] = 0;
	}

	for(i = 0; i < mpool->numRules; i++){

		for(j = 0; j < mpool->numInputs; j++){

			cur_MF_sz = (int)(mpool->inputRange[j][mpool->rules[i*rules_sz_ln+j]]);
			cur_MF = (float*)malloc(sizeof(float)*cur_MF_sz);

			jump = 0;
			for(k = 0; k < mpool->rules[i*rules_sz_ln+j]; k++)
				jump += (int)(mpool->inputRange[j][k]);

			for(k = 0; k < cur_MF_sz; k++){
				cur_MF[k] = mpool->inputRange[j][k+jump+1];
			}

			cur_value_v[j] = fuzzify(in[j], cur_MF, cur_MF_sz);
			free(cur_MF);
		}

		//Implications

		fuzzyValues[i] = cur_value_v[0];
		for(j = 1; j < mpool->numInputs; j ++){

			if(mpool->rules[i*rules_sz_ln+rules_sz_ln-1] == 1)
				fuzzyValues[i] = andOp(cur_value_v[j], fuzzyValues[i]);

			else if(mpool->rules[i*rules_sz_ln+rules_sz_ln-1] == 2)
				fuzzyValues[i] = orOp(cur_value_v[j], fuzzyValues[i]);
		}

		//Aggregations

		int l = 0;
		for(j = mpool->numInputs; j < mpool->numOutputs + mpool->numInputs; j++){

			cur_MF_sz = (int)(mpool->outputRange[j - mpool->numInputs][mpool->rules[i*rules_sz_ln+j]]);
			cur_MF = (float*)malloc(sizeof(float)*cur_MF_sz);

			jump = 0;
			for(k = 0; k < mpool->rules[i*rules_sz_ln+j]; k++)
				jump += (int)(mpool->outputRange[j - mpool->numInputs][k]);

			for(k = 0; k < cur_MF_sz; k++){
				cur_MF[k] = mpool->outputRange[j - mpool->numInputs][k+jump+1];
			}

			if(strcmp(mpool->type_name, "mamdani") == 0){
			 	defuzzify(fuzzyValues[i], cur_MF, cur_MF_sz, &outValues[l], &outValues[l+1]);
			} else if(strcmp(mpool->type_name, "sugeno") == 0){
				defuzzify_sugeno(in, fuzzyValues[i], cur_MF, cur_MF_sz, &outValues[l], &outValues[l+1]);
			}

			l += 2;
			free(cur_MF);
		}

	}

	for (i = 0; i < mpool->numOutputs*2; i += 2 )
	{
		if (outValues[i] == 0) {
			out[i] = 0;
			continue;
		}
		out[i] = outValues[i]/outValues[i+1];
	}

	return out;
}

void* runtime (void* arg) {
	char* cmd = (char*) arg;
	char log[100];
	int i;
	int my_slot = NEXT_SLOT;

	double* out;
	double* in;

	/* Socket variables */
	int sockfd, client_sockfd;
	socklen_t len, client_len;
	struct sockaddr_in address;
	struct sockaddr_in client_address;
	int socket_message_len;
	char* socket_message;

	/* Fuzzy loading */
	sprintf(log, "The System is trying to run on slot %i", my_slot);
	logger(LOG, log);

	FZ_M_POOL* mpool = load_fis(cmd);
	if (!mpool) {
		pthread_exit(NULL);
	}
	logger(LOG, log);

	sprintf(log, "The System is ready on slot %i", my_slot);
	pthread_mutex_lock(&new_fuzzy_mtx);
	NEXT_SLOT++;
	pthread_mutex_unlock(&new_fuzzy_mtx);

	out = (double*) malloc(sizeof(double)*mpool->numOutputs);
    in = (double*) malloc(sizeof(double)*mpool->numInputs);
    /* end loading */

    /* Socket creating */
    socket_message_len = 1024;
    socket_message = (char*) malloc(sizeof(char)*socket_message_len);

    sockfd  = socket(AF_INET, SOCK_STREAM,0);
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr("127.0.0.1");
	address.sin_port = 3000 + my_slot;
	len = sizeof(address);
	bind(sockfd, (struct sockaddr *) &address, len);
  	listen(sockfd, 5);
  	sprintf(log, "This fuzzy machine is bond to port %i", address.sin_port);
  	logger(INFO, log);
	/* socket created */

	while(1) {
		logger(INFO, "Waiting data...");
		client_sockfd = accept(sockfd, (struct sockaddr *) &client_address, &client_len);
		read(client_sockfd, socket_message, socket_message_len);
		for(i = 0; i < mpool->numInputs; i++) {
			sscanf(socket_message, "%lf ", &in[i]);
		}

	    out = eval_fis(mpool, in);

	    sprintf(log, "<%s> Inputs %.3f, %.3f",mpool->name, in[0], in[1]);
	    logger(LOG, log);
	    for (i = 0; i < mpool->numOutputs; i++) {
	    	sprintf(log, "<%s> Output[%i] %.3f", mpool->name, i, out[i]);
	    	write(client_sockfd, log, socket_message_len);
	    	logger(LOG, log);
	    }
	    close(client_sockfd);
	}

	free(socket_message);
	free(mpool);
	close(sockfd);
}

/*
 * FIXME: Set this function work
 */
static void daemonize(const char* lockfile)
{
	char msg[20];
    pid_t pid, sid;
    int lfp;
    /* already a daemon */
    if ( getppid() == 1 ) return;

    /* Create the lock file as the current user */
    if ( lockfile && lockfile[0] ) {
        lfp = open(lockfile,O_RDWR|O_CREAT,0640);
        if ( lfp < 0 ) {
            exit(EXIT_FAILURE);
        }
    }

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then we can exit the parent process. */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    /* At this point we are executing as the child process */

    /* Change the file mode mask */
    umask(0);

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }

    sprintf(msg, "PID %i", sid);
    logger(INFO, msg);

    /* Redirect standard files to /dev/null */
    freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
    freopen( "/dev/null", "w", stderr);
}

void sigint_handler (int signo) {
	logger(WARN, "To exit type shutdown");
	prompt();
}
