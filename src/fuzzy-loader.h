#ifndef FUZZY_LOADER
#define FUZZY_LOADER

	#define MAX_BUFFER 100
	#define SUGENO_T  2
	#define MAMDANI_T 1

	void get_type(FILE* fp, char * type);

	void get_name(FILE* fp, char * name);

	void next_line(FILE* fp);

	float* pack_io(FILE* fp, int* sz);

	float* load_input(FILE* fp, int* sz);

	float* load_output(FILE* fp, int* sz);

	int* load_rules(FILE *fp,int numInputs, int numOutputs, int numRules, int *total);

	int get_numInputs(FILE* fp);

	int get_numOutputs(FILE* fp);

	int get_numRules(FILE* fp);

#endif
