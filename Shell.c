#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
//#include <fctnl.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define MAX_PATH 1024
#define CREATE_FLAGS (O_WRONLY | O_CREAT | O_APPEND)
#define CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

pid_t pid;

// function prototypes
void setup(char inputBuffer[], char *args[], int *background);
void findExecutable(char *command, char *path, char **result);
void executeExternalCommand(char *args[], int background);
void executeInternalCommand(char *args[], int background);
void handleStopCommand();
void handleSearchCommand();
void handleBookmarkCommand();
void handleInternalCommand();
void handleIORedirection();

// main function
int main(void)
{
    char inputBuffer[MAX_LINE];   /* buffer to hold command entered */
    int background;               /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE / 2 + 1]; /* command line arguments */

    printf("###====================***Welcome to myshell! Type in your commands.***====================###\n");
    while (1)
    {
        background = 0;
        printf("myshell: ");

        /* setup() calls exit() when Control-D is entered */
        setup(inputBuffer, args, &background);

        /** the steps are:
         * (1) fork a child process using fork()
         * (2) the child process will invoke execv()
         * (3) if background == 0, the parent will wait,
         *     otherwise, it will invoke the setup() function again. */

        // Execute built-in commands
        if (strcmp(args[0], "^Z") == 0)
        {
            // Handle stop command
            handleStopCommand();
        }
        else if (strcmp(args[0], "search") == 0)
        {
            // Handle search command
            handleSearchCommand();
        }
        else if (strcmp(args[0], "bookmark") == 0)
        {
            // Handle bookmark command
            handleBookmarkCommand();
        }
        else if (strcmp(args[0], "exit") == 0)
        {
            // Terminate the Shell
            // Check for background processes before exiting============ may change later=======
            // Notify the user if there are background processes still running
            // and prompt the user to terminate them first
            printf("Checking for background processes...\n");
            // Add your code to check for background processes here
            exit(1);
        }
        else
        {
            // Execute External Command
            executeExternalCommand(args, background);
        }
    }
    return 0;
}

void executeExternalCommand(char *args[], int background) {
    // getenv() returns a string of the form dir1:dir2:dir3 each dir containing bin files
    char *path = getenv("PATH");
    char pathCopy[MAX_PATH];
    strcpy(pathCopy, path);
    // max number of directories in path is MAX_PATH/2
    char *directories[MAX_PATH / 2 + 1];

    // tokenize the path. each token is a directory
    int numOfDirs = 0;
    char *token = strtok(pathCopy, ":");
    while (token != NULL) {
        // another directory found, add it to the array
        directories[numOfDirs++] = token;
        token = strtok(NULL, ":");
    }
    // terminate the array
    directories[numOfDirs] = NULL;

    // loop through directories and try to find the bin file to execute the command
    int i;
    for (i = 0; i < numOfDirs; i++) {
        // append the command name to the directory
        char fullPath[MAX_PATH];
        strcpy(fullPath, directories[i]);
        if (fullPath[strlen(fullPath) - 1] != '/') {
            strcat(fullPath, "/");
        }
        // concatenate the executable command name to fullPath
        strcat(fullPath, args[0]);

        pid = fork();

        if (pid < 0) {
            // Fork Failure!
            fprintf(stderr, "Fork Failure! -> fork() System call failed to create a process!");
            exit(1);
        } else if (pid == 0) {
            // Child Process --> Execute the Command
            // Find the executable file path
            if (execv(fullPath, args)) {
                // printf("%s\n", "Executed! -> execv() is Successful!");
                // If execv was successful then we don't need to continue here                
            } else {
                // execv Failed!
                fprintf(stderr, "Execv Failure -> execv() is not Successful!\n");
                // Continue with next directories
                continue;
            }
        } else {
            // Parent Process --> Wait for Child Process to Finish
            if (background == 0) {
                // -- Wait for the child process to finish 
                // -- Store the exit status in 'status'
                int status;
                waitpid(pid, &status, 0);
                fprintf(stderr, "Child process exited with status %d\n", status);
            }

            // Command found and executed
            break;
        }
    }
    // command was not found
    if (i == numOfDirs) {
        fprintf(stderr, "Command not found: %s\n", args[0]);
    }
}



void executeInternalCommand(char *args[], int background)
{
    // Handle internal commands
}

