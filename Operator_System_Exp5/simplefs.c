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
        printf("Initialed success!\n");
        do_format();
    }

    /** Init the first openfile entry. */
    fcb_cpy(&openfile_list[0].open_fcb, ((fcb *) (fs_head + 5 * BLOCK_SIZE)));
    strcpy(openfile_list[0].dir, ROOT);
    openfile_list[0].count = 0;
    openfile_list[0].fcb_state = 0;
    openfile_list[0].free = 1;
    curdir = 0;

    /** Init the other openfile entry. */
    fcb *empty = (fcb *) malloc(sizeof(fcb));
    set_fcb(empty, "\0", "\0", 0, 0, 0, 0);
    for (i = 1; i < MAX_OPENFILE; i++) {
        fcb_cpy(&openfile_list[i].open_fcb, empty);
        strcpy(openfile_list[i].dir, "\0");
        openfile_list[i].free = 0;
        openfile_list[i].count = 0;
        openfile_list[i].fcb_state = 0;
    }

    /** Init global variables. */
    strcpy(current_dir, openfile_list[curdir].dir);
    start = ((block0 *) fs_head)->start_block;
    free(empty);
    return 0;
}

/**
 * Entry for command "format".
 * @author Leslie Van
 */
int my_format(char **args) {
    unsigned char *ptr;
    FILE *fp;
    int i;

    /**< Check argument count. */
    for (i = 0; args[i] != NULL; i++);
    if (i > 2) {
        fprintf(stderr, "csh: expected argument to \"format\"\n");
        return 1;
    }

    /**< Check argument value. */
    if (args[1] != NULL) {
        /**< Fill with 0. */
        if (!strcmp(args[1], "-x")) {
            ptr = (unsigned char *) malloc(DISK_SIZE);
            memset(ptr, 0, DISK_SIZE);
            fp = fopen(SYS_PATH, "w");
            fwrite(ptr, DISK_SIZE, 1, fp);
            free(ptr);
            fclose(fp);
        } else {
            fprintf(stderr, "csh: expected argument to \"format\"\n");
            return 1;
        }
    }
    do_format();

    return 1;
}

/**
 * Fast format file system.
 * Create boot block, file allocation tables and root directory.
 * @author Leslie Van
 */
int do_format(void) {
    unsigned char *ptr = fs_head;
    int i;
    int first;
    FILE *fp;

    /** Init the boot block(block0). */
    block0 *init_block = (block0 *) ptr;
    strcpy(init_block->information,
           "Disk Size = 1MB, Block Size = 1KB, Block0 in 0, FAT0/1 in 1/3, Root Directory in 5");
    init_block->root = 5;
    init_block->start_block = init_block + BLOCK_SIZE * 7;
    ptr += BLOCK_SIZE;

    /** Init FAT0/1. */
    set_free(0, BLOCK_NUM, 1);

    /** Allocate 5 blocks to one block0(1) and two fat(2). */
    set_free(get_free(1), 1, 0);
    set_free(get_free(2), 2, 0);
    set_free(get_free(2), 2, 0);

    ptr += BLOCK_SIZE * 4;

    /** 2 blocks to root directory. */
    first = get_free(ROOT_BLOCK_NUM);
    set_free(first, ROOT_BLOCK_NUM, 0);

    fcb *root = (fcb *) ptr;
    set_fcb(root, ".", "di", 0, first, sizeof(fcb) * 4, 1);
    root++;
    set_fcb(root, "..", "di", 0, first, sizeof(fcb) * 4, 1);
    root++;

    /** Create test file and test folder.(delete after test) */
    first = get_free(1);
    set_free(first, 1, 0);
    set_fcb(root, "file", "txt", 1, first, sizeof(fcb) * 4, 1);
    root++;
    first = get_free(1);
    set_free(first, 1, 0);
    set_fcb(root, "folder", "di", 0, first, sizeof(fcb) * 4, 1);
    root++;
    fcb *folder = fs_head + BLOCK_SIZE * first;
    set_fcb(folder, ".", "di", 0, first, sizeof(fcb) * 3, 1);
    folder++;
    set_fcb(folder, "..", "di", 0, first, sizeof(fcb) * 3, 1);
    folder++;
    set_fcb(folder, "test", "di", 0, first, sizeof(fcb) * 3, 1);

    // for (i = 2; i < BLOCK_SIZE * 2 / sizeof(fcb); i++, root++) {
    for (i = 4; i < BLOCK_SIZE * 2 / sizeof(fcb); i++, root++) {
        root->free = 0;
    }

    set_free(0, BLOCK_NUM, 1);

    /** Write back. */
    fp = fopen(SYS_PATH, "w");
    fwrite(fs_head, DISK_SIZE, 1, fp);
    fclose(fp);
}

