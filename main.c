#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#define MAX_ARGS 400

// Function declarations handle input parsing and command execution
void tokenizeInput(char *inputLine, char *argv[MAX_ARGS], int *redirectFlag, int *sequenceFlag, int *parallelFlag, int *pipeFlag);
void runSimpleCommand(char *argv[MAX_ARGS]);
void runParallelCommands(char *argv[MAX_ARGS]);
void runSequentialCommands(char *argv[MAX_ARGS]);
void runRedirectedCommand(char *argv[MAX_ARGS]);
void runPipedCommands(char *argv[MAX_ARGS]);

// Signal handlers to intercept Ctrl+C and Ctrl+Z signals gracefully
void handleSIGINT(int signum) {
    printf("SIGINT (Ctrl+C) received. Terminating shell.\n");
    exit(signum);
}
void handleSIGTSTP(int signum) {
    printf("SIGTSTP (Ctrl+Z) received. Terminating shell.\n");
    exit(signum);
}

int main() {

    // Prevent shell from being interrupted by child status or terminal signals
    signal(SIGCHLD, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    char *argv[MAX_ARGS];
    char *inputBuffer = NULL;
    size_t bufferSize = 0;
    char *cwd;

    while (1) {
        cwd = getcwd(NULL, 0);
        printf("%s$", cwd);
        free(cwd);

        if (getline(&inputBuffer, &bufferSize, stdin) == -1)
            break;

        int isRedirect = 0, isSequential = 0, isParallel = 0, isPiped = 0;

        // Split user input into tokens and detect operators
        tokenizeInput(inputBuffer, argv, &isRedirect, &isSequential, &isParallel, &isPiped);

        if (argv[0] != NULL && strcmp(argv[0], "exit") == 0) {
            printf("Exiting shell...\n");
            free(inputBuffer);
            break;
        }

        if (argv != NULL && argv[0] != NULL && strlen(argv[0]) > 0) {
            if (isParallel)
                runParallelCommands(argv);
            else if (isSequential)
                runSequentialCommands(argv);
            else if (isRedirect)
                runRedirectedCommand(argv);
            else if (isPiped)
                runPipedCommands(argv);
            else
                runSimpleCommand(argv);
        }

        free(inputBuffer);
        inputBuffer = NULL;
        bufferSize = 0;
    }
    return 0;
}

// Tokenize input on spaces and check for special symbols setting flags accordingly
void tokenizeInput(char *inputLine, char *argv[MAX_ARGS], int *redirectFlag, int *sequenceFlag, int *parallelFlag, int *pipeFlag) {
    int index = 0, beganToken = 0;

    while (*inputLine == ' ') inputLine++;

    while (*inputLine != '\0' && index < MAX_ARGS - 1) {
        if (*inputLine == ' ' && beganToken) {
            beganToken = 0;
            *inputLine = '\0';
        } else if (*inputLine != ' ' && *inputLine != '\n' && !beganToken) {
            beganToken = 1;
            argv[index++] = inputLine;
        }
        inputLine++;
    }

    for (int i = 0; i < index; i++) {
        if (strcmp(argv[i], ">") == 0) *redirectFlag = 1;
        if (strcmp(argv[i], "&&") == 0) *parallelFlag = 1;
        if (strcmp(argv[i], "##") == 0) *sequenceFlag = 1;
        if (strcmp(argv[i], "|") == 0) *pipeFlag = 1;
    }

    if (index > 0) {
        char *newlinePos = strchr(argv[index - 1], '\n');
        if (newlinePos != NULL)
            *newlinePos = '\0';
    }
    argv[index] = NULL;
}

// Execute a single simple command or builtin cd
void runSimpleCommand(char *argv[MAX_ARGS]) {

    if (strcmp(argv[0], "cd") == 0) {
        if (argv[1] == NULL)
            chdir(getenv("HOME"));
        else if (chdir(argv[1]) == -1)
            printf("bash: cd: %s: No such file or directory\n", argv[1]);
        return;
    }

    pid_t childPid = fork();
    if (childPid < 0) exit(1);
    else if (childPid == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        if (execvp(argv[0], argv) == -1) {
            printf("Shell: Incorrect command\n");
            exit(1);
        }
    } else {
        int status;
        waitpid(childPid, &status, WUNTRACED);
    }
}

// Execute multiple commands in parallel separated by &&
void runParallelCommands(char *argv[MAX_ARGS]) {
    pid_t pids[MAX_ARGS];
    int processCount = 0, i = 0;

    while (argv[i] != NULL) {
        char *cmd[MAX_ARGS];
        int j = 0;

        while (argv[i] != NULL && strcmp(argv[i], "&&") != 0)
            cmd[j++] = argv[i++];
        if (argv[i] != NULL && strcmp(argv[i], "&&") == 0) i++;
        cmd[j] = NULL;

        if (cmd[0] != NULL && strcmp(cmd[0], "cd") == 0) {
            if (cmd[1] == NULL)
                chdir(getenv("HOME"));
            else if (chdir(cmd[1]) == -1)
                printf("bash: cd: %s: No such file or directory\n", cmd[1]);
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) exit(1);
        else if (pid == 0) {
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);

            if (execvp(cmd[0], cmd) == -1) {
                printf("Shell: Incorrect command\n");
                exit(1);
            }
        } else {
            pids[processCount++] = pid;
        }
    }

    for (int k = 0; k < processCount; k++) {
        int status;
        waitpid(pids[k], &status, WUNTRACED);
        if(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTSTP)
            kill(pids[k], SIGKILL);
    }
}

