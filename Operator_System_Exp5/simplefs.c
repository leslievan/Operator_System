/**
 * @file    simplefs.c
 * @brief   Definition in FAT16 file system.
 * @details Macro definitions, structs such as FCB and FAT, and some global variable.
 * @author  Leslie Van
 * @date    2018-12-19 to 2019-1-3
 */

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

    /**< Init the first openfile entry. */
    fcb_cpy(&openfile_list[0].open_fcb, ((fcb *) (fs_head + 5 * BLOCK_SIZE)));
    strcpy(openfile_list[0].dir, ROOT);
    openfile_list[0].count = 0;
    openfile_list[0].fcb_state = 0;
    openfile_list[0].free = 1;
    curdir = 0;

    /**< Init the other openfile entry. */
    fcb *empty = (fcb *) malloc(sizeof(fcb));
    set_fcb(empty, "\0", "\0", 0, 0, 0, 0);
    for (i = 1; i < MAX_OPENFILE; i++) {
        fcb_cpy(&openfile_list[i].open_fcb, empty);
        strcpy(openfile_list[i].dir, "\0");
        openfile_list[i].free = 0;
        openfile_list[i].count = 0;
        openfile_list[i].fcb_state = 0;
    }

    /**< Init global variables. */
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
        fprintf(stderr, "format: expected argument to \"format\"\n");
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
            fprintf(stderr, "format: expected argument to \"format\"\n");
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
    int first, second;
    FILE *fp;

    /**< Init the boot block(block0). */
    block0 *init_block = (block0 *) ptr;
    strcpy(init_block->information,
           "Disk Size = 1MB, Block Size = 1KB, Block0 in 0, FAT0/1 in 1/3, Root Directory in 5");
    init_block->root = 5;
    init_block->start_block = (unsigned char *) (init_block + BLOCK_SIZE * 7);
    ptr += BLOCK_SIZE;

    /**< Init FAT0/1. */
    set_free(0, 0, 2);

    /**< Allocate 5 blocks to one block0(1) and two fat(2). */
    set_free(get_free(1), 1, 0);
    set_free(get_free(2), 2, 0);
    set_free(get_free(2), 2, 0);

    ptr += BLOCK_SIZE * 4;

    /**< 2 blocks to root directory. */
    fcb *root = (fcb *) ptr;
    first = get_free(ROOT_BLOCK_NUM);
    set_free(first, ROOT_BLOCK_NUM, 0);
    set_fcb(root, ".", "di", 0, first, BLOCK_SIZE * 2, 1);
    root++;
    set_fcb(root, "..", "di", 0, first, BLOCK_SIZE * 2, 1);
    root++;

    for (i = 2; i < BLOCK_SIZE * 2 / sizeof(fcb); i++, root++) {
        root->free = 0;
    }

    memset(fs_head + BLOCK_SIZE * 7, 'a', 15);
    /**< Write back. */
    fp = fopen(SYS_PATH, "w");
    fwrite(fs_head, DISK_SIZE, 1, fp);
    fclose(fp);
}

/**
 * Change current directory.
 * @param args
 * @return Always 1.
 */
int my_cd(char **args) {
    int i;
    int fd;
    char abspath[PATHLENGTH];
    fcb *dir;

    /**< Check argument count. */
    for (i = 0; args[i] != NULL; i++);
    if (i != 2) {
        fprintf(stderr, "cd: expected argument to \"format\"\n");
        return 1;
    }

    /**< Check argument value. */
    memset(abspath, '\0', PATHLENGTH);
    get_abspath(abspath, args[1]);
    dir = find_fcb(abspath);
    if (dir == NULL || dir->attribute == 1) {
        fprintf(stderr, "cd: No such folder\n");
        return 1;
    }

    /**< Check if the folder fcb exist in openfile_list. */
    for (i = 0; i < MAX_OPENFILE; i++) {
        if (openfile_list[i].free == 0) {
            continue;
        }

        if (!strcmp(dir->filename, openfile_list[i].open_fcb.filename) &&
            dir->first == openfile_list[i].open_fcb.first) {
            /**< Folder is open. */
            do_chdir(i);
            return 1;
        }
    }

    /**< Folder is close, open it and change current directory. */
    if ((fd = do_open(abspath)) > 0) {
        do_chdir(fd);
    }

    return 1;
}

/**
 * Just do change directory action.
 * @param fd File descriptor of directory.
 */
void do_chdir(int fd) {
    curdir = fd;
    memset(current_dir, '\0', sizeof(current_dir));
    strcpy(current_dir, openfile_list[curdir].dir);
}

