#ifndef MOD_FIS_H
#define MOD_FIS_H

#include <pthread.h>

#define START_D 0
#define START 0b00000001

#define STOP 0b11111110 & START
#define DIRTY_D 7
#define DIRTY 0b10000000

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
    unsigned char invalid;

} FZ_M_POOL;

typedef struct mod_fis_args
{
	char* cmd;
	int thread_slot;
	FZ_M_POOL* mpool;

} MOD_FIS_ARGS;

typedef struct mod_fis_clean_args
{
	char* sockmsg;
	int sockfd;
	FZ_M_POOL* mpool;
} MOD_FIS_CLEAN_ARGS;

struct fis_payload {
	int size;
	double* data;
	unsigned char finish;
};

char* summary (FZ_M_POOL* mpool);
FZ_M_POOL* load_fis (char* cmd);
double* eval_fis(FZ_M_POOL* mpool, double* in);
void* runtime (void* arg);

#endif