/**
 *
 * @author
 * @param args
 */
int my_cd(char **args) {
    return 1;
}


int do_chdir(int fd) {
    curdir = fd;
    return 0;
}

int my_pwd(char **args) {
    printf("%s\n", current_dir);
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
 * @return
 */
int do_mkdir() {

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
 * @return
 */
int do_rmdir() {

}

/**
 *
 * @author
 */
int my_ls(char **args) {
    int first = openfile_list[curdir].open_fcb.first;
    int i;
    fcb *dir;

    /**< Check argument count. */
    for (i = 0; args[i] != NULL; i++);
    if (i > 2) {
        fprintf(stderr, "csh: expected argument to \"ls\"\n");
        return 1;
    }

    if (args[1] != NULL) {
        dir = find_fcb(args[1]);
        if (dir != NULL && dir->attribute == 0) {
            first = dir->first;
        } else {
            fprintf(stderr, "csh: No such folder\n");
            return 1;
        }
    }
    do_ls(first, 'n');

    return 1;
}

void do_ls(int first, char mode) {
    char fullname[NAMELENGTH];
    int count;
    fcb *root = (fcb *) (fs_head + BLOCK_SIZE * first);

    for (count = 1; root->free != 0; root++, count++) {
        if (root->attribute == 0) {
            printf("%s", FOLDER_COLOR);
            printf("%s\t", root->filename);
            printf("%s", DEFAULT_COLOR);
        } else {
            get_fullname(fullname, root);
            printf("%s\t", fullname);
        }
        if (count % 5 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

/**
 *
 * @author
 * @param args
 * @return
 * @author
 */
int my_create(char **args) {
    char filename[NAMELENGTH];

    if (args[1] != NULL) {
        strcpy(filename, args[1]);
    } else {
        fprintf(stderr, "csh: expected argument to \"create\"\n");
        return 1;
    }
    do_create(filename);

    return 1;
}

int do_create(const char *filename) {
    /** Allocate space. */
    int first = get_free(1);
    if (first == -1) {
        /**< No enough space. */
        fprintf(stderr, "csh: no enough space to create file\n");
        return -1;
    }

    set_free(first, 1, 0);
    char str[NAMELENGTH];
    char fname[8], exname[3];
    char *token;

    /** Find an empty fcb. */
    fcb *cur = find_fcb("./");
    for (; cur->free == 1; cur++);

    /** Split name and initial variables. */
    strcpy(str, filename);
    token = strtok(str, ".");
    strcpy(fname, token);
    token = strtok(NULL, ".");
    strcpy(exname, token);
    set_fcb(cur, fname, exname, 1, first, 0, 1);

    return 0;
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


int do_rm() {

}

int my_open(char **args) {
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


int my_close(char **args) {
    return 1;
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

    return -1;
}

/**
 * Change value of FAT.
 * @param first The starting block number.
 * @param lengt The blocks count.
 * @param mode 0 to allocate and 1 to set free.
 * @author Leslie Van
 */
int set_free(unsigned short first, unsigned short lengt, int mode) {
    unsigned char *ptr = fs_head;
    fat *fat0 = (fat *) (ptr + BLOCK_SIZE);
    fat *fat1 = (fat *) (ptr + BLOCK_SIZE * 3);
    int i;

    for (i = 0; i < first; i++, fat0++, fat1++);

    if (mode == 1) {
        if (lengt == 0) {
            /**< Reclaim space. */
            while (fat0->id != END) {
                fat0->id = FREE;
                fat1->id = FREE;
            }
            fat0->id = FREE;
            fat1->id = FREE;
        } else {
            for (i = 0; i < lengt; i++, fat0++, fat1++) {
                fat0->id = FREE;
                fat1->id = FREE;
            }
        }
    } else {
        /**< Allocate consecutive space. */
        for (; i < first + lengt - 1; i++, fat0++, fat1++) {
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
    int i = 0;
    char empty[16] = "\0";
    time_t *now = (time_t *) malloc(sizeof(time_t));
    struct tm *timeinfo;
    time(now);
    timeinfo = localtime(now);

    strncpy(f->filename, empty, 8);
    strncpy(f->exname, empty, 3);
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

/**
 * Copy a fcb.
 * @param dest Destination fcb.
 * @param src Source fcb.
 */
int fcb_cpy(fcb *dest, fcb *src) {
    strcpy(dest->filename, src->filename);
    strcpy(dest->exname, src->exname);
    dest->attribute = src->attribute;
    dest->time = src->time;
    dest->date = src->date;
    dest->first = src->first;
    dest->length = src->length;
    dest->free = src->free;

    return 0;
}

/**
 * Translate relative path to absolute path
 * @param abspath Absolute path.
 * @param relpath Relative path.
 * @return 0.
 */
int get_abspath(char *abspath, const char *relpath) {
    char str[PATHLENGTH];
    char *token;
    int i;
    ssize_t n;

    for (i = 0; i < PATHLENGTH; i++) {
        abspath[i] = '\0';
    }

    strcpy(str, relpath);
    token = strtok(str, DELIM);

    /** Root directory. */
    if (token == NULL) {
        return 0;
    }

    if (!strcmp(token, "..")) {
        /** par folder */
        n = strlen(openfile_list[curdir].open_fcb.filename);
        for (i = 0; i < strlen(current_dir) - n + 1; i++) {
            abspath[i] = current_dir[i];
        }
    } else {
        /** cur folder */
        if (!strcmp(token, ".")) {
            token = strtok(NULL, DELIM);
        }

        /** avoid to double "/" when par folder is root */
        if (strcmp(ROOT, current_dir)) {
            strcpy(abspath, current_dir);
        } else {
            strcpy(abspath, "");
        }

        while (token != NULL) {
            strcat(abspath, DELIM);
            strcat(abspath, token);
            if (strrchr(token, '.') == NULL) {
                strcat(abspath, ".di");
            }
            token = strtok(NULL, DELIM);
        }
    }

    return 0;
}

/**
 * Find fcb by abspath
 * @param path
 * @return
 */
fcb *find_fcb(const char *path) {
    char abspath[PATHLENGTH];
    get_abspath(abspath, path);
    char *token = strtok(abspath, DELIM);
    if (token == NULL) {
        return (fcb *) (fs_head + BLOCK_SIZE * 5);
    }
    return find_fcb_r(token, 5);
}

/**
 * A procedure to find fcb recursively.
 * @param token File name in (ptr).
 * @param first Par fcb pointer.
 * @return FCB pointer of token.
 */
fcb *find_fcb_r(char *token, int first) {
    int i;
    char fullname[NAMELENGTH] = "\0";
    fcb *root;
    root = (fcb *) (BLOCK_SIZE * first + fs_head);

    for (i = 0; i < root->length / sizeof(fcb); i++, root++) {
        get_fullname(fullname, root);
        if (!strcmp(token, fullname)) {
            token = strtok(NULL, DELIM);
            if (token == NULL) {
                return root;
            }
            return find_fcb_r(token, root->first);
        }
    }
    return NULL;
}

/**
 * Get a empty useropen entry.
 * @return If empty useropen exist return entry index, else return -1;
 */
int get_useropen() {
    int i;

    for (i = 0; i < MAX_OPENFILE; i++) {
        if (openfile_list[i].free == 0) {
            return i;
        }
    }

    return -1;
}

/**  */
void init_block(fcb *fcb1) {
    int i;

    fcb *root = (fcb *) (fs_head + BLOCK_NUM * fcb1->first);
    set_fcb(root, ".", "di", 0, fcb1->first, sizeof(fcb) * 2, 1);
    root++;
    set_fcb(root, "..", "di", 0, fcb1->first, sizeof(fcb) * 2, 1);
    root++;
    for (i = 2; i < BLOCK_SIZE / sizeof(fcb); i++, root++) {
        root->free = 0;
    }
}

/**
 * Get file full name.
 * @param fullname A char array[NAMELENGTH].
 * @param fcb1 Source fcb pointer.
 */
void get_fullname(char *fullname, fcb *fcb1) {
    if (fcb1 == NULL) {
        fullname = NULL;
        return;
    }

    int i;

    for (i = 0; i < NAMELENGTH; i++) {
        fullname[i] = '\0';
    }
    strncat(fullname, fcb1->filename, 8);
    strncat(fullname, ".", 2);
    strncat(fullname, fcb1->exname, 3);
}