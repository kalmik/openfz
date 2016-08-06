#ifndef LOGGER_H
#define LOGGER_H

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#define INFO 1
#define WARN 2
#define ERR  3
#define LOG  4

#define DEBUG_MODE 1

const char* banner();
const char* help ();
void prompt ();
void logger (int type, char* msg);

#endif
