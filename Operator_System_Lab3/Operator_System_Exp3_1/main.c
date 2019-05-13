/******************************************************************************
 * @file        main.c
 *
 * @author      Arcana
 *
 * @date        2018.11.10
 *
 * @brief       CSH (A simple shell in C)
 *****************************************************************************/

#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/*
 * Funcion Declarations for builtin shell commands:
 */
int csh_cd(char **args);
int csh_help(char **args);
int csh_exit(char **args);
int csh_hello(char **args);
int csh_print_info(char **args);

/*
 * List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
    "cd",
    "help",
    "hello",
    "print_info",
    "exit"
};

int (*builtin_func[]) (char **) = {
    &csh_cd,
    &csh_help,
    &csh_hello,
    &csh_print_info,
    &csh_exit
};

int csh_num_builtins(void) {
    return sizeof(builtin_str) / sizeof(char*);
}

/*
 * Builtin function implementations
 */

/*
 * @brief Builtin command: change directory.
 * @param args List of args. args[0] is "cd". args[1] is the directory.
 * @return Always returns 1, to continue executing.
 */
int csh_cd(char **args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "csh: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("csh");
        }
    }
    return 1;
}

/*
 * @brief Builtin command: print help.
 * @param args List of args. Not examined.
 * @return Always return 1, to continue executing.
 */
int csh_help(char **args)
{
    int i;
    printf("A simple shell in C\n");
    printf("Type program names and arguments, and press enter.\n");
    printf("The following are built in:\n");

    for (i = 0; i < csh_num_builtins(); i++) {
        printf("  %s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n");
    return 1;
}

/*
 * @brief Builtin command: print hello and current time.
 * @param args List of args. Not examined.
 * @return Always return 1, to continue executing.
 */
int csh_hello(char **args)
{
    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    printf("Hello, Leslie Van.\n");
    printf("Current local time and date: %s", asctime(timeinfo));
    return 1;
}

/*
 * @brief Builtin command: print kernel info.
 * @param args List of args. Not examined.
 * @return Always return 1, to continue executing.
 */
int csh_print_info(char **args)
{
    system("uname -a");
    return 1;
}

/*
 * @brief Builtin command: exit.
 * @param args List of args. Not examined.
 * @return Always return 0, to terminate execution.
 */
int csh_exit(char **args)
{
    return 0;
}

/*
 * @brief Launch a program and wait for it to terminate
 * @param args Null terminated list of arguments.
 * @return Always return 1, to continue executing.
 */
int csh_launch(char **args)
{
    pid_t pid, wpid;
    int status;
    
    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("csh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        perror("csh");
    } else {
        // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

/*
 * @brief Execute shell built-in or launch program.
 * @param args Null terminated list of arguments.
 * @return 1 if the shell should continue running, 0 if it should terminate.
 */
int csh_execute(char **args)
{
    int i;
    if (args[0] == NULL) {
        // An empty command was entered
        return 1;
    }

    for (i = 0; i < csh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    // return csh_launch(args);
    printf("Command not found.\n");
    return 1;
}

/*
 * @brief Read a line of input from stdin.
 * @return The line from stdin.
 */
char *csh_read_line(void)
{
    char *line = NULL;
    ssize_t bufsize = 0;
    getline(&line, &bufsize, stdin);
    return line;
}

#define CSH_TOK_BUFSIZE 64
#define CSH_TOK_DELIM " \t\r\n\a"
/*
 * @brief Split a line into tokens.
 * @param line The line.
 * @return Null-terminated array of tokens.
 */
char **csh_split_line(char *line)
{
    int bufsize = CSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "csh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, CSH_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;
        
        if (position >= bufsize) {
            bufsize += CSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "csh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, CSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

/*
 * @brief Loop getting input and execting it.
 */
void csh_loop(void)
{
    char *line;
    char **args;
    int status;

    do {
        printf("> ");
        line = csh_read_line();
        args = csh_split_line(line);
        status = csh_execute(args);

        free(line);
        free(args);
    } while (status);
}

/*
 * @brief Main entry point.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return status code.
 */
int main(int argc, char **argv)
{
    csh_loop();

    return EXIT_SUCCESS;
}