/**
 * Display current directory.
 * @param args No argument.
 * @return Always 1.
 */
int my_pwd(char **args) {
    /**< Check argument count. */
    if (args[1] != NULL) {
        fprintf(stderr, "pwd: too many arguments\n");
        return 1;
    }

    printf("%s\n", current_dir);
    return 1;
}

/**
 * Create one or many directory once.
 * Provide function to create two or more directory once.
 * If par folder not exists, print error, others will continue.
 * @param args Folder names.
 * @return Always 1.
 */
int my_mkdir(char **args) {
    int i;
    char path[PATHLENGTH];
    char parpath[PATHLENGTH], dirname[NAMELENGTH];
    char *end;

    /**< Check argument count. */
    if (args[1] == NULL) {
        fprintf(stderr, "mkdir: missing operand\n");
        return 1;
    }

    /**< Do mkdir. */
    for (i = 1; args[i] != NULL; i++) {
        /**< Split path into parent folder and current child folder. */
        get_abspath(path, args[i]);
        end = strrchr(path, '/');
        if (end == path) {
            strcpy(parpath, "/");
            strcpy(dirname, path + 1);
        } else {
            strncpy(parpath, path, end - path);
            strcpy(dirname, end + 1);
        }

        if (find_fcb(parpath) == NULL) {
            fprintf(stderr, "create: cannot create \'%s\': Parent folder not exists\n", parpath);
            continue;
        }
        if (find_fcb(path) != NULL) {
            fprintf(stderr, "create: cannot create \'%s\': Folder or file exists\n", args[i]);
            continue;
        }

        do_mkdir(parpath, dirname);
    }

    return 1;
}

/**
 * Just do create directory.
 * @param parpath Par folder of the folder you want to create.
 * @param dirname Folder name you want to create.
 * @return Error with -1, else return 0.
 */
int do_mkdir(const char *parpath, const char *dirname) {
    int second = get_free(1);
    int i, flag = 0, first = find_fcb(parpath)->first;
    fcb *dir = (fcb *) (fs_head + BLOCK_SIZE * first);

    /**< Check for free fcb. */
    for (i = 0; i < BLOCK_SIZE / sizeof(fcb); i++, dir++) {
        if (dir->free == 0) {
            flag = 1;
            break;
        }
    }
    if (!flag) {
        fprintf(stderr, "mkdir: Cannot create more file in %s\n", parpath);
        return -1;
    }

    /**< Check for free space. */
    if (second == -1) {
        fprintf(stderr, "mkdir: No more space\n");
        return -1;
    }
    set_free(second, 1, 0);

    /**< Set fcb and init folder. */
    set_fcb(dir, dirname, "di", 0, second, BLOCK_SIZE, 1);
    init_folder(first, second);
    return 0;
}

/**
 * Remove folder one or more once.
 * @param args Folders name you want remove.
 */
int my_rmdir(char **args) {
    int i, j;
    fcb *dir;

    /**< Check argument count. */
    if (args[1] == NULL) {
        fprintf(stderr, "rmdir: missing operand\n");
        return 1;
    }

    /**< Do remove. */
    for (i = 1; args[i] != NULL; i++) {
        if (!strcmp(args[i], ".") || !strcmp(args[i], "..")) {
            fprintf(stderr, "rmdir: cannot remove %s: '.' or '..' is read only \n", args[i]);
            return 1;
        }

        if (!strcmp(args[i], "/")) {
            fprintf(stderr, "rmdir:  Permission denied\n");
            return 1;
        }

        dir = find_fcb(args[i]);
        if (dir == NULL) {
            fprintf(stderr, "rmdir: cannot remove %s: No such folder\n", args[i]);
            return 1;
        }

        if (dir->attribute == 1) {
            fprintf(stderr, "rmdir: cannot remove %s: Is a directory\n", args[i]);
            return 1;
        }

        /**< Check if the folder fcb exist in openfile_list. */
        for (j = 0; j < MAX_OPENFILE; j++) {
            if (openfile_list[j].free == 0) {
                continue;
            }

            if (!strcmp(dir->filename, openfile_list[j].open_fcb.filename) &&
                dir->first == openfile_list[j].open_fcb.first) {
                /**< Folder is open. */
                fprintf(stderr, "rmdir: cannot remove %s: File is open\n", args[i]);
                return 1;
            }
        }

        do_rmdir(dir);
    }
    return 1;
}

/**
 * Just do remove directory.
 */
