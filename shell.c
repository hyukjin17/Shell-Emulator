#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "parser.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <limits.h>	/* PATH_MAX */

#define MAX_TOKENS 32

/*
 * change directory to home by default, or to a given directory
 */
int cd(int argc, char *argv[]) {
    int status = 0;

    if (argc == 0) { // sets to home directory if no arguments are given
        char *dir = getenv("HOME");
        chdir(dir);
    } else if (argc == 1) { // sets to given directory
        char *dir = argv[0];
        int complete = chdir(dir);
        if (complete == -1) { // prints error message in case of failure
            fprintf(stderr, "cd: %s\n", strerror(errno));
            status = 1;
        }
    } else { // does not accept more than 1 arguments
        fprintf(stderr, "cd: wrong number of arguments\n");
        status = 1;
    }

    return status;
}

/*
 * print current directory
 */
int pwd(int argc, char *argv[]) {
    int status;
    
    if (argc != 0) { // should have no arguments
        fprintf(stderr, "pwd: too many arguments\n");
        status = 1;
    } else {
        char buf[PATH_MAX];
        getcwd(buf, PATH_MAX);
        printf("%s\n", buf);
        status = 0;
    }
    return status;
}

/*
 * exit out of shell emulator
 */
int exit_shell(int argc, char *argv[]) {
    int status = 0;
    if (argc == 0) { // exit
        exit(0);
    } else if (argc == 1) { // set the status value to given integer
        exit(atoi(argv[0]));
    } else { // does not accept more than 1 arguments
        fprintf(stderr, "exit: too many arguments\n");
        status = 1;
    }

    return status;
}

/*
 * external commands with no I/O redirection
 * includes file redirects and pipes
 */
