#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <exception>
#include <iostream>
#include <pthread.h>
#include "fis.h"
#include "quanser.h"

using namespace std;


int main(){

    Fis* f = new Fis("fuzzy-pi-ga.fis");


    double* out = (double*)malloc(sizeof(double)*f->numOutputs);
    double* in = (double*)malloc(sizeof(double)*f->numInputs);

    srand(time(NULL));
    printf("Hello\n");

    while(1) {
        in[0] = -1;
        in[1] = -1;
        out = f->Evalfis(in);

        printf("{\"input1\": %f, \"input2\": %f, \"output\": %f}\n", in[0], in[1], out[0]);

        usleep(1000000);

    }

    return 0;
}

