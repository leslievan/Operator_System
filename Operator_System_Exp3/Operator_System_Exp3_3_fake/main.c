/**
 * @file        main.c
 * @date        2018-12-18
 * @author      Leslie Van
 * @brief       Communicate by message queue between a receiver and two senders(fake).
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
        P(snd_mutex);
        printf("sender1> ");
        fflush(stdout);
        fgets(buf.buffer, BUFSIZ, stdin);
        buf.message_type = snd_to_rcv1;
        /* send the message */
        CHECK(0 <= msgsnd(mq, (void *) &buf, MAX_SIZE, 0));
        V(rcv_mutex);
    } while (strncmp(buf.buffer, MSG_STOP, strlen(MSG_STOP)));

    /* wait for response */
    bytes_read = msgrcv(mq, (void *) &buf, MAX_SIZE, rcv_to_snd1, 0);
    CHECK(bytes_read >= 0);
    printf("%s", buf.buffer);
    printf("--------------------------------------------\n");
    V(snd_mutex);
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
        P(snd_mutex);
        printf("sender2> ");
        fflush(stdout);
        fgets(buf.buffer, BUFSIZ, stdin);
        buf.message_type = snd_to_rcv2;
        /* send the message */
        CHECK(0 <= msgsnd(mq, (void *) &buf, MAX_SIZE, 0));
        V(rcv_mutex);
    } while (strncmp(buf.buffer, MSG_STOP, strlen(MSG_STOP)));

    /* wait for response */
    bytes_read = msgrcv(mq, (void *) &buf, MAX_SIZE, rcv_to_snd2, 0);
    CHECK(bytes_read >= 0);
    printf("%s", buf.buffer);
    printf("--------------------------------------------\n");
    V(snd_mutex);
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
        P(rcv_mutex);

        bytes_read = msgrcv(mq, (void *) &buf, MAX_SIZE, 0, 0);
        CHECK(bytes_read >= 0);
        if (!strncmp(buf.buffer, MSG_STOP, strlen(MSG_STOP))) {
            if (buf.message_type == 1) {
                bytes_write = msgsnd(mq, (void *) &over1, MAX_SIZE, 0);
                CHECK(bytes_write >= 0);
                must_stop--;
            } else if (buf.message_type == 2) {
                bytes_write = msgsnd(mq, (void *) &over2, MAX_SIZE, 0);
                CHECK(bytes_write >= 0);
                must_stop--;
            }
        } else {
            printf("Received%d: %s", buf.message_type, buf.buffer);
            printf("--------------------------------------------\n");
            V(snd_mutex);
        }
    } while (must_stop);


    /* cleanup */
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
    pthread_t rcv_pid, snd2_pid, snd1_pid;
    int state;

    sem_init(&snd_mutex, 0, 1);
    sem_init(&rcv_mutex, 0, 0);

    state = pthread_create(&rcv_pid, NULL, receiver, NULL);
    CHECK(state == 0);
    state = pthread_create(&snd1_pid, NULL, sender1, NULL);
    CHECK(state == 0);
    state = pthread_create(&snd2_pid, NULL, sender2, NULL);
    CHECK(state == 0);

    pthread_join(snd1_pid, NULL);
    pthread_join(snd2_pid, NULL);
    pthread_join(rcv_pid, NULL);
    return 0;
}
