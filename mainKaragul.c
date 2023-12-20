#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>


#define MAX_INPUT_SIZE 1024
#define MAX_ARG_SIZE 64
#define MAX_FILE_NAME_SIZE 1024
#define MAX_LINE_SIZE 512

void setup(char inputBuffer[], char *args[], int *background);
void executeCommand(char **args, int background);
int findExecutable(const char *command, char *fullPath);
void searchFiles(const char *searchString, int recursive);
void searchFilesKaragul(char *searchString, int recursive);
void searchFilesKaragulHelper(const char *filePath, const char *searchString);
void redirection(char **args);

volatile sig_atomic_t isRunningInBackground = 0;

void handleCtrlZ(int signo)
{
    if (signo == SIGTSTP)
    {
        if (isRunningInBackground)
        {
            printf("\nCtrl+Z received. Stopping the current process.\n");
            kill(0, SIGSTOP); // Stop all processes in the foreground group
            isRunningInBackground = 0;
        }
        else
        {
            printf("\nNo foreground process to stop.\n");
        }
    }
}

int main()
{

    signal(SIGTSTP, handleCtrlZ);
    char inputBuffer[MAX_INPUT_SIZE];
    char *args[MAX_ARG_SIZE];
    int background;

    while (1)
    {
        background = 0;
        char *searchString;
        int recursive;
        printf("myshell: ");
        fflush(stdout);
        setup(inputBuffer, args, &background);


        //search
        if (strcmp(args[0], "search") == 0)
        {
            // Handle search command separately
            if (args[1] != NULL)
            {
                if (strcmp(args[1], "-r") == 0)
                {
                    if (args[2] != NULL)
                    {
                        searchString = args[2];
                        recursive = 1;
                    }
                    else
                    {
                        printf("Usage: search -r filename or search filename\n");
                    }
                }
                else
                {
                    searchString = args[1];
                    recursive = 0;
                }
                searchFilesKaragul(searchString, recursive);
            }
            else 
            {
                printf("Usage: search [-r] <searchedString> \n");
            }
        }
        else //part A
        {
            //redirection(args);
            executeCommand(args, background);
        }
    }

    return 0;
}

void setup(char inputBuffer[], char *args[], int *background)
{
    int length, i, start, ct;

    ct = 0;
    start = -1;

    length = read(STDIN_FILENO, inputBuffer, MAX_INPUT_SIZE);

    if (length == 0)
        exit(0);

    if ((length < 0) && (errno != EINTR))
    {
        perror("error reading the command");
        exit(-1);
    }

    for (i = 0; i < length; i++)
    {
        switch (inputBuffer[i])
        {
        case ' ':
        case '\t':
            if (start != -1)
            {
                args[ct] = &inputBuffer[start];
                ct++;
            }
            inputBuffer[i] = '\0';
            start = -1;
            break;

        case '\n':
            if (start != -1)
            {
                args[ct] = &inputBuffer[start];
                ct++;
            }
            inputBuffer[i] = '\0';
            args[ct] = NULL;
            break;

        default:
            if (start == -1)
                start = i;
            if (inputBuffer[i] == '&')
            {
                if (i == length - 1 || inputBuffer[i + 1] == ' ' || inputBuffer[i + 1] == '\t' || inputBuffer[i + 1] == '\n')
                {
                    *background = 1;
                    inputBuffer[i] = '\0';
                }
                else
                {
                    printf("myshell: syntax error near unexpected token `&'\n");
                    return;
                }
            }
        }
    }

    args[ct] = NULL;
    isRunningInBackground = *background; // isRunningInBackground'ı ayarla
}

