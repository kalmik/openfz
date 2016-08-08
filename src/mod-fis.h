#ifndef MOD_FIS_H
#define MOD_FIS_H

#include <pthread.h>

#define START_D 0
#define START 0b00000001 | 0b00000001

#define STOP 0b11111110 & START

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
	int slot;
	int port;
	unsigned char config;

} FZ_M_POOL;

typedef struct mod_fis_args
{
	char* cmd;
	int* thread_slot;
	pthread_mutex_t mtx;
	FZ_M_POOL* mpool;
	
} MOD_FIS_ARGS;

FZ_M_POOL* load_fis (char* cmd);
double* eval_fis(FZ_M_POOL* mpool, double* in);
void* runtime (void* arg);

#endif
