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
#include <stdbool.h>
#include <sys/stat.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_SIZE 64
#define MAX_FILE_NAME_SIZE 1024
#define MAX_LINE_SIZE 512
#define MAX_BOOKMARKS 10
#define CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)


void setup(char inputBuffer[], char *args[], int *background);
void executeCommand(char **args, int background);
int findExecutable(const char *command, char *fullPath);
void searchFiles(const char *searchString, int recursive);
void searchFilesKaragul(char *searchString, int recursive);
void searchFilesKaragulHelper(const char *filePath, const char *searchString);
void redirection(char **args);

void unredirection() ;
void addBookmark(const char *command);
void listBookmarks();
void executeBookmark(int index);
void deleteBookmark(int index);
void exitShell();

volatile sig_atomic_t isRunningInBackground = 0;
char *bookmarks[MAX_BOOKMARKS];
int bookmarkCount = 0;
int redirection_fd = -1; // Dosya tanımlayıcısını global olarak tanımla
int unRedirection = 0;
int original_stdin;
int original_stdout;
int original_stderr;
int background;
char inputBuffer[MAX_INPUT_SIZE];
char *args[MAX_ARG_SIZE];

// Signal handler function
void handleCtrlZ(int signo) {
    if (signo == SIGTSTP) {
       
        printf("\nCtrl+Z received. Stopping the current process.\n");

    }else {
        printf("No foreground process to stop.\n");
    }
}

int main() {


    
    original_stdin = dup(STDIN_FILENO);  // Save the original file descriptor for stdin
    original_stdout = dup(STDOUT_FILENO);  // Save the original file descriptor for stdout
    original_stderr = dup(STDERR_FILENO);  // Save the original file descriptor for stderr


    while (1) {
        background = 0;
        char *searchString;
        int recursive;
        printf("myshell: ");
        fflush(stdout);

        setup(inputBuffer, args, &background);

        isRunningInBackground = background; // isRunningInBackground'ı ayarla

        if (!isRunningInBackground)
        {
            signal(SIGTSTP, handleCtrlZ); // Ctrl+Z'yi yakala
        }else if(isRunningInBackground){
            signal(SIGTSTP, SIG_IGN); // Ctrl+Z'yi yoksay
        }

        if (strcmp(args[0], "exit") == 0) {
            exitShell();
        }
        
            

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
        else if (strcmp(args[0], "bookmark") == 0) {
            if (args[1] != NULL) {
                if (strcmp(args[1], "-l") == 0) {
                    // List bookmarks
                    listBookmarks();
                } else if (strcmp(args[1], "-i") == 0) {
                    // Execute bookmark by index
                    if (args[2] != NULL) {
                        int index = atoi(args[2]);
                        executeBookmark(index);
                    } else {
                        fprintf(stderr, "Usage: bookmark -i index\n");
                    }
                } else if (strcmp(args[1], "-d") == 0) {
                    // Delete bookmark by index
                    if (args[2] != NULL) {
                        int index = atoi(args[2]);
                        deleteBookmark(index);
                    } else {
                        fprintf(stderr, "Usage: bookmark -d index\n");
                    }
                } else {
                    // Add new bookmark
                    int bookmarkIndex = 1;
                    char bookmarkCommand[MAX_INPUT_SIZE];
                    strcpy(bookmarkCommand, args[bookmarkIndex]);
                    for (int i = bookmarkIndex + 1; args[i] != NULL; i++) {
                        strcat(bookmarkCommand, " ");
                        strcat(bookmarkCommand, args[i]);
                    }
                    addBookmark(bookmarkCommand);
                }
            }
        }    
        else //part A
        {
            redirection(args); // Redirection işlemi
            executeCommand(args, background);
            unredirection(); // Unredirection işlemi
        }
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
        case '\"': // Çift tırnak karakterini kontrol et
                if (start == -1) {
                    start = i + 1;
                } else {
                    inputBuffer[i] = '\0';
                }
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
}

void executeCommand(char **args, int background)
{
    pid_t pid, wpid;
    int status;

    pid = fork();
    //printf("execude sa\n");
    //printf("args[0]: %s\n", args[0]);
    if (pid == 0)
    {   
        //printf("child burada\n");
        // Child process
        char fullPath[256];
        //printf("deneme\n");
        int result =findExecutable(args[0], fullPath);
        //printf("result: %d\n", result);
        if (result)
        {
            //printf("hata 0 \n");
            //printf("fullPath: %s\n", fullPath);
            /*for (int i = 0; args[i] != NULL; i++)
            {
                printf("args[%d]: %s\n", i, args[i]);
            }*/
            int execvResult = execv(fullPath, args);
            //printf("execvResult: %d\n", execvResult);
            if (execvResult == -1)
            {
                perror("myshell");
                exit(EXIT_FAILURE);
            }
            
            /*if (execv(fullPath, args) == -1)
            {
                printf("hata 1 \n");
                perror("myshell");
                
                exit(EXIT_FAILURE);
            }*/
           // printf("hata 3 \n");
        }
        else
        {
          //  printf("hata 2 \n");
            perror("myshell");
            exit(EXIT_FAILURE);
        }
        //printf("Result: %d\n", result);
        //printf("child burada 2\n");
    }
    else if (pid < 0)
    {
        // Error forking
        perror("myshell");
    }
    else
    {
        //printf("parent burada\n");
        if (!background)
        {
            do
            {
                wpid = waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));

        }
        else
        {
            printf("Background process ID: %d\n", pid);
            return;
        }
        //printf("parent burada 2\n");
    }
}

