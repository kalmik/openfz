#ifndef MOD_FIS_H
#define MOD_FIS_H

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

typedef struct mod_fis_args
{
	char* cmd;
	int thread_slot;
	
} MOD_FIS_ARGS;

FZ_M_POOL* load_fis (char* cmd);
double* eval_fis(FZ_M_POOL* mpool, double* in);
void* runtime (void* arg);

#endif