void findExecutable(char *command, char *path, char **result)
{
    char *token = strtok(path, ":");

    while (token != NULL)
    {
        *result = malloc(strlen(token) + strlen(command) + 2);

        if (*result == NULL)
        {
            fprintf(stderr, "Memory Allocation Failure!");
            exit(1);
        }

        sprintf(*result, "%s/%s", token, command);

        if (access(*result, X_OK) == 0)
        {
            // Found the executable file
            return;
        }

        // Free Memory if not found
        free(*result);
        token = strtok(NULL, ":");
    }

    // Set to NULL if not found
    *result = NULL;
}

void handleInternalCommand()
{
    // Define type of the Internal command
}

void handleStopCommand()
{
    // Implementation of ^Z command
}

void handleSearchCommand()
{
    // Implementation of search command
}

void handleBookmarkCommand()
{
    // Implementation of bookmark command
}
/*
void handleIORedirection(char *args[]) {
    // Implementation of I/O redirection
    int fd_in = -1;
    int fd_out = -1;
    int fd_err = -1;

    for (int i = 1; args[i] != NULL; ++i) {
        if (strcmp(args[i], ">") == 0) {
            // Redirect output only
            fd_out = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC);
            if (fd_out == -1) {
                fprintf(stderr, "Open! -> File is Open!");
                exit(1);
            }

            // Replace stdout with the opened file descriptor + Close the file.
            dup2(stdout, STDOUT_FILENO);
            close(fd_out);

            // Remove ">" and the filename from args
            args[i] = NULL;
            break;
        } else if (strcmp(args[i], ">>") == 0) {
            // Append to file instead of overwriting it
            fd_out = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND);
            if (fd_out == -1) {
                fprintf(stderr, "Open! -> File is Open!");
                exit(1);
            }
            // Replace stdout with the opened file descriptor + Close the file.
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);

            // Remove ">" and the filename from args
            args[i] = NULL;
            break;
        } else if (strcmp(args[i], "<") == 0) {
            // Redirect input only
            fd_in = open(args[i + 1], O_RDONLY);
            if (fd_in == -1){
                fprintf(stderr, "Open! -> File is Open!");
                exit(1);
            }

            // Replace stdin with the opened file descriptor
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
            // Remove "<" and the filename from args
            args[i] = NULL;
            break;
        } else if (strcmp(args[i], "2>") == 0) {
            // Save stderr in a separate variable so we can close it later.
            fd_err = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC | O_SYNC);
            if (fd_err == -1) {
                fprintf(stderr, "Open! -> Error opening the File!");
                exit(1);
            }
            // Duplicate the new fd into stderr + Close the file.
            dup2(fd_err, STDERR_FILENO);
            close(fd_err);

            // Remove "2>" and the filename from args
            args[i] = NULL;
            break;
        }
    }
}
*/
/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */
void setup(char inputBuffer[], char *args[], int *background)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */

    ct = 0;

    /* read what the user enters on the command line */
    length = read(STDIN_FILENO, inputBuffer, MAX_LINE);

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0); /* ^d was entered, end of user command stream */

    /* the signal interrupted the read system call */
    /* if the process is in the read() system call, read returns -1
      However, if this occurs, errno is set to EINTR. We can check this  value
      and disregard the -1 value */
    if ((length < 0) && (errno != EINTR))
    {
        perror("error reading the command");
        exit(-1); /* terminate with error code of -1 */
    }

    printf(">>%s<<", inputBuffer);
    for (i = 0; i < length; i++)
    { /* examine every character in the inputBuffer */

        switch (inputBuffer[i])
        {
        case ' ':
        case '\t': /* argument separators */
            if (start != -1)
            {
                args[ct] = &inputBuffer[start]; /* set up pointer */
                ct++;
            }
            inputBuffer[i] = '\0'; /* add a null char; make a C string */
            start = -1;
            break;

        case '\n': /* should be the final char examined */
            if (start != -1)
            {
                args[ct] = &inputBuffer[start];
                ct++;
            }
            inputBuffer[i] = '\0';
            args[ct] = NULL; /* no more arguments to this command */
            break;

        default: /* some other character */
            if (start == -1)
                start = i;
            if (inputBuffer[i] == '&')
            {
                *background = 1;
                inputBuffer[i - 1] = '\0';
            }
        }            /* end of switch */
    }                /* end of for */
    args[ct] = NULL; /* just in case the input line was > 80 */

    for (i = 0; i <= ct; i++)
        printf("args %d = %s\n", i, args[i]);
} /* end of setup routine */
