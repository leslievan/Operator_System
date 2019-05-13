/**
 * @file        main.c
 * @date        2018-11-26
 * @author      Leslie Van
 * @brief       Communicate by message queue between a receiver and two senders
 */
#include "common.h"

/**
 * @brief Create mq and send message to receiver.
 * @return
 */
void *sender1() {
    int mq;
    struct msg_st buf;
    ssize_t bytes_read;

    /* open the mail queue */
    mq = msgget((key_t) QUEUE_ID, 0666 | IPC_CREAT);
    CHECK((key_t) -1 != mq);

    do {
        P(w_mutex);
        P(snd_dp);
        printf("sender1> ");
        V(rcv_dp);
        fflush(stdout);
        fgets(buf.buffer, BUFSIZ, stdin);
        buf.message_type = snd_to_rcv1;
        /* send the message */
        P(empty);
        CHECK(0 <= msgsnd(mq, (void*)&buf, MAX_SIZE, 0));
        V(full);
        V(w_mutex);
     } while (strncmp(buf.buffer, MSG_STOP, strlen(MSG_STOP)));

    /* wait for response */
    P(over);
    bytes_read = msgrcv(mq, (void *) &buf, MAX_SIZE, rcv_to_snd1, 0);
    CHECK(bytes_read >= 0);
    printf("%s", buf.buffer);
    printf("--------------------------------------------\n");
    V(snd_dp);
    pthread_exit(NULL);
}

/**
 * @brief Send message to receiver.
 * @return
 */
void *sender2() {
    int mq;
    struct msg_st buf;
    ssize_t bytes_read;

    /* open the mail queue */
    mq = msgget((key_t) QUEUE_ID, 0666 | IPC_CREAT);
    CHECK((key_t) -1 != mq);

    do {
        P(w_mutex);
        P(snd_dp);
        printf("sender2> ");
        V(rcv_dp);
        fflush(stdout);
        fgets(buf.buffer, BUFSIZ, stdin);
        buf.message_type = snd_to_rcv2;
        /* send the message */
        P(empty);
        CHECK(0 <= msgsnd(mq, (void *) &buf, MAX_SIZE, 0));
        V(full);
        V(w_mutex);
     } while (strncmp(buf.buffer, MSG_STOP, strlen(MSG_STOP)));

    /* wait for response */
    P(over);
    bytes_read = msgrcv(mq, (void *) &buf, MAX_SIZE, rcv_to_snd2, 0);
    CHECK(bytes_read >= 0);
    printf("%s", buf.buffer);
    printf("--------------------------------------------\n");
    V(snd_dp);
    pthread_exit(NULL);
}

/**
 * @brief Receive message from sender, and response when sender exit.
 * @return
 */
void *receiver() {
    struct msg_st buf, over1, over2;
    int mq, must_stop = 2;
    struct msqid_ds t;
    over1.message_type = 3;
    strcpy(over1.buffer, "over1\n");
    over2.message_type = 4;
    strcpy(over2.buffer, "over2\n");

    /* open the mail queue */
    mq = msgget((key_t) QUEUE_ID, 0666 | IPC_CREAT);
    CHECK((key_t) -1 != mq);

    do {
        ssize_t bytes_read, bytes_write;
        /* receive the message */
        P(full);
        bytes_read = msgrcv(mq, (void *) &buf, MAX_SIZE, 0, 0);
        V(empty);
        CHECK(bytes_read >= 0);
        if (!strncmp(buf.buffer, MSG_STOP, strlen(MSG_STOP))) {
            if (buf.message_type == 1) {
                bytes_write = msgsnd(mq, (void *) &over1, MAX_SIZE, 0);
                CHECK(bytes_write >= 0);
                V(over);
                must_stop--;
            } else if (buf.message_type == 2) {
                bytes_write = msgsnd(mq, (void *) &over2, MAX_SIZE, 0);
                CHECK(bytes_write >= 0);
                V(over);
                must_stop--;
            }
        } else {
            P(rcv_dp);
            printf("Received%d: %s", buf.message_type, buf.buffer);
            printf("--------------------------------------------\n");
            V(snd_dp);
        }
    } while (must_stop);


    /* cleanup */
    P(snd_dp);
    CHECK(!msgctl(mq, IPC_RMID, &t));
    pthread_exit(NULL);
}

/**
 * Create three thread to test functions
 * @param argc Argument count
 * @param argv Argument vector
 * @return Always 0
 */
int main(int argc, char **argv) {
    pthread_t t1, t2, t3;
    int state;

    sem_init(&snd_dp, 1, 1);
    sem_init(&rcv_dp, 1, 0);
    sem_init(&empty, 1, 10);
    sem_init(&full, 1, 0);
    sem_init(&w_mutex, 1, 1);
    sem_init(&over, 1, 0);

    state = pthread_create(&t1, NULL, receiver, NULL);
    CHECK(state == 0);
    state = pthread_create(&t3, NULL, sender1, NULL);
    CHECK(state == 0);
    state = pthread_create(&t2, NULL, sender2, NULL);
    CHECK(state == 0);

    pthread_join(t3, NULL);
    pthread_join(t2, NULL);
    pthread_join(t1, NULL);
    return 0;
}
