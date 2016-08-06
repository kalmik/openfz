#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "fuzzy-loader.h"
#include "fuzzy-core.h"

#include "logger.h"

#include "mod-fis.h"

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

void* runtime (void* arg)
{
	char* cmd = ((MOD_FIS_ARGS*)arg)->cmd;
	char log[100];
	int i;
	int my_slot = ((MOD_FIS_ARGS*)arg)->thread_slot;

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