// Run multiple commands in sequence separated by ##
void runSequentialCommands(char *argv[MAX_ARGS]) {
    int i = 0;

    while (argv[i] != NULL) {
        char *cmd[MAX_ARGS];
        int j = 0;

        while (argv[i] != NULL && strcmp(argv[i], "##") != 0)
            cmd[j++] = argv[i++];
        if (argv[i] != NULL && strcmp(argv[i], "##") == 0)
            i++;
        cmd[j] = NULL;

        if (cmd[0] != NULL && strcmp(cmd[0], "cd") == 0) {
            if (cmd[1] == NULL)
                chdir(getenv("HOME"));
            else if(chdir(cmd[1]) == -1)
                printf("bash: cd: %s: No such file or directory\n", cmd[1]);
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) exit(1);
        else if (pid == 0) {
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);

            if (execvp(cmd[0], cmd) == -1) {
                printf("Shell: Incorrect command\n");
                exit(1);
            }
        } else {
            int status;
            waitpid(pid, &status, WUNTRACED);
        }
    }
}

// Execute command with output redirected via >
void runRedirectedCommand(char *argv[MAX_ARGS]) {
    char *cmd[MAX_ARGS];
    int i = 0, j = 0;

    while (argv[i] != NULL && strcmp(argv[i], ">") != 0)
        cmd[j++] = argv[i++];
    cmd[j] = NULL;

    i++;

    if (cmd[0] != NULL && strcmp(cmd[0], "cd") == 0) {
        if (cmd[1] == NULL)
            chdir(getenv("HOME"));
        else if (chdir(cmd[1]) == -1)
            printf("bash: cd: %s: No such file or directory\n", cmd[1]);
        return;
    }

    if (cmd[0] == NULL || argv[i] == NULL) {
        printf("Shell: Incorrect command\n");
        return;
    }

    pid_t pid = fork();
    if(pid < 0) exit(1);
    else if(pid == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        int fd = open(argv[i], O_CREAT | O_WRONLY | O_APPEND, 0666);
        if(fd == -1) {
            perror("open");
            exit(1);
        }

        if(dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            close(fd);
            exit(1);
        }

        close(fd);

        if(execvp(cmd[0], cmd) == -1) {
            printf("Shell: Incorrect command\n");
            exit(1);
        }
    } else {
        int status;
        waitpid(pid, &status, WUNTRACED);
    }
}

// Execute chain of piped commands connected via |
void runPipedCommands(char *argv[MAX_ARGS]) {
    int cmdCount = 0;
    int idx = 0;
    int inputFd = STDIN_FILENO;
    int prevPipeFd = -1;

    while (argv[idx] != NULL) {
        if (strcmp(argv[idx], "|") == 0) {
            if (cmdCount == 0) {
                printf("Shell: Incorrect command\n");
                return;
            }

            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("pipe");
                exit(1);
            }

            pid_t pid = fork();
            if (pid == 0) {
                signal(SIGINT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);

                if (dup2(inputFd, STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }

                if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(1);
                }

                close(pipefd[0]);
                close(pipefd[1]);
                if (prevPipeFd != -1) close(prevPipeFd);

                char *cmdArgs[MAX_ARGS];
                int j = idx - cmdCount, k = 0;
                while (j < idx)
                    cmdArgs[k++] = argv[j++];
                cmdArgs[k] = NULL;

                if (cmdArgs[0] != NULL && strcmp(cmdArgs[0], "cd") == 0) {
                    if (cmdArgs[1] == NULL || chdir(cmdArgs[1]) != 0)
                        printf("Shell: Incorrect command\n");
                    exit(0);
                }

                execvp(cmdArgs[0], cmdArgs);
                printf("Shell: Incorrect command\n");
                exit(1);
            } else if (pid > 0) {
                close(pipefd[1]);
                if (prevPipeFd != -1) close(prevPipeFd);
                waitpid(pid, NULL, 0);

                prevPipeFd = inputFd;
                inputFd = pipefd[0];
                cmdCount = 0;
            } else {
                perror("fork");
                exit(1);
            }
        } else {
            cmdCount++;
        }
        idx++;
    }

    if (cmdCount > 0) {
        pid_t pid = fork();
        if (pid == 0) {
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);

            if (dup2(inputFd, STDIN_FILENO) == -1) {
                perror("dup2");
                exit(1);
            }
            if (inputFd != STDIN_FILENO)
                close(inputFd);

            char *cmdArgs[MAX_ARGS];
            int j = idx - cmdCount, k = 0;
            while (j < idx)
                cmdArgs[k++] = argv[j++];
            cmdArgs[k] = NULL;

            if (cmdArgs[0] != NULL && strcmp(cmdArgs[0], "cd") == 0) {
                if (cmdArgs[1] == NULL || chdir(cmdArgs[1]) != 0)
                    printf("Shell: Incorrect command\n");
                exit(0);
            }

            execvp(cmdArgs[0], cmdArgs);
            printf("Shell: Incorrect command\n");
            exit(1);
        } else if (pid > 0) {
            if (inputFd != STDIN_FILENO)
                close(inputFd);
            waitpid(pid, NULL, 0);
        } else {
            perror("fork");
            exit(1);
        }
    }
}