void do_rmdir(fcb *dir) {
    int first = dir->first;

    dir->free = 0;
    dir = (fcb *) (fs_head + BLOCK_SIZE * first);
    dir->free = 0;
    dir++;
    dir->free = 0;

    set_free(first, 1, 1);
}

/**
 * Show all thing in folder.
 * @param args Empty to show current folder. '-l' to show by a long format. 'path' to show a specific folder.
 * @return Always 1.
 */
int my_ls(char **args) {
    int first = openfile_list[curdir].open_fcb.first;
    int i, mode = 'n';
    int flag[3];
    fcb *dir;

    /**< Check argument count. */
    for (i = 0; args[i] != NULL; i++) {
        flag[i] = 0;
    }
    if (i > 3) {
        fprintf(stderr, "ls: expected argument\n");
        return 1;
    }

    flag[0] = 1;
    for (i = 1; args[i] != NULL; i++) {
        if (args[i][0] == '-') {
            flag[i] = 1;
            if (!strcmp(args[i], "-l")) {
                mode = 'l';
                break;
            } else {
                fprintf(stderr, "ls: wrong operand\n");
                return 1;
            }
        }
    }

    for (i = 1; args[i] != NULL; i++) {
        if (flag[i] == 0) {
            dir = find_fcb(args[i]);
            if (dir != NULL && dir->attribute == 0) {
                first = dir->first;
            } else {
                fprintf(stderr, "ls: cannot access '%s': No such file or directory\n", args[i]);
                return 1;
            }
            break;
        }
    }

    do_ls(first, mode);

    return 1;
}

/**
 * Just do ls.
 * @param first First block of folder you want to show.
 * @param mode 'n' to normal format, and 'l' to long format.
 */
void do_ls(int first, char mode) {
    int i, count, length = BLOCK_SIZE;
    char fullname[NAMELENGTH], date[16], time[16];
    fcb *root = (fcb *) (fs_head + BLOCK_SIZE * first);
    block0 *init_block = (block0 *) fs_head;
    if (first == init_block->root) {
        length = ROOT_BLOCK_NUM * BLOCK_SIZE;
    }

    if (mode == 'n') {
        for (i = 0, count = 1; i < length / sizeof(fcb); i++, root++) {
            /**< Check if the fcb is used. */
            if (root->free == 0) {
                continue;
            }

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
            count++;
        }
    } else if (mode == 'l') {
        for (i = 0, count = 1; i < length / sizeof(fcb); i++, root++) {
            /**< Check if the fcb is used. */
            if (root->free == 0) {
                continue;
            }

            trans_date(date, root->date);
            trans_time(time, root->time);
            get_fullname(fullname, root);
            printf("%d\t%6d\t%6ld\t%s\t%s\t", root->attribute, root->first, root->length, date, time);
            if (root->attribute == 0) {
                printf("%s", FOLDER_COLOR);
                printf("%s\n", fullname);
                printf("%s", DEFAULT_COLOR);
            } else {
                printf("%s\n", fullname);
            }
            count++;
        }
    }
    printf("\n");
}

/**
 * Create one or more files once.
 * @param args Filename you want to create.
 * @return Always 1.
 */
int my_create(char **args) {
    int i;
    char path[PATHLENGTH];
    char parpath[PATHLENGTH], filename[NAMELENGTH];
    char *end;

    /**< Check argument count. */
    if (args[1] == NULL) {
        fprintf(stderr, "create: missing operand\n");
        return 1;
    }

    memset(parpath, '\0', PATHLENGTH);
    memset(filename, '\0', NAMELENGTH);

    /**< Do create */
    for (i = 1; args[i] != NULL; i++) {
        /**< Split parent folder and filename. */
        get_abspath(path, args[i]);
        end = strrchr(path, '/');
        if (end == path) {
            strcpy(parpath, "/");
            strcpy(filename, path + 1);
        } else {
            strncpy(parpath, path, end - path);
            strcpy(filename, end + 1);
        }

        if (find_fcb(parpath) == NULL) {
            fprintf(stderr, "create: cannot create \'%s\': Parent folder not exists\n", parpath);
            continue;
        }
        if (find_fcb(path) != NULL) {
            fprintf(stderr, "create: cannot create \'%s\': Folder or file exists\n", args[i]);
            continue;
        }

        do_create(parpath, filename);
    }

    return 1;
}

/**
 * Just do create file.
 * @param parpath File par folder.
 * @param filename File name.
 * @return Error with -1, else return 0.
 */
