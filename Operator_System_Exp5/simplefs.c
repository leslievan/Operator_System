/**
 * @file    simplefs.c
 * @brief   Definition in FAT16 file system.
 * @details Macro definitions, structs such as FCB and FAT, and some global variable.
 * @author  Leslie Van
 * @date    2018-12-19 to
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "simplefs.h"


/* Definition of functions */
/**
 * Start file system and initial variable.
 * @author Leslie Van
 */
int start_sys(void) {
    fs_head = (unsigned char *) malloc(DISK_SIZE);
    memset(fs_head, 0, DISK_SIZE);
    FILE *fp;
    int i;

    if ((fp = fopen(SYS_PATH, "r")) != NULL) {
        fread(fs_head, DISK_SIZE, 1, fp);
        fclose(fp);
    } else {
        printf("System is not initialized, now install it and create system file.\n");
        printf("Please don't leave program.\n");
        do_format();
    }

    /** Init the first openfile entry. */
    strcpy(openfile_list[0].filename, "root");
    strcpy(openfile_list[0].exname, "di");
    strcpy(openfile_list[0].dir, "/root");
    openfile_list[0].attribute = 0;
    openfile_list[0].time = ((fcb *) (fs_head + 5 * BLOCK_SIZE))->time;
    openfile_list[0].date = ((fcb *) (fs_head + 5 * BLOCK_SIZE))->date;
    openfile_list[0].first = ((fcb *) (fs_head + 5 * BLOCK_SIZE))->first;
    openfile_list[0].length = ((fcb *) (fs_head + 5 * BLOCK_SIZE))->length;
    openfile_list[0].count = 1;
    openfile_list[0].fcb_state = 0;
    openfile_list[0].free = 1;
    curdir = 0;

    /** Init the other openfile entry. */
    for (i = 1; i < MAX_OPENFILE; i++) {
        set_useropen(&openfile_list[i], "\0", "\0", "\0", 0, 0, 0, 1, 0, 0);
    }

    /** Init global variables. */
    strcpy(current_dir, openfile_list[curdir].dir);
    start = ((block0 *) fs_head)->start_block;

    return 0;
}

/**
 * Entry for command "format".
 * @author Leslie Van
 */
int my_format(char **args) {
    do_format();
    return 1;
}

/**
 * Format file system.
 * Create boot block, file allocation tables and root directory.
 * @author Leslie Van
 */
int do_format(void) {
    unsigned char *ptr = fs_head;
    int i;
    int first;

    /** Init the boot block.(block0) */
    block0 *block01 = (block0 *) ptr;
    strcpy(block01->information, "Disk Size = 1MB, Block Size = 1KB, Block0 in 0, FAT0/1 in 1/3, Root Directory in 5");
    block01->root = 5;
    block01->start_block = block01 + BLOCK_SIZE * 7;
    ptr += BLOCK_SIZE;

    /** Init fat0 and fat1. */
    fat *fat0 = (fat *) ptr;
    fat *fat1 = (fat *) (ptr + BLOCK_SIZE * 2);
    fat *t0 = fat0;
    fat *t1 = fat1;
    for (i = 0; i < 1024; i++, t0++, t1++) {
        t0->id = FREE;
        t1->id = FREE;  /**< Set free flag to all block. */
    }
    /** 5 blocks to one block0(1) and two fat(2). */
    for (i = 0; i < 5; i++) {
        set_free(i, 1, 0);
    }
    ptr += BLOCK_SIZE * 4;

    /** 2 blocks to root directory. */
    first = get_free(ROOT_BLOCK_NUM);
    set_free(first, ROOT_BLOCK_NUM, 0);

    fcb *root = (fcb *) ptr;
    set_fcb(root, ".", "di", 1, first, sizeof(fcb), 1);
    root++;
    set_fcb(root, "..", "di", 1, first, sizeof(fcb), 1);
    root++;
    for (i = 2; i < BLOCK_SIZE * 2 / sizeof(fcb); i++, root++) {
        root->free = 0;
    }

    return 0;
}

/**
 *
 * @author
 * @param args
 */
int my_cd(char **args) {
    return 1;
}

/**
 *
 * @author
 * @param args
 */
int my_mkdir(char **args) {
    return 1;
}

/**
 *
 * @author
 * @param args
 */
int my_rmdir(char **args) {
    return 1;
}

/**
 *
 * @author
 */
int my_ls(char **args) {
    return 1;
}

/**
 *
 * @author
 * @param args
 * @return
 * @author
 */
int my_create(char **args) {
    return 1;
}

/**
 *
 * @author
 * @param args
 * @author
 */
int my_rm(char **args) {
    return 1;
}

/**
 *
 * @param filename
 * @param mode
 * @return
 * @author
 */
int do_open(char *filename, char mode) {

}

/**
 *
 * @brief
 * @param fd
 * @author
 */
int do_close(int fd) {

}

/**
 *
 * @brief
 * @param args
 * @return
 * @author
 */
int my_write(char **args) {
    return 1;
}

/**
 *
 * @param fd
 * @param text
 * @param len
 * @param mode
 * @return
 * @author
 */
int do_write(int fd, char *text, int len, char mode) {

}

