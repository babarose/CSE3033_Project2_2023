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

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_SIZE 64
#define MAX_FILE_NAME_SIZE 1024
#define MAX_LINE_SIZE 512
#define MAX_BOOKMARKS 10


void setup(char inputBuffer[], char *args[], int *background);
void executeCommand(char **args, int background);
int findExecutable(const char *command, char *fullPath);
void searchFiles(const char *searchString, int recursive);
void searchFilesKaragul(char *searchString, int recursive);
void searchFilesKaragulHelper(const char *filePath, const char *searchString);
void redirection(char **args);
void addBookmark(const char *command);
void listBookmarks();
void executeBookmark(int index);
void deleteBookmark(int index);
void exitShell();

volatile sig_atomic_t isRunningInBackground = 0;
char *bookmarks[MAX_BOOKMARKS];
int bookmarkCount = 0;


// Signal handler function
void handleCtrlZ(int signo) {
    if (signo == SIGTSTP) {
       
        printf("\nCtrl+Z received. Stopping the current process.\n");

    }else {
        printf("No foreground process to stop.\n");
    }
}

int main() {

    char inputBuffer[MAX_INPUT_SIZE];
    char *args[MAX_ARG_SIZE];
    int background;

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
        if (strcmp(args[0], "bookmark") == 0) {
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
            redirection(args);
            executeCommand(args, background);
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
            //isRunningInBackground = 0; // Reset isRunningInBackground
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


void redirection(char **args) {
    for (int i = 0; args[i] != NULL; ++i) {
        printf("test 1"  );
        if (strcmp(args[i], ">") == 0) {
            printf("test"  );
            char *outputFile = args[i + 1];
            int redirection_fd = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0666);
            if (redirection_fd == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(redirection_fd, STDOUT_FILENO);
            close(redirection_fd);

            args[i] = NULL; // Remove ">" from arguments
            args[i + 1] = NULL; // Remove the output file name
        } else if (strcmp(args[i], ">>") == 0) {
            char *outputFile = args[i + 1];
            int redirection_fd = open(outputFile, O_CREAT | O_APPEND | O_WRONLY, 0666);
            if (redirection_fd == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(redirection_fd, STDOUT_FILENO);
            close(redirection_fd);

            args[i] = NULL; // Remove ">>" from arguments
            args[i + 1] = NULL; // Remove the output file name
        } else if (strcmp(args[i], "2>") == 0) {
            char *outputFile = args[i + 1];
            int redirection_fd = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0666);
            if (redirection_fd == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(redirection_fd, STDERR_FILENO);
            close(redirection_fd);

            args[i] = NULL; // Remove "2>" from arguments
            args[i + 1] = NULL; // Remove the output file name
        } else if (strcmp(args[i], "<") == 0) {
            char *inputFile = args[i + 1];
            int redirection_fd = open(inputFile, O_RDONLY);
            if (redirection_fd == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(redirection_fd, STDIN_FILENO);
            close(redirection_fd);

            args[i] = NULL; // Remove "<" from arguments
            args[i + 1] = NULL; // Remove the input file name
        }
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
        char inputBuffer[MAX_INPUT_SIZE];
        char *args[MAX_ARG_SIZE];
        int background = 0;

        // Copy the bookmarked command to inputBuffer
        strcpy(inputBuffer, bookmarks[index]);

        // Setup and execute the bookmarked command
        setup(inputBuffer, args, &background);
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