int do_create(const char *parpath, const char *filename) {
    char fullname[NAMELENGTH], fname[16], exname[8];
    char *token;
    int first = get_free(1);
    int i, flag = 0;
    fcb *dir = (fcb *) (fs_head + BLOCK_SIZE * find_fcb(parpath)->first);

    /**< Check for free fcb. */
    for (i = 0; i < BLOCK_SIZE / sizeof(fcb); i++, dir++) {
        if (dir->free == 0) {
            flag = 1;
            break;
        }
    }
    if (!flag) {
        fprintf(stderr, "create: Cannot create more file in %s\n", parpath);
        return -1;
    }

    /**< Check for free space. */
    if (first == -1) {
        fprintf(stderr, "create: No more space\n");
        return -1;
    }
    set_free(first, 1, 0);

    /**< Split name and initial variables. */
    memset(fullname, '\0', NAMELENGTH);
    memset(fname, '\0', 8);
    memset(exname, '\0', 3);
    strcpy(fullname, filename);
    token = strtok(fullname, ".");
    strncpy(fname, token, 8);
    token = strtok(NULL, ".");
    if (token != NULL) {
        strncpy(exname, token, 3);
    } else {
        strncpy(exname, "d", 2);
    }

    /**< Set fcb. */
    set_fcb(dir, fname, exname, 1, first, 0, 1);

    return 0;
}

/**
 * Remove files.
 * @param args Filename you want to remove.
 * @return Always return 1.
 */
int my_rm(char **args) {
    int i, j;
    fcb *file;

    /**< Check argument count. */
    if (args[1] == NULL) {
        fprintf(stderr, "rm: missing operand\n");
        return 1;
    }

    /**< Do remove. */
    for (i = 1; args[i] != NULL; i++) {
        file = find_fcb(args[i]);
        if (file == NULL) {
            fprintf(stderr, "rm: cannot remove %s: No such file\n", args[i]);
            return 1;
        }

        if (file->attribute == 0) {
            fprintf(stderr, "rm: cannot remove %s: Is a directory\n", args[i]);
            return 1;
        }

        /**< Check if the file exist in openfile_list. */
        for (j = 0; j < MAX_OPENFILE; j++) {
            if (openfile_list[j].free == 0) {
                continue;
            }

            if (!strcmp(file->filename, openfile_list[j].open_fcb.filename) &&
                file->first == openfile_list[j].open_fcb.first) {
                /**< Folder is open. */
                fprintf(stderr, "rm: cannot remove %s: File is open\n", args[i]);
                return 1;
            }
        }

        do_rm(file);
    }

    return 1;
}

/**
 * Just do remove file.
 * @param file FCB pointer which file you want to remove.
 */
void do_rm(fcb *file) {
    int first = file->first;

    file->free = 0;
    set_free(first, 0, 1);
}

/**
 * Open file.
 * @param args '-l' to show all files opened. 'path' to open file.
 * @return Always 1.
 */
int my_open(char **args) {
    int i, j;
    fcb *file;
    char path[PATHLENGTH];

    /**< Check argument count. */
    if (args[1] == NULL) {
        fprintf(stderr, "open: missing operand\n");
        return 1;
    }
    if (args[1][0] == '-') {
        if (!strcmp(args[1], "-l")) {
            printf("fd filename exname state path\n");
            for (i = 0; i < MAX_OPENFILE; i++) {
                if (openfile_list[i].free == 0) {
                    continue;
                }

                printf("%2d %8s %-6s %-5d %s\n", i, openfile_list[i].open_fcb.filename,
                       openfile_list[i].open_fcb.exname,
                       openfile_list[i].fcb_state, openfile_list[i].dir);
            }
            return 1;
        } else {
            fprintf(stderr, "open: wrong argument\n");
            return 1;
        }
    }

    /**< Do open. */
    for (i = 1; args[i] != NULL; i++) {
        file = find_fcb(args[i]);
        if (file == NULL) {
            fprintf(stderr, "open: cannot open %s: No such file or folder\n", args[i]);
            return 1;
        }

        /**< Check if the file exist in openfile_list. */
        for (j = 0; j < MAX_OPENFILE; j++) {
            if (openfile_list[j].free == 0) {
                continue;
            }

            if (!strcmp(file->filename, openfile_list[j].open_fcb.filename) &&
                file->first == openfile_list[j].open_fcb.first) {
                /**< file is open. */
                fprintf(stderr, "open: cannot open %s: File or folder is open\n", args[i]);
                continue;
            }
        }

        do_open(get_abspath(path, args[i]));
    }
    return 1;
}

/**
 * Just do open file.
 * @param path Abspath of file you want to open..
 * @return Error with -1, else return fd;
 */