void executeCommand(char **args, int background)
{
    pid_t pid, wpid;
    int status;

    pid = fork();

    if (pid == 0)
    {
        // Child process
        char fullPath[256];



        if (findExecutable(args[0], fullPath))
        {
            if (execv(fullPath, args) == -1)
            {
                perror("myshell");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            perror("myshell");
            exit(EXIT_FAILURE);
        }
    }
    else if (pid < 0)
    {
        perror("myshell");
    }
    else
    {
        if (!background)
        {
            do
            {
                wpid = waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            isRunningInBackground = 0; // Reset isRunningInBackground
        }
        else
        {
            printf("Background process ID: %d\n", pid);
            return;
        }
    }
}

int findExecutable(const char *command, char *fullPath)
{
    char *pathEnv = getenv("PATH");
    char *path = strdup(pathEnv);

    char *token = strtok(path, ":");

    while (token != NULL)
    {
        sprintf(fullPath, "%s/%s", token, command);
        if (access(fullPath, X_OK) == 0)
        {
            free(path);
            return 1; // Bulundu
        }

        token = strtok(NULL, ":");
    }

    free(path);
    return 0; // Bulunamadı
}

void searchFilesKaragulHelper(const char *filePath, const char *searchString)
{
    FILE *file = fopen(filePath, "r");
    if (file == NULL)
    {
        perror("fopen");
        return;
    }

    char line[MAX_LINE_SIZE];
    int lineNumber = 0;

    // Sadece dosya adını al
    const char *fileName = strrchr(filePath, '/');
    fileName = (fileName != NULL) ? (fileName + 1) : filePath;

    while (fgets(line, sizeof(line), file) != NULL)
    {
        lineNumber++;
        if (strstr(line, searchString) != NULL)
        {
            printf("%d: \t./%s \t-> \t%s", lineNumber, fileName, line);
        }
    }

    fclose(file);
}

void searchFilesKaragul(char *searchString, int recursive)
{
    DIR *dir;
    struct dirent *entry;
    char currentDir[MAX_FILE_NAME_SIZE];

    if (getcwd(currentDir, sizeof(currentDir)) == NULL)
    {
        perror("getcwd");
        return;
    }

    if ((dir = opendir(currentDir)) == NULL)
    {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG)
        {
            // Dosyanın tam yolu
            char filePath[MAX_FILE_NAME_SIZE];
            strcpy(filePath, currentDir);
            strcat(filePath, "/");
            strcat(filePath, entry->d_name);

            // Dosya uzantısını kontrol et
            size_t len = strlen(filePath);
            if ((len > 2) &&
                ((filePath[len - 2] == '.') &&
                ((filePath[len - 1] == 'c') || (filePath[len - 1] == 'C') ||
                (filePath[len - 1] == 'h') || (filePath[len - 1] == 'H'))))
            {
                searchFilesKaragulHelper(filePath, searchString);
            }
        }
        else if (recursive && entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            char subDir[MAX_FILE_NAME_SIZE];
            strcpy(subDir, currentDir);
            strcat(subDir, "/");
            strcat(subDir, entry->d_name);

            // Recursively search subdirectories
            if (chdir(subDir) == -1)
            {
                perror("chdir");
                return;
            }

            searchFilesKaragul(searchString, recursive);

            if (chdir("..") == -1)
            {
                perror("chdir");
                return;
            }
        }
    }

    closedir(dir);
}


void redirection(char **args){
    if (args[MAX_ARG_SIZE - 2] == NULL){
        //no redirection
    }else if (strstr(args[MAX_ARG_SIZE - 2], ">") != NULL){
        //redirect output to file
        char *outputFile = args[MAX_ARG_SIZE - 1];
        int redirection_fd = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY);
        if (redirection_fd== -1 ){
            perror("open");
            exit(EXIT_FAILURE);
        }
        dup2(redirection_fd, STDOUT_FILENO);
        close(redirection_fd);

        args[MAX_ARG_SIZE - 2] = NULL; // Remove ">" from arguments
    }else if (strstr(args[MAX_ARG_SIZE - 2], ">>") != NULL){
        //redirect output to file - overwrite
        char *outputFile = args[MAX_ARG_SIZE - 1];
        int redirection_fd = open(outputFile, O_CREAT | O_APPEND | O_WRONLY);
        if (redirection_fd== -1 ){
            perror("open");
            exit(EXIT_FAILURE);
        }
        dup2(redirection_fd, STDOUT_FILENO);
        close(redirection_fd);

        args[MAX_ARG_SIZE - 2] = NULL; // Remove ">>" from arguments
    }else if(strstr(args[MAX_ARG_SIZE - 2], "2>") != NULL){
        //redirect error to file
        char *outputFile = args[MAX_ARG_SIZE - 1];
        int redirection_fd = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY , 0666);

        if (redirection_fd== -1 ){
            perror("open");
            exit(EXIT_FAILURE);
        }
        dup2(redirection_fd, STDOUT_FILENO);
        close(redirection_fd);

        args[MAX_ARG_SIZE - 2] = NULL; // Remove "2>" from arguments

    }else if(strstr(args[MAX_ARG_SIZE - 2], "<") != NULL){
        //redirect input to file
        char *inputFile = args[MAX_ARG_SIZE - 1];
        int redirection_fd = open(inputFile, O_RDONLY);
        if (redirection_fd== -1 ){
            perror("open");
            exit(EXIT_FAILURE);
        }
        dup2(redirection_fd, STDIN_FILENO);
        close(redirection_fd);

        args[MAX_ARG_SIZE - 2] = NULL; // Remove "<" from arguments
    }
}