//
// Created by leslie on 18-12-18.
//

#ifndef OPERATOR_SYSTEM_EXP3_3_FAKE_COMMON_H
#define OPERATOR_SYSTEM_EXP3_3_FAKE_COMMON_H

#endif //OPERATOR_SYSTEM_EXP3_3_FAKE_COMMON_H
//
// Created by leslie on 18-12-11.
//

#ifndef EXP3_3_COMMON_H
#define EXP3_3_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sys/msg.h>
#include <pthread.h>
#include <semaphore.h>

#define QUEUE_ID    10086
#define MAX_SIZE    1024
#define MSG_STOP    "exit"
#define snd_to_rcv1 1
#define snd_to_rcv2 2
#define rcv_to_snd1 3
#define rcv_to_snd2 4
#define CHECK(x) \
    do { \
        if (!(x)) { \
            fprintf(stderr, "%s:%d: ", __func__, __LINE__); \
            perror(#x); \
            exit(-1); \
        } \
    } while (0) \

#define P(x) sem_wait(&x)
#define V(x) sem_post(&x)
struct msg_st {
    long int message_type;
    char buffer[MAX_SIZE + 1];
};

/* function */
void *sender1();
void *sender2();
void *receiver();

/* global variable */
sem_t rcv_mutex, snd_mutex;

#endif //EXP3_3_COMMON_H
