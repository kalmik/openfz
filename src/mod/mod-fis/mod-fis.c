#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>

#include "include/fuzzy-loader.h"
#include "include/fuzzy-core.h"

#include "../../include/logger.h"
#include "../../include/request.h"

#include "include/mod-fis.h"

#define BASE_PATH "conf/fis/%s"

char* summary (FZ_M_POOL* mpool) {
    char* summ = (char*)malloc(sizeof(char)*REQ_BUFFER_SIZE);
    sprintf(summ,
            "\tName\t %s\n"
            "\tType\t %s\n"
            "\tInputs\t %i\n"
            "\tOutputs\t %i\n"
            "\tRules\t %i\n",
            mpool->name, mpool->type_name, mpool->numInputs,
            mpool->numOutputs, mpool->numRules);

    return summ;
}

FZ_M_POOL* load_fis (char* cmd)
{
    char path[LOGGER_BUFFER_SIZE];
    char full_path[LOGGER_BUFFER_SIZE];
    int argc, i;
    char log[LOGGER_BUFFER_SIZE];
    FILE* fp;

    FZ_M_POOL* mpool;
    mpool = (struct fz_m_pool*) malloc(sizeof(struct fz_m_pool));
    sprintf(mpool->client_inet4, "");
    argc = sscanf(cmd, "%*s %s", path);
    if (argc < 1) {
        logger(ERR, "usage: loadfis <filename>. type help to see more informations");
        return NULL;
    }
    sprintf(full_path, BASE_PATH, path);
    fp = fopen(full_path, "r");
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

    mpool->config = START;
    mpool->invalid = 0;

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

    free(fuzzyValues);
    free(outValues);
    free(cur_value_v);

    return out;
}

static void cleanup_fis (void* arg) {
    char log[LOGGER_BUFFER_SIZE];

    char* sockmsg =( (MOD_FIS_CLEAN_ARGS*) arg)->sockmsg;
    FZ_M_POOL* mpool =  ( (MOD_FIS_CLEAN_ARGS*) arg)->mpool;
    int sockfd = ( (MOD_FIS_CLEAN_ARGS*) arg)->sockfd;

    sprintf(log, "<%s> Cleaning up", ((MOD_FIS_CLEAN_ARGS*)arg)->mpool->name);
    logger(INFO,log);

    mpool->config |= DIRTY;
    mpool->invalid = 1;

    free(sockmsg);
    free(mpool);
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);

    sprintf(log, "<%s> Exited", ((MOD_FIS_CLEAN_ARGS*)arg)->mpool->name);
    logger(INFO,log);

}

void* runtime (void* arg)
{
    unsigned char _exit_ = 0;

    char* cmd = ((MOD_FIS_ARGS*)arg)->cmd;
    char log[LOGGER_BUFFER_SIZE];
    int i;
    int my_slot = ((MOD_FIS_ARGS*)arg)->thread_slot;
    FZ_M_POOL* mpool;

    /* Socket variables */
    int sockfd, client_sockfd;
    socklen_t len, client_len;
    struct sockaddr_in address;
    struct sockaddr_in client_address;
    int socket_message_len;
    char* socket_message;

    double* request;
    double* response;
    ssize_t request_sz = 0;

    /* Fuzzy loading */
    sprintf(log, "mod-fis.c slot %i, start", my_slot);
    logger(LOG, log);

    mpool = load_fis(cmd);
    ((MOD_FIS_ARGS*)arg)->mpool = mpool;
    if (!mpool) {
        pthread_exit(NULL);
    }

    mpool->slot = my_slot;
    mpool->port = mpool->slot+3000;

    sprintf(log, "mod-fis.c %s slot %i ready", mpool->name, my_slot);
    logger(LOG, log);

    /* end loading */

    /* Socket creating */
    socket_message_len = 1024;
    socket_message = (char*) malloc(sizeof(char)*socket_message_len);

    sockfd  = socket(AF_INET, SOCK_STREAM,0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = (in_port_t)mpool->port;
    len = sizeof(address);
    bind(sockfd, (struct sockaddr *) &address, len);
    listen(sockfd, 5);
    sprintf(log, "mod-fis.c %s slot %i listen on port %i", mpool->name, my_slot, address.sin_port);
    logger(INFO, log);
    /* socket created */

    MOD_FIS_CLEAN_ARGS clean = {
        socket_message,
        sockfd,
        mpool
    };


    request = (double*) malloc(sizeof(double)*mpool->numInputs);
    response = (double*) malloc(sizeof(double)*mpool->numOutputs);

    pthread_cleanup_push(cleanup_fis, &clean);

    signal(SIGPIPE, SIG_IGN);
    while (!_exit_) {
        client_sockfd = accept(sockfd, (struct sockaddr *) &client_address, &client_len);
        sprintf(log, "mod-fis.c %s slot %i connection stablished to %s", mpool->name, my_slot, inet_ntoa(address.sin_addr));
        logger(INFO, log);
        sprintf(mpool->client_inet4, inet_ntoa(client_address.sin_addr));
        while (client_sockfd >= 0) {
            request_sz = recv(client_sockfd, request, mpool->numInputs * sizeof(double), 0);

            if (!request_sz) break;

            response = eval_fis(mpool, request);

            send(client_sockfd, response, mpool->numOutputs * sizeof(double), 0);
        }
        sprintf(mpool->client_inet4, "");
    }

    pthread_cleanup_pop(0);

    return NULL;
}