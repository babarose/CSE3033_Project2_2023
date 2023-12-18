#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_SIZE 64

void setup(char inputBuffer[], char *args[], int *background);
void executeCommand(char **args, int background);
int findExecutable(const char *command, char *fullPath);

volatile sig_atomic_t isRunningInBackground = 0;

void handleCtrlZ(int signo) {
    if (signo == SIGTSTP) {
        if (isRunningInBackground) {
            printf("\nCtrl+Z received. Stopping the current process.\n");
            kill(getpid(), SIGSTOP); // Sadece ön plana alınan işlemi duraklat
        } else {
            printf("\nNo background process to stop.\n");
        }
    }
}

int main() {
    signal(SIGTSTP, handleCtrlZ);

    char inputBuffer[MAX_INPUT_SIZE];
    char *args[MAX_ARG_SIZE];
    int background;

    while (1) {
        background = 0;
        printf("myshell: ");
        fflush(stdout);
        setup(inputBuffer, args, &background);
        executeCommand(args, background);
    }

    return 0;
}

void setup(char inputBuffer[], char *args[], int *background) {
    int length, i, start, ct;

    ct = 0;
    start = -1;

    length = read(STDIN_FILENO, inputBuffer, MAX_INPUT_SIZE);

    if (length == 0)
        exit(0);

    if ((length < 0) && (errno != EINTR)) {
        perror("error reading the command");
        exit(-1);
    }

    for (i = 0; i < length; i++) {
        switch (inputBuffer[i]) {
            case ' ':
            case '\t':
                if (start != -1) {
                    args[ct] = &inputBuffer[start];
                    ct++;
                }
                inputBuffer[i] = '\0';
                start = -1;
                break;

            case '\n':
                if (start != -1) {
                    args[ct] = &inputBuffer[start];
                    ct++;
                }
                inputBuffer[i] = '\0';
                args[ct] = NULL;
                break;

            default:
                if (start == -1)
                    start = i;
                if (inputBuffer[i] == '&') {
                    if (i == length - 1 || inputBuffer[i + 1] == ' ' || inputBuffer[i + 1] == '\t' || inputBuffer[i + 1] == '\n') {
                        *background = 1;
                        inputBuffer[i] = '\0';
                    } else {
                        printf("myshell: syntax error near unexpected token `&'\n");
                        return;
                    }
                }
        }
    }

    args[ct] = NULL;
    isRunningInBackground = *background; // isRunningInBackground'ı ayarla
}

void executeCommand(char **args, int background) {
    pid_t pid, wpid;
    int status;

    pid = fork();

    if (pid == 0) {
        isRunningInBackground = 1;

        char fullPath[256];

        if (findExecutable(args[0], fullPath)) {
            if (execv(fullPath, args) == -1) {
                perror("myshell");
                exit(EXIT_FAILURE);
            }
        } else {
            perror("myshell");
            exit(EXIT_FAILURE);
        }
    } else if (pid < 0) {
        perror("myshell");
    } else {
        if (!background) {
            do {
                wpid = waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        } else {
            printf("Background process ID: %d\n", pid);
            return;
        }
    }
}

int findExecutable(const char *command, char *fullPath) {
    char *pathEnv = getenv("PATH");
    char *path = strdup(pathEnv);

    char *token = strtok(path, ":");

    while (token != NULL) {
        sprintf(fullPath, "%s/%s", token, command);
        if (access(fullPath, X_OK) == 0) {
            free(path);
            return 1; // Bulundu
        }

        token = strtok(NULL, ":");
    }

    free(path);
    return 0; // Bulunamadı
}
