/**
 * @file        main.c
 * @author      Leslie Van
 * @date        2018.11.12
 * @brief       Children process communicate with parent by pipe.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
#include <semaphore.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define BUF_MAX_SIZE 8192
#define CHECK(x) \
    do { \
        if (!(x)) { \
            fprintf(stderr, "%s:%d: ", __func__, __LINE__); \
            perror(#x); \
            exit(-1); \
        } \
    } while (0) \

/**
 * Create three children processes to test pipe communication.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return status code.
 */
int main(int argc, char **argv) {
    int pipefd[2], pid, i = 0;
    int flag = 0;
    ssize_t n;
    char buf[BUF_MAX_SIZE];
    char str[BUF_MAX_SIZE];
    sem_t *write_mutex;
    write_mutex = sem_open("pipe_test_wm", O_CREAT | O_RDWR, 0666, 1);

    for (i = 0; i < BUF_MAX_SIZE; i++) {
        buf[i] = 'a';
    }

    CHECK(pipe(pipefd) >= 0);
    CHECK((pid = fork()) >= 0);

    if (pid == 0) {
        int count = 0;
        sem_wait(write_mutex);
        close(pipefd[0]);
        int flags = fcntl(pipefd[1], F_GETFL);
        fcntl(pipefd[1], F_SETFL, flags | O_NONBLOCK);
        while (!flag) {
            n = write(pipefd[1], buf, BUF_MAX_SIZE);
            if (n == -1) {
                flag = 1;
            } else {
                count++;
                printf("children 1 write %dB\n", n);
            }
        }
        printf("space = %dKB\n", (count * BUF_MAX_SIZE) / 1024);
        sem_post(write_mutex);
        exit(0);
    } else {
        CHECK((pid = fork()) >= 0);

        if (pid == 0) {
            sleep(1);
            sem_wait(write_mutex);
            close(pipefd[0]);
            n = write(pipefd[1], "This is the second children.\n", 29);
            printf("children 2 write %dB\n", n);
            sem_post(write_mutex);
            exit(0);
        } else {
            CHECK((pid = fork()) >= 0);

            if (pid == 0) {
                sleep(1);
                sem_wait(write_mutex);
                close(pipefd[0]);
                n = write(pipefd[1], "This is the third children.\n", 28);
                printf("children 3 write %dB\n", n);
                sem_post(write_mutex);
                exit(0);
            } else {
                wait(0);
                close(pipefd[1]);
                int flags = fcntl(pipefd[0], F_GETFL);
                fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);
                while (!flag) {
                    n = read(pipefd[0], str, BUF_MAX_SIZE);
                    if (n == -1) {
                        flag = 1;
                    } else {
                        printf("%dB read\n", n);
                    }
                }
                sleep(2);
                n = read(pipefd[0], str, BUF_MAX_SIZE);
                printf("%dB read\n", n);
                for (i = 0; i < n; i++) {
                    printf("%c", str[i]);
                }
            }
        }
    }

    sem_close(write_mutex);
    sem_unlink("pipe_test_wm");
    return 0;
}