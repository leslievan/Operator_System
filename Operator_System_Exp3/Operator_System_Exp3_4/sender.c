/**
 * @file    receiver.c
 *
 * @date    2018-12-11
 *
 * @author  Leslie
 *
 * @brief   Send message by shared memory.
 */

#include "common.h"

int main() {
    full = sem_open(FULL_PATH, O_CREAT | O_RDWR, 0666, 0);
    empty = sem_open(EMPTY_PATH, O_CREAT | O_RDWR, 0666, 1);
    rcv_to_snd = sem_open(RTS_PATH, O_CREAT | O_RDWR, 0666, 0);
    void *shm = NULL;
    struct sm_st *shared;
    int shmid, count = 0;
    key_t sm_id = ftok("tok", 1);

    shmid = shmget(sm_id, sizeof(struct sm_st), 0666 | IPC_CREAT);
    CHECK(shmid >= 0);
    shm = shmat(shmid, 0, 0);
    CHECK(shm >= 0);
    shared = (struct sm_st *) shm;

    do {
        P(empty);
        printf("sender> ");
        fflush(stdout);
        memset(shared->data, 0, MAX_SIZE);
        fgets(shared->data, MAX_SIZE, stdin);
        V(full);
    } while (strncmp(shared->data, MSG_STOP, strlen(MSG_STOP)));

    P(rcv_to_snd);
    P(full);
    printf("---------------------------\n");
    printf("receive: %s\n", shared->data);
    V(empty);

    sleep(1);
    CHECK(shmdt(shared) == 0);
    CHECK(shmctl(shmid, IPC_RMID, 0) == 0);
    CHECK(sem_close(full) == 0);
    CHECK(sem_close(empty) == 0);
    CHECK(sem_close(rcv_to_snd) == 0);
    CHECK(sem_unlink(EMPTY_PATH) == 0);
    CHECK(sem_unlink(FULL_PATH) == 0);
    CHECK(sem_unlink(RTS_PATH) == 0);
    return 0;
}