int do_open(char *path) {
    int fd = get_useropen();
    fcb *file = find_fcb(path);

    if (fd == -1) {
        fprintf(stderr, "open: cannot open file, no more useropen entry\n");
        return -1;
    }
    fcb_cpy(&openfile_list[fd].open_fcb, file);
    openfile_list[fd].free = 1;
    openfile_list[fd].count = 0;
    memset(openfile_list[fd].dir, '\0', 80);
    strcpy(openfile_list[fd].dir, path);

    return fd;
}

/**
 * Close file and save it.
 * @param args '-a' to close all file. 'path' to close file.
 * @return Always 1.
 */
int my_close(char **args) {
    int i, j;
    fcb *file;

    /**< Check argument count. */
    if (args[1] == NULL) {
        fprintf(stderr, "close: missing operand\n");
        return 1;
    }
    if (args[1][0] == '-') {
        if (!strcmp(args[1], "-a")) {
            for (i = 0; i < MAX_OPENFILE; i++) {
                if (i == curdir) {
                    continue;
                }
                openfile_list[i].free = 0;
            }
            return 1;
        } else {
            fprintf(stderr, "close: wrong argument\n");
            return 1;
        }
    }


    /**< Do close. */
    for (i = 1; args[i] != NULL; i++) {
        file = find_fcb(args[i]);
        if (file == NULL) {
            fprintf(stderr, "close: cannot close %s: No such file or folder\n", args[i]);
            return 1;
        }

        /**< Check if the file exist in openfile_list. */
        for (j = 0; j < MAX_OPENFILE; j++) {
            if (openfile_list[j].free == 0) {
                continue;
            }

            if (!strcmp(file->filename, openfile_list[j].open_fcb.filename) &&
                file->first == openfile_list[j].open_fcb.first) {
                /**< File is open. */
                do_close(j);
            }
        }
    }
    return 1;
}

/**
 * Just do close file.
 * @param fd File descriptor.
 */
void do_close(int fd) {
    if (openfile_list[fd].fcb_state == 1) {
        fcb_cpy(find_fcb(openfile_list[fd].dir), &openfile_list[fd].open_fcb);
    }
    openfile_list[fd].free = 0;
}

/**
 * Write file.
 * @param args [-a|-c|-w] append|cover|write, 'path' path of file.
 * @return
 */
int my_write(char **args) {
    int i, j = 0, flag = 0;
    int mode = 'w';
    char c;
    char path[PATHLENGTH];
    char str[WRITE_SIZE];
    fcb *file;

    /**< Check for arguments count. */
    for (i = 1; args[i] != NULL; i++) {
        if (args[i][0] == '-') {
            if (!strcmp(args[i], "-w")) {
                mode = 'w';
            } else if (!strcmp(args[i], "-c")) {
                mode = 'c';
            } else if (!strcmp(args[i], "-a")) {
                mode = 'a';
            } else {
                fprintf(stderr, "write: wrong argument\n");
                return 1;
            }
        } else {
            flag += 1 << i;
        }
    }
    if ((flag == 0) || (flag > 4) || i > 3) {
        fprintf(stderr, "write: wrong argument\n");
        return 1;
    }

    /**< Check if it's a file or folder. */
    strcpy(path, args[flag >> 1]);
    if ((file = find_fcb(path)) == NULL) {
        fprintf(stderr, "write: File not exists\n");
        return 1;
    }
    if (file->attribute == 0) {
        fprintf(stderr, "write: cannot access a folder\n");
        return 1;
    }

    memset(str, '\0', WRITE_SIZE);
    /**< Check if it's open. */
    for (i = 0; i < MAX_OPENFILE; i++) {
        if (openfile_list[i].free == 0) {
            continue;
        }

        if (!strcmp(file->filename, openfile_list[i].open_fcb.filename) &&
            file->first == openfile_list[i].open_fcb.first) {
            /**< File is open. */
            if (mode == 'c') {
                printf("Please input location: ");
                scanf("%d", &openfile_list[i].count);
                getchar();
            }
            while (1) {
                for (; (str[j] = getchar()) != '\n'; j++);
                j++;
                if ((c = getchar()) == '\n') {
                    break;
                } else {
                    str[j] = c;
                    j++;
                }
            }

            if (mode == 'c') {
                do_write(i, str, j - 1, mode);
            } else {
                do_write(i, str, j + 1, mode);
            }

            return 1;
        }
    }

    fprintf(stderr, "write: file is not open\n");
    return 1;
}

/**
 * @param fd File descriptor.
 * @param wstyle Write style.
 * @return Bytes write
 */
