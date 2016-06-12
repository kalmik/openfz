#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
// #include <mosquitto.h>
#include <exception>
#include <iostream>
#include <pthread.h>
#include "quanser.h"
#include "timer.h"
#include "fis.h"


using namespace std;

#define NTHREADS 1

Quanser *server;
double lvl1, lvl2, mv;

double security(double level1, double mv) {

    //trava de saturacao de setpoint
    if(mv > 4)  mv = 4;
    if(mv < -4) mv = -4;
    //trava de nivel alto -  calibrar 28 e 3.15
    if(level1 > 28 && mv > 3.15) mv = 3.15;
    //trava nivel muito alto
    if(level1 > 29) mv = 0;
    //]trava de nivel baixo
    if(level1 < 3 && mv < 0) mv = 0;

    return mv;

}
double normalize(double mv, double max) {
    return mv/max;
}
double saturate(double mv, double max, double min) {
    if(mv > max) return max;
    if(mv < min) return min;

    return mv;
}

void *fuzzy(void *arg) {

    Fis* f = new Fis("fuzzy-pi-ga.fis");
    // Fis* f = new Fis("b7.fis");

    double* out = (double*)malloc(sizeof(double)*f->numOutputs);
    double* in = (double*)malloc(sizeof(double)*f->numInputs);

    double start, finish, elapsed;
    double err = 0, err_old = 0;
    double mv_old = 0;

    while(1) {

        GET_TIME(start);

        lvl1 = (double) server->readAD(0)*6.25;
        lvl2 = (double) server->readAD(1)*6.25;

        err = (15.0 - lvl2);

        in[0] = err;
        // in[0] = -1;
        // in[0] = normalize(in[0], 30.0);
        // in[0] = saturate(in[0], 1, -1);

        in[1] = (err - err_old) / 0.1;
        // in[1] = -1;
        // in[1] = normalize(in[1], 1.0);
        in[1] = saturate(in[1], 10, -10);

        out = f->Evalfis(in);

        mv += out[0]*0.1;

        mv = security(lvl1, mv);
        server->writeDA(0, mv);

        err_old = err;
        //mv_old = mv;
        GET_TIME(finish);

        int st = usleep(100000 - (finish - start)*1000 );

    }
}

int main(){

    pthread_t a_thread[NTHREADS];


    server = new Quanser("10.13.99.69", 20081);
    pthread_create(&a_thread[0], NULL, fuzzy, NULL);
    while(1){
        printf("Level1 %f, Level2 %f, Output %f\n", lvl1, lvl2, mv);
        usleep((int)(0.1*1000000));
    }
    pthread_join(a_thread[0], NULL);

    return 0;
}