/**
 *
 * @param args
 * @return
 * @author
 */
int my_read(char **args) {
    return 1;
}

/**
 *
 * @param fd
 * @param len
 * @param text
 * @return
 * @author
 */
int do_read(int fd, int len, char *text) {

}

/**
 * Exit system, save changes.
 * @author
 */
int my_exit_sys(void) {
    FILE *fp;
    fp = fopen(SYS_PATH, "w");
    fwrite(fs_head, DISK_SIZE, 1, fp);
    free(fs_head);
    fclose(fp);
    return 0;
}

/**
 * Detect free blocks in FAT.
 * @param count Count of needed blocks.
 * @return 0 without enough space, else return the first block number.
 * @author Leslie Van
 */
int get_free(int count) {
    unsigned char *ptr = fs_head;
    fat *fat0 = (fat *) (ptr + BLOCK_SIZE);
    int i, j, flag = 0;
    int fat[1024];

    /** Copy FAT. */
    for (i = 0; i < 1024; i++, fat0++) {
        fat[i] = fat0->id;
    }

    /** Find a continuous space. */
    for (i = 0; i < 1024 - count; i++) {
        for (j = i; j < i + count; j++) {
            if (fat[j] > 0) {
                flag = 1;
                break;
            }
        }
        if (flag) {
            flag = 0;
            i = j;
        } else {
            return i;
        }
    }

    return 0;
}

/**
 * Change value of FAT.
 * @param first The starting block number.
 * @param length The blocks count.
 * @param mode 0 to allocate and 1 to set free.
 * @author Leslie Van
 */
int set_free(int first, int length, int mode) {
    unsigned char *ptr = fs_head;
    fat *fat0 = (fat *) (ptr + BLOCK_SIZE);
    fat *fat1 = (fat *) (ptr + BLOCK_SIZE * 3);
    int i;
    for (i = 0; i < first; i++, fat0++, fat1++);
    if (mode == 1) {
        for (; i < first + length; i++, fat0++, fat1++) {
            fat0->id = FREE;
            fat1->id = FREE;
        }
    } else {
        for (; i < first + length - 1; i++, fat0++, fat1++) {
            fat0->id = first + 1;
            fat1->id = first + 1;
        }
        fat0->id = END;
        fat1->id = END;
    }
    return 0;
}

/**
 * Set fcb attribute.
 * @param f The pointer of fcb.
 * @param filename FCB filename.
 * @param exname FCB file extensions name.
 * @param attr FCB file attribute.
 * @param first FCB starting block number.
 * @param length FCB file length.
 * @param ffree 1 when file occupied, else 0.
 * @author Leslie Van
 */
int set_fcb(fcb *f, char *filename, char *exname, unsigned char attr, unsigned short first, unsigned long length,
            char ffree) {
    time_t *now = (time_t *) malloc(sizeof(time_t));
    struct tm *timeinfo;
    time(now);
    timeinfo = localtime(now);

    strcpy(f->filename, filename);
    strcpy(f->exname, exname);
    f->attribute = attr;
    f->time = get_time(timeinfo);
    f->date = get_date(timeinfo);
    f->first = first;
    f->length = length;
    f->free = ffree;

    free(now);
    return 0;
}

/**
 *
 * @param u The pointer of useropen.
 * @param filename Useropen filename.
 * @param exname Useropen file extensions name.
 * @param attr Useropen file attribute.
 * @param first Useropen starting block number.
 * @param length Useropen file length.
 * @param count Useropen file R/W cursor location.
 * @param fstate FCB state.
 * @param ffree 1 when file occupied, else 0.
 * @author Leslie Van
 */
int set_useropen(useropen *u, char *filename, char *exname, char *dir, unsigned char attr, unsigned short first,
                 unsigned long length, int count, char fstate, char ffree) {
    time_t *now = (time_t) malloc(sizeof(time_t));
    struct tm *timeinfo;
    time(now);
    timeinfo = localtime(now);

    strcpy(u->filename, filename);
    strcpy(u->exname, exname);
    strcpy(u->dir, dir);
    u->attribute = attr;
    u->time = get_time(timeinfo);
    u->date = get_date(timeinfo);
    u->first = first;
    u->length = length;
    u->count = count;
    u->fcb_state = fstate;
    u->free = ffree;

    free(now);
    return 0;
}

/**
 * Translate ISO time to short time.
 * @param timeinfo Current time structure.
 * @return Time number after translation.
 * @author Leslie Van
 */
unsigned short get_time(struct tm *timeinfo) {
    int hour, min, sec;
    unsigned short result;

    hour = timeinfo->tm_hour;
    min = timeinfo->tm_min;
    sec = timeinfo->tm_sec;
    result = (hour << 11) + (min << 5) + (sec >> 1);

    return result;
}

/**
 * Translate ISO date to short date.
 * @param timeinfo local
 * @return Date number after translation.
 * @author Leslie Van
 */
unsigned short get_date(struct tm *timeinfo) {
    int year, mon, day;
    unsigned short result;

    year = timeinfo->tm_year;
    mon = timeinfo->tm_mon;
    day = timeinfo->tm_mday;
    result = (year << 9) + (mon << 5) + day;

    return result;
}