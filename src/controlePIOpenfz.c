#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <mosquitto.h>
#include <exception>
#include <iostream>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>



#include "quanser.h"
#include "timer.h"
#include "mod-fis.h"



using namespace std;

#define NTHREADS 1

Quanser *server;
double lvl1, lvl2, mv;
double sp, kp, ki, kd;
int run;

#define MSG_SIZE 200

//The library automatically reconnects to broker

string hst = "10.13.100.161"; //Borker
const char *host = hst.c_str();
int port = 1883;
string vhost = "yourvhost";
string usn = "username";
string vhusn = vhost + ":" + usn;
const char *username = vhusn.c_str();
string pwd = "password";
const char *password = pwd.c_str();
string tpc_tx = "fuzzy/tx";
const char *topic_tx = tpc_tx.c_str();
string tpc_rx = "fuzzy/rx";
const char *topic_rx = tpc_rx.c_str();


double security(double level1, double mv);
double normalize(double mv, double max);
double saturate(double mv, double max, double min);


void start();
void stop();
void setSP(double);
void setKP(double);
void setKI(double);
void setKD(double);

void onConnect(struct mosquitto *mosq, void *userdata, int result);
void onMessage(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message);

void *fuzzy(void *arg);


int main(){

    pthread_t a_thread[NTHREADS];

    int keepalive = 60;
    bool clean_session = true;
    struct mosquitto *mosq = NULL;

    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, clean_session, NULL);
    mosquitto_username_pw_set(mosq, username, password);
    mosquitto_connect_callback_set(mosq, onConnect);
    mosquitto_message_callback_set(mosq, onMessage);

    if(mosquitto_connect(mosq, host, port, keepalive)) {
        printf("Error: Failed to connect\n");
        return 1;
    }

    mosquitto_loop_start(mosq);


    pthread_create(&a_thread[0], NULL, fuzzy, NULL);

    char payload[MSG_SIZE];
    while(1){
        printf("Level1 %f, Level2 %f, Output %f\n", lvl1, lvl2, mv);

        sprintf(payload, "{\"input1\": %f, \"input2\": %f, \"output\": %f}\0", lvl1, lvl2, sp);

        try {
            mosquitto_publish(mosq, NULL, topic_tx, MSG_SIZE, payload, 0, false);
        } catch(exception& e) {
            printf("Error: Failed to publish message\n%s\n", e.what());
            return 1;
        }

        usleep(1000000);
    }
    pthread_join(a_thread[0], NULL);

    return 0;
}

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

     int _exit_ = 0;
    int sockfd;
    int len;
    struct sockaddr_in address;
    int result;

    double* request;
    double* response;

    // Fis* f = new Fis("fuzzy-pi-ga.fis");

    double* out = (double*)malloc(sizeof(double)*2);
    double* in = (double*)malloc(sizeof(double));

    double start, finish, elapsed;
    double err = 0, err_old = 0;
    double mv_old;

    sockfd  = socket(AF_INET, SOCK_STREAM,0);  // criacao do socket

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("10.13.100.161");
    address.sin_port = 3000;

    len = sizeof(address);

    result = connect(sockfd, (struct sockaddr *) &address, len);

    if (result == -1) {
        perror ("Houve erro no cliente");
        exit(1);
    }

    //sp = 15;
    //server = new Quanser("10.13.100.135", 20081);
    //run = 1;

    while(1) {

        if(!run) continue;

        GET_TIME(start);

        lvl1 = (double) server->readAD(0)*6.25;
        lvl2 = (double) server->readAD(1)*6.25;

        err = (sp - lvl2);

        in[0] = err;
        //in[0] = normalize(in[0], 30.0);
        // in[0] = saturate(in[0], 30, -30);

        in[1] = ((err - err_old) / 0.1);
        //in[1] = normalize(in[1], sp);
        // in[1] = saturate(in[1], 10, -10);

        send(sockfd, in, 2* sizeof(double),0);
        recv(sockfd, out, 1* sizeof(double), 0);

        // out = f->Evalfis(in);

        // mv_old = mv;

        // mv = ;
        mv += out[0]*0.1;

        mv = security(lvl1, mv);

        server->writeDA(0, mv);

        GET_TIME(finish);

        err_old = err;

        usleep(100000 - ((finish-start)*1000) );

    }
}


void onConnect(struct mosquitto *mosq, void *userdata, int result) {
    if (!result) {
        try {
            mosquitto_subscribe(mosq, NULL, topic_rx, 1);
            mosquitto_subscribe(mosq, NULL, "fuzzy/start", 1);
            mosquitto_subscribe(mosq, NULL, "fuzzy/stop", 1);
            mosquitto_subscribe(mosq, NULL, "fuzzy/sp", 1);
            mosquitto_subscribe(mosq, NULL, "fuzzy/kp", 1);
            mosquitto_subscribe(mosq, NULL, "fuzzy/ki", 1);
            mosquitto_subscribe(mosq, NULL, "fuzzy/kd", 1);
        } catch (exception& e) {
            printf("Error: Failed to subscribe\n%s\n", e.what());
        }
    } else {
        printf("Error: Failed to connect\n");
    }
}

void onMessage(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message) {
    if(message->payloadlen) {

        if(strcmp(message->topic, "fuzzy/sp") == 0)    setSP(atof((char*)message->payload));
        if(strcmp(message->topic, "fuzzy/kp") == 0)    setKP(atof((char*)message->payload));
        if(strcmp(message->topic, "fuzzy/ki") == 0)     setKI(atof((char*)message->payload));
        if(strcmp(message->topic, "fuzzy/kd") == 0)    setKD(atof((char*)message->payload));
        if(strcmp(message->topic, "fuzzy/start") == 0) start();
        if(strcmp(message->topic, "fuzzy/stop") == 0)  stop();


    } else {
        printf("Topic: %s, Message: (null)\n", message->topic);
    }
    fflush(stdout);
}

void start() {
    printf("Start\n");
    server = new Quanser("10.13.99.69", 20081);
    //server = new Quanser("10.13.100.135", 20081);
    run = 1;
}
void stop() {
    printf("Stop\n");
    delete server;
    run = 0;
}

void setSP(double value) {
    sp = value;
}

void setKP(double value) {
    kp = value;
    printf("kp %f\n", kp);
}

void setKI(double value) {
    ki = value;
    printf("ki %f\n", ki);
}

void setKD(double value) {
    kd = value;
    printf("kd %f\n", kd);
}