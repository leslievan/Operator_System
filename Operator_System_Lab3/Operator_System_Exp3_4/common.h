//
// Created by leslie on 18-12-11.
//

#ifndef EXP3_4_COMMON_H
#define EXP3_4_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <fcntl.h>
#include <string.h>

#define FULL_PATH "shm_sgn_full"
#define EMPTY_PATH "shm_sgn_empty"
#define RTS_PATH "shm_sgn_rts"
#define MSG_STOP "exit"
#define MAX_SIZE 1024
#define CHECK(x) \
    do { \
        if (!(x)) { \
            fprintf(stderr, "%s:%d: ", __func__, __LINE__); \
            perror(#x); \
            exit(-1); \
        } \
    } while (0) \

#define P(x) sem_wait(x)
#define V(x) sem_post(x)
struct sm_st {
    char data[MAX_SIZE];
};

sem_t *full, *empty, *rcv_to_snd;

#endif //EXP3_4_COMMON_H