int do_write(int fd, char *content, size_t len, int wstyle) {
    //fat1表

    fat *fat1 = (fat *) (fs_head + BLOCK_SIZE);
    fat *fat2 = (fat *) (fs_head + 3*BLOCK_SIZE);

    //定义输入字符串数组，初始化
    char text[WRITE_SIZE] = {0};

    int write = openfile_list[fd].count;
    openfile_list[fd].count = 0;
    do_read(fd, openfile_list[fd].open_fcb.length, text);  //读取
    openfile_list[fd].count = write;

    int i = openfile_list[fd].open_fcb.first;

    char input[WRITE_SIZE] = {0};
    strncpy(input, content, len);
    //文件处理：截断写、覆盖写、追加写
    if (wstyle == 'w') {
        memset(text, 0, WRITE_SIZE);
        memcpy(text, input, len);
//        strncpy(text, input, len);
    } else if (wstyle == 'c') {
        memcpy(text + openfile_list[fd].count, input, len);
//        strncpy(&text[openfile_list[fd].count], input, len);
    } else if (wstyle == 'a') {
        memcpy(text + openfile_list[fd].count, input, len);
//        strncpy(&text[openfile_list[fd].open_fcb.length], input, len);
    }
    //写入文件系统
    int length = strlen(text); //需要写入的长度
    int num = length / BLOCK_SIZE + 1;
    int static_num = num;

    while (num) {
        char buf[BLOCK_SIZE] = {0};
        memcpy(buf, &text[(static_num - num) * BLOCK_SIZE], BLOCK_SIZE);
        unsigned char *p = fs_head + i * BLOCK_SIZE;
        memcpy(p, buf, BLOCK_SIZE);
        num = num - 1;
        if (num > 0) // 是否还有下一次循环
        {
            fat *fat_cur = fat1 + i;

            if (fat_cur->id == END)  //需要申请索引块
            {
                int next = get_free(1);
                fat_cur->id = next;
                fat_cur = fat1 + next;
                fat_cur->id = END;
            }
            i = (fat1 + i)->id;
        }
    }
    //这时的i是刚写完的最后一个磁盘块，剩下的磁盘块可以free掉

    if (fat1[i].id != END) {
        int j = fat1[i].id;
        fat1[i].id = END;
        i = j;
        while (fat1[i].id != END) {
            int m = fat1[i].id;
            fat1[i].id = FREE;
            i = m;
        }
        fat1[i].id = FREE;
    }

    memcpy(fat2, fat1, 2*BLOCK_SIZE);
    openfile_list[fd].open_fcb.length = length;
    openfile_list[fd].fcb_state = 1;
    return (strlen(input));


}

/**
 * Read file.
 * @param args [-s|-a] select|all, 'path' path of file.
 * @return Bytes read.
 */
int my_read(char **args) {
    int i, flag = 0;
    int length;
    int mode = 'a';
    char path[PATHLENGTH];
    char str[WRITE_SIZE];
    fcb *file;

    /**< Check for arguments count. */
    for (i = 1; args[i] != NULL; i++) {
        if (args[i][0] == '-') {
            if (!strcmp(args[i], "-s")) {
                mode = 's';
            } else if (!strcmp(args[i], "-a")) {
                mode = 'a';
            } else {
                fprintf(stderr, "read: wrong argument\n");
                return 1;
            }
        } else {
            flag += 1 << i;
        }
    }
    if ((flag == 0) || (flag > 4) || i > 3) {
        fprintf(stderr, "read: wrong argument\n");
        return 1;
    }

    /**< Check if it's a file or folder. */
    strcpy(path, args[flag >> 1]);
    if ((file = find_fcb(path)) == NULL) {
        fprintf(stderr, "read: File not exists\n");
        return 1;
    }
    if (file->attribute == 0) {
        fprintf(stderr, "read: cannot access a folder\n");
        return 1;
    }

    memset(str, '\0', WRITE_SIZE);
    /**< Check if it's open. */
    for (i = 0; i < MAX_OPENFILE; i++) {
        if (openfile_list[i].free == 0) {
            continue;
        }

        if (!strcmp(file->filename, openfile_list[i].open_fcb.filename) &&
            file->first == openfile_list[i].open_fcb.first) {
            /**< File is open. */
            if (mode == 'a') {
                openfile_list[i].count = 0;
                length = UINT16_MAX;
            }
            if (mode == 's') {
                printf("Please input location: ");
                scanf("%d", &openfile_list[i].count);
                printf("Please input length: ");
                scanf("%d", &length);
                printf("-----------------------");
            }
            do_read(i, length, str);
            fputs(str, stdout);
            return 1;
        }
    }

    fprintf(stderr, "read: file is not open\n");
    return 1;
}