int findExecutable(const char *command, char *fullPath)
{
    char *pathEnv = getenv("PATH");
    if (pathEnv == NULL)
    {
        perror("getenv");
        return 0;
    }

    char *path = strdup(pathEnv);

    char *token = strtok(path, ":");

    while (token != NULL)
    {
        // snprintf kullanımı burada
        snprintf(fullPath, 255, "%s/%s", token, command);

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

    while (fgets(line, sizeof(line), file) != NULL)
    {
        lineNumber++;
        if (strstr(line, searchString) != NULL)
        {
            printf("%-5d: %s -> %s", lineNumber, filePath, line);
        }
    }

    fclose(file);
}

void concatenatePaths(const char *path1, const char *path2, char *result)
{
    strcpy(result, path1);
    strcat(result, "/");
    strcat(result, path2);
}

void searchFilesKaragul(char *searchString, int recursive)
{
    DIR *dir;
    struct dirent *entry;
    char currentDir[MAX_FILE_NAME_SIZE];
    char startDir[MAX_FILE_NAME_SIZE];

    if (getcwd(startDir, sizeof(startDir)) == NULL)
    {
        perror("getcwd");
        return;
    }

    if ((dir = opendir(startDir)) == NULL)
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
            concatenatePaths(startDir, entry->d_name, filePath);

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
            concatenatePaths(startDir, entry->d_name, subDir);

            // Recursively search subdirectories
            if (chdir(subDir) == -1)
            {
                perror("chdir");
                closedir(dir);
                return;
            }

            searchFilesKaragul(searchString, recursive);

            if (chdir(startDir) == -1)
            {
                perror("chdir");
                closedir(dir);
                return;
            }
        }
    }

    if (closedir(dir) == -1)
    {
        perror("closedir");
    }
}


void redirection(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0 || strcmp(args[i], "2>") == 0 || strcmp(args[i], "<") == 0) {
            // Check if there is a filename after the operator
            if (args[i + 1] != NULL) {
                char *fileName = args[i + 1];

                if (redirection_fd != -1) {
                    // Eğer önceki bir dosya tanımlayıcısı açıldıysa, kapatın
                    close(redirection_fd);
                }

                if (strcmp(args[i], ">") == 0) {
                    redirection_fd = open(fileName, O_CREAT | O_TRUNC | O_WRONLY, 0666);
                    dup2(redirection_fd, STDOUT_FILENO);

                } else if (strcmp(args[i], ">>") == 0) {
                    redirection_fd = open(fileName, O_CREAT | O_APPEND | O_WRONLY, 0666);
                    dup2(redirection_fd, STDOUT_FILENO);
                } else if (strcmp(args[i], "2>") == 0) {
                    redirection_fd = open(fileName, O_CREAT | O_TRUNC | O_WRONLY, 0666);
                    dup2(redirection_fd, STDERR_FILENO);
                } else if (strcmp(args[i], "<") == 0) {
                    redirection_fd = open(fileName, O_RDONLY);
                    dup2(redirection_fd, STDIN_FILENO);
                }

                if (redirection_fd == -1) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }

                // Remove the operator and filename from arguments
                args[i] = NULL;
                args[i + 1] = NULL;
                unRedirection = 1;
            } else {
                fprintf(stderr, "Error: Missing filename after %s\n", args[i]);
                exit(EXIT_FAILURE);
            }
        }
    }
}

void unredirection() {
    if (unRedirection) {
        close(redirection_fd);
        dup2(original_stdin, STDIN_FILENO);
        dup2(original_stdout, STDOUT_FILENO);
        dup2(original_stderr, STDERR_FILENO);
        unRedirection = 0;

    }
}


void addBookmark(const char *command) {
    if (bookmarkCount < MAX_BOOKMARKS) {
        bookmarks[bookmarkCount++] = strdup(command);
    } else {
        fprintf(stderr, "Bookmark list is full. Cannot add more bookmarks.\n");
    }
}
void listBookmarks() {
    int i;
    for (i = 0; i < bookmarkCount; ++i) {
        printf("%d \"%s\"\n", i, bookmarks[i]);
    }
}
void executeBookmark(int index) {
    if (index >= 0 && index < bookmarkCount) {
        // Copy the bookmarked command to inputBuffer
        strcpy(inputBuffer, bookmarks[index]);

        // Parçalama için geçici bir değişken
        char *token;

        // Parçala ve args array'ine ekle
        int i = 0;
        token = strtok(inputBuffer, " "); // Boşluklara göre parçala
        while (token != NULL && i < MAX_ARG_SIZE - 1) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }

        // Geri kalan args elemanlarını NULL olarak ayarla
        for (; i < MAX_ARG_SIZE; i++) {
            args[i] = NULL;
        }

        // Execute the command
        executeCommand(args, background);
    } else {
        fprintf(stderr, "Invalid bookmark index.\n");
    }
}
void deleteBookmark(int index) {
    if (index >= 0 && index < bookmarkCount) {
        free(bookmarks[index]);

        // Shift the remaining bookmarks
        for (int i = index; i < bookmarkCount - 1; ++i) {
            bookmarks[i] = bookmarks[i + 1];
        }

        --bookmarkCount;
    } else {
        fprintf(stderr, "Invalid bookmark index.\n");
    }
}
void exitShell() {
    printf("\n%d\n",isRunningInBackground);
    if (isRunningInBackground) {
        printf("There are background processes still running. Please wait for them to finish or terminate them.\n");
        return;
        
    }else {
    printf("Exiting the shell. Goodbye!\n");
    exit(0);
    }
}
