/**
 * @file    receiver.c
 *
 * @date    2018-12-11
 *
 * @author  Leslie
 *
 * @brief   Receive message by shared memory. Communicate between two processes with shared memory.
 */

#include "common.h"

int main(int argc, char **argv) {
    full = sem_open("shm_sgn_full", O_CREAT | O_RDWR, 0666, 0);
    empty = sem_open("shm_sgn_empty", O_CREAT | O_RDWR, 0666, 1);
    rcv_to_snd = sem_open("shm_sgn_rts", O_CREAT | O_RDWR, 0666, 0);
    void *shm = NULL;
    struct sm_st *shared;
    int shmid, count = 0, must_stop = 0;
    key_t sm_id = ftok("tok", 1);

    shmid = shmget(sm_id, sizeof(struct sm_st), 0666 | IPC_CREAT);
    CHECK(shmid >= 0);
    shm = shmat(shmid, 0, 0);
    CHECK(shm >= 0);
    shared = (struct sm_st *) shm;

    do {
        P(full);
        if (!strncmp(shared->data, MSG_STOP, strlen(MSG_STOP))) {
            must_stop = 1;
        } else {
            count++;
            printf("[%d]\n", count);
            printf("[*]receive: %s\n", shared->data);
        }
        V(empty);
    } while (!must_stop);

    P(empty);
    strcpy(shared->data, "over");
    V(rcv_to_snd);
    V(full);

    CHECK(shmdt(shared) == 0);
    CHECK(sem_close(full) == 0);
    CHECK(sem_close(empty) == 0);
    CHECK(sem_close(rcv_to_snd) == 0);

    return 0;
}