/**
 * @param fd File descriptor.
 * @param len Length of text.
 * @param text Read file into text.
 * @return
 */
int do_read(int fd, int len, char *text) {
    memset(text, '\0', BLOCK_SIZE * 20);

    if (len <= 0) //想要读取0个字符
    {
        return 0;
    }

    fat *fat1 = (fat *) (fs_head + BLOCK_SIZE); //FAT1表
    int location = 0;//text的写入位置
    int length;
    int count = openfile_list[fd].count; //读写指针位置
    //排除了id出现end的情况
    if ((openfile_list[fd].open_fcb.length - count) >= len) //可以读取的字符多于想要读取的字符
    {
        length = len;  //想要读取的字符
    } else {
        length = openfile_list[fd].open_fcb.length - count; //只能读取这些字符
    }
    while (length) //需要读取的字符串个数
    {
        char *buf = (char *) malloc(BLOCK_SIZE); //申请空闲缓冲区
        int count = openfile_list[fd].count; //读写指针位置
        int logic_block_num = count / BLOCK_SIZE;//逻辑块号（起始为0）
        int off = count % BLOCK_SIZE;//块内偏移量
        int physics_block_num = openfile_list[fd].open_fcb.first;//文件起始物理块号（引导块号为0）

        for (int i = 0; i < logic_block_num; i++)  //物理块号
        {
            physics_block_num = (fat1 + physics_block_num)->id; //FAT第一项为0，若为1则physics_block_num-1
        }
        unsigned char *p = fs_head + BLOCK_SIZE * physics_block_num; //该物理块起始地址
        memcpy(buf, p, BLOCK_SIZE);

        if ((off + length) <= BLOCK_SIZE) {
            memcpy(&text[location], &buf[off], length);
            openfile_list[fd].count = openfile_list[fd].count + length;
            location += length;  //下一次写的位置
            length = length - length;  //lenght = 0 将退出循环
        } else {
            memcpy(&text[location], &buf[off], BLOCK_SIZE - off);
            openfile_list[fd].count += BLOCK_SIZE - off;
            location += BLOCK_SIZE - off;
            length = length - BLOCK_SIZE - off; //还剩下的想要读取的字节数
        }
    }

    return location;
}

/**
 * Exit system, save changes.
 * @author
 */