int external(int argc, char *argv[]) {

    // creates an array of commands (tokens separated by a pipe symbol)
    // max 16 commands since MAX_TOKENS = 32 and pipes count as tokens as well
    char *cmds[16][16];
    int cmd_count = 0, arg_count = 0;

    // error if first command is a pipe
    if (strcmp(argv[0], "|") == 0) {
        fprintf(stderr, "syntax error: missing command before pipe\n");
        return 1;
    }
    // error if last command is a pipe
    if (strcmp(argv[argc - 1], "|") == 0) {
        fprintf(stderr, "syntax error: missing command after pipe\n");
        return 1;
    }

    // split the tokens using the pipe as the delimiter
    bool empty_command = true;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "|") == 0) {
            if (empty_command) continue;
            cmds[cmd_count][arg_count] = NULL;
            cmd_count++;
            arg_count = 0;
            empty_command = true;
        } else {
            cmds[cmd_count][arg_count++] = argv[i];
            empty_command = false;
        }
    }
    cmds[cmd_count][arg_count] = NULL; // add final NULL after the last command
    cmd_count++;

    // create all the pipes
    int pipes[cmd_count - 1][2];
    for (int i = 0; i < cmd_count - 1; i++) {
        pipe(pipes[i]);
    }

    int pids[cmd_count]; // keeps track of pids of child processes

    // fork for each command in the array
    for (int i = 0; i < cmd_count; i++) {
        int pid = fork();
        if (pid == 0) {
            // creates the input-output redirection for every pipe
            if (i > 0)
                dup2(pipes[i - 1][0], 0);
            if (i < cmd_count - 1)
                dup2(pipes[i][1], 1);

            // close all pipes in the child
            for (int j = 0; j < cmd_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            int arg_count = 0;
            while (cmds[i][arg_count] != NULL) arg_count++;

            // this section checks for redirects inside each command
            int fd;
            for (int j = 0; j < arg_count; j++) { // loop through the tokens and check for < or >
                if (strcmp(cmds[i][j], ">") == 0 || strcmp(cmds[i][j], "<") == 0) { // redirects
                    if (j == 0) {
                        fprintf(stderr, "syntax error: missing command before %s\n", cmds[i][j]);
                        exit(EXIT_FAILURE);
                    }
                    int is_output_redirect = (strcmp(cmds[i][j], ">") == 0);
                    cmds[i][j] = NULL; // removes the redirect symbols from the token array
                    if (arg_count > j + 1) { // checks if filename is provided after the redirect
                        if (is_output_redirect) fd = open(cmds[i][j + 1], O_WRONLY|O_CREAT|O_TRUNC, 0666); // open a write file for '>'
                        else fd = open(cmds[i][j + 1], O_RDONLY); // open a read only file for '<'
                        if (fd == -1) { // file open failure
                            fprintf(stderr, "%s: %s\n", cmds[i][j + 1], strerror(errno));
                            exit(EXIT_FAILURE);
                        }
                        // redirect the input or output depending on the redirect symbol
                        if (is_output_redirect) {
                            dup2(fd, 1);
                        } else {
                            dup2(fd, 0);
                        }
                        
                        close(fd);
                        j++;// skip the filename and move to the next token
                    } else { // if filename is not provided
                        fprintf(stderr, "syntax error: missing filename after redirect\n");
                        exit(EXIT_FAILURE);
                    }
                }
            }

            // execute the commands
            execvp(cmds[i][0], cmds[i]);
            fprintf(stderr, "%s: %s\n", cmds[i][0], strerror(errno)); // if it reaches this point, it means execvp failed
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            pids[i] = pid; // parent process adds the pid of child process into array
        } else {
            fprintf(stderr, "fork failure");
            exit(EXIT_FAILURE);
        }
    }

    // close all pipes in parent
    for (int i = 0; i < cmd_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    int status;
    int exit_status;
    // wait for all child processes
    for (int i = 0; i < cmd_count; i++) {
        do {
            waitpid(pids[i], &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        exit_status = WEXITSTATUS(status);
    }

    return exit_status;
}

int main(int argc, char **argv)
{
    int last_status = 0; // keep track of the last status value before the command execution
    bool interactive = isatty(STDIN_FILENO);
    FILE *fp = stdin;

    if (argc == 2) {
        interactive = false;
        fp = fopen(argv[1], "r");
        if (fp == NULL) {
            fprintf(stderr, "%s: %s\n", argv[1], strerror(errno));
            exit(EXIT_FAILURE); /* see: man 3 exit */
        }
    }
    if (argc > 2) {
        fprintf(stderr, "%s: too many arguments\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    char line[1024], linebuf[1024];
    char *tokens[MAX_TOKENS];
    
    /* loop:
     *   if interactive: print prompt
     *   read line, break if end of file
     *   tokenize it
     *   print it out <-- your logic goes here
     */
    while (true) {
        if (interactive) {

            signal(SIGINT, SIG_IGN); /* ignore SIGINT=^C */
            
            /* print prompt. flush stdout, since normally the tty driver doesn't
             * do this until it sees '\n'
             */
            printf("$ ");
            fflush(stdout);
        }

        /* see: man 3 fgets (fgets returns NULL on end of file)
         */
        if (!fgets(line, sizeof(line), fp))
            break;

        /* read a line, tokenize it, and print it out
         */
        int n_tokens = parse(line, MAX_TOKENS, tokens, linebuf, sizeof(linebuf));

        int status = 0;
        if (n_tokens == 0) continue; // ignore empty line

        char qbuf[16];
        snprintf(qbuf, sizeof(qbuf), "%d", last_status); // prints last exit status into the buffer
        for (int i = 0; i < n_tokens; i++) {
            // goes through the array of tokens and replaces "$?" with the last exit status
            if (strcmp(tokens[i], "$?") == 0) {
                tokens[i] = qbuf;
            }
        } // "echo $?" now prints out the last status message and does not get overwritten by echo itself

        // runs cd, pwd, exit or any other external command
        if (strcmp(tokens[0], "cd") == 0) {
            status = cd(n_tokens - 1, tokens + 1);
        } else if (strcmp(tokens[0], "pwd") == 0) {
            status = pwd(n_tokens - 1, tokens + 1);
        } else if (strcmp(tokens[0], "exit") == 0) {
            status = exit_shell(n_tokens - 1, tokens + 1);
        } else {
            status = external(n_tokens, tokens);
        }
        
        last_status = status; // update the status value
    }

    if (interactive)            /* make things pretty */
        printf("\n");           /* to see why, try deleting this and then quit with ^D */
}