int my_exit_sys(void) {
    int i;
    FILE *fp;

    for (i = 0; i < MAX_OPENFILE; i++) {
        do_close(i);
    }

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
    int fat[BLOCK_NUM];

    /** Copy FAT. */
    for (i = 0; i < BLOCK_NUM; i++, fat0++) {
        fat[i] = fat0->id;
    }

    /** Find a continuous space. */
    for (i = 0; i < BLOCK_NUM - count; i++) {
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
 * @param length The blocks count.
 * @param mode 0 to allocate, 1 to reclaim and 2 to format.
 * @author Leslie Van
 */
int set_free(unsigned short first, unsigned short length, int mode) {
    fat *flag = (fat *) (fs_head + BLOCK_SIZE);
    fat *fat0 = (fat *) (fs_head + BLOCK_SIZE);
    fat *fat1 = (fat *) (fs_head + BLOCK_SIZE * 3);
    int i;
    int offset;

    for (i = 0; i < first; i++, fat0++, fat1++);

    if (mode == 1) {
        /**< Reclaim space. */
        while (fat0->id != END) {
            offset = fat0->id - (fat0 - flag) / sizeof(fat);
            fat0->id = FREE;
            fat1->id = FREE;
            fat0 += offset;
            fat1 += offset;
        }
        fat0->id = FREE;
        fat1->id = FREE;
    } else if (mode == 2) {
        /**< Format FAT */
        for (i = 0; i < BLOCK_NUM; i++, fat0++, fat1++) {
            fat0->id = FREE;
            fat1->id = FREE;
        }
    } else {
        /**< Allocate consecutive space. */
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
int set_fcb(fcb *f, const char *filename, const char *exname, unsigned char attr, unsigned short first,
            unsigned long length, char ffree) {
    time_t *now = (time_t *) malloc(sizeof(time_t));
    struct tm *timeinfo;
    time(now);
    timeinfo = localtime(now);

    memset(f->filename, 0, 8);
    memset(f->exname, 0, 3);
    strncpy(f->filename, filename, 7);
    strncpy(f->exname, exname, 2);
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
 * @return Destination fcb pointer.
 */
fcb *fcb_cpy(fcb *dest, fcb *src) {
    memset(dest->filename, '\0', 8);
    memset(dest->exname, '\0', 3);

    strcpy(dest->filename, src->filename);
    strcpy(dest->exname, src->exname);
    dest->attribute = src->attribute;
    dest->time = src->time;
    dest->date = src->date;
    dest->first = src->first;
    dest->length = src->length;
    dest->free = src->free;

    return dest;
}

/**
 * Translate relative path to absolute path
 * @param abspath Absolute path.
 * @param relpath Relative path.
 * @return Absolute path.
 */
char *get_abspath(char *abspath, const char *relpath) {
    /**< If relpath is abspath. */
    if (!strcmp(relpath, DELIM) || relpath[0] == '/') {
        strcpy(abspath, relpath);
        return 0;
    }

    char str[PATHLENGTH];
    char *token, *end;

    memset(abspath, '\0', PATHLENGTH);
    abspath[0] = '/';
    strcpy(abspath, current_dir);

    strcpy(str, relpath);
    token = strtok(str, DELIM);

    do {
        if (!strcmp(token, ".")) {
            continue;
        }
        if (!strcmp(token, "..")) {
            if (!strcmp(abspath, ROOT)) {
                continue;
            } else {
                end = strrchr(abspath, '/');
                if (end == abspath) {
                    strcpy(abspath, ROOT);
                    continue;
                }
                memset(end, '\0', 1);
                continue;
            }
        }
        if (strcmp(abspath, "/")) {
            strcat(abspath, DELIM);
        }
        strcat(abspath, token);
    } while ((token = strtok(NULL, DELIM)) != NULL);

    return abspath;
}

/**
 * Find fcb by abspath.
 * @param path File path.
 * @return File fcb pointer.
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
    int i, length = BLOCK_SIZE;
    char fullname[NAMELENGTH] = "\0";
    fcb *root = (fcb *) (BLOCK_SIZE * first + fs_head);
    fcb *dir;
    block0 *init_block = (block0 *) fs_head;
    if (first == init_block->root) {
        length = ROOT_BLOCK_NUM * BLOCK_SIZE;
    }

    for (i = 0, dir = root; i < length / sizeof(fcb); i++, dir++) {
        if (dir->free == 0) {
            continue;
        }
        get_fullname(fullname, dir);
        if (!strcmp(token, fullname)) {
            token = strtok(NULL, DELIM);
            if (token == NULL) {
                return dir;
            }
            return find_fcb_r(token, dir->first);
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

/**
 * Init a folder.
 * @param first Parent folder block num.
 * @param second Current folder block num.
 */
void init_folder(int first, int second) {
    int i;
    fcb *par = (fcb *) (fs_head + BLOCK_SIZE * first);
    fcb *cur = (fcb *) (fs_head + BLOCK_SIZE * second);

    set_fcb(cur, ".", "di", 0, second, BLOCK_SIZE, 1);
    cur++;
    set_fcb(cur, "..", "di", 0, first, par->length, 1);
    cur++;
    for (i = 2; i < BLOCK_SIZE / sizeof(fcb); i++, cur++) {
        cur->free = 0;
    }
}

/**
 * Get file full name.
 * @param fullname A char array[NAMELENGTH].
 * @param fcb1 Source fcb pointer.
 */
void get_fullname(char *fullname, fcb *fcb1) {
    memset(fullname, '\0', NAMELENGTH);

    strcat(fullname, fcb1->filename);
    if (fcb1->attribute == 1) {
        strncat(fullname, ".", 2);
        strncat(fullname, fcb1->exname, 3);
    }
}

/**
 * Translate unsigned short number to date string.
 * @param sdate Date to string.
 * @param date A number to represent date.
 * @return sdate.
 */
char *trans_date(char *sdate, unsigned short date) {
    int year, month, day;
    memset(sdate, '\0', 16);

    year = date & 0xfe00;
    month = date & 0x01e0;
    day = date & 0x001f;
    sprintf(sdate, "%04d-%02d-%02d", (year >> 9) + 1900, (month >> 5) + 1, day);
    return sdate;
}

/**
 * Translate unsigned short number to time string.
 * @param stime Time to string.
 * @param time A number to represent time.
 * @return stime.
 */
char *trans_time(char *stime, unsigned short time) {
    int hour, min, sec;
    memset(stime, '\0', 16);

    hour = time & 0xfc1f;
    min = time & 0x03e0;
    sec = time & 0x001f;
    sprintf(stime, "%02d:%02d:%02d", hour >> 11, min >> 5, sec << 1);
    return stime;
}