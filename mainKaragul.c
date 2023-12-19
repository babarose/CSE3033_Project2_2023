#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
 
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define MAX_PATH 1024
#define CREATE_FLAGS (O_WRONLY | O_CREAT | O_APPEND)
#define CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define MAX_BOOKMARKS 10
#define MAX_COMMAND_LENGTH 256
char bookmarks[MAX_BOOKMARKS][MAX_COMMAND_LENGTH];
int bookmarkCount = 0;

void setup(char inputBuffer[], char *args[],int *background);
void runCommand(char *args[], int background);
void internalCommand(char *args[], int background, pid_t childpid);

void stop(int background , pid_t childpid);
void stopForegroundProcess(pid_t childpid);

void search(char *args[], int background);
void searchFiles(const char *dirname, const char *searchString , int recursive);


void bookmark(char *args[], int background);
void listbookmarks();
void addBookmark(const char *command,char *args[], int background);
void deleteBookmark(int index);
void executeBookmark(int index);



void exit(char *args[], int background);


pid_t childpid; /* variable to store the child's pid */
 
int main(void)
{
    char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/2 + 1]; /*command line arguments */
    while (1){
        background = 0;
        printf("myshell: ");
        printf("My process ID is: %d\n", getpid());
        printf("My parent's process ID is: %d\n", getppid());
        /*setup() calls exit() when Control-D is entered */
        setup(inputBuffer, args, &background);               
        internalCommand(args, background, childpid);
    }
            
}



/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */
void setup(char inputBuffer[], char *args[],int *background)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */
    
    ct = 0;
        
    /* read what the user enters on the command line */
    length = read(STDIN_FILENO,inputBuffer,MAX_LINE);  

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */

/* the signal interrupted the read system call */
/* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
	exit(1);           /* terminate with error code of -1 */
    }

	printf(">>%s<<",inputBuffer);
    for (i=0;i<length;i++){ /* examine every character in the inputBuffer */

        switch (inputBuffer[i]){
	    case ' ':
	    case '\t' :               /* argument separators */
		if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
		    ct++;
		}
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
		start = -1;
		break;

            case '\n':                 /* should be the final char examined */
		if (start != -1){
                    args[ct] = &inputBuffer[start];     
		    ct++;
		}
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
		break;

	    default :             /* some other character */
		if (start == -1)
		    start = i;
                if (inputBuffer[i] == '&'){
		    *background  = 1;
                    inputBuffer[i-1] = '\0';
		}
	    } /* end of switch */
    }    /* end of for */
     args[ct] = NULL; /* just in case the input line was > 80 */

	for (i = 0; i <= ct; i++)
		printf("args %d = %s\n",i,args[i]);
} /* end of setup routine */
void runCommand(char *args[], int background)
{
  
    childpid = fork();
                        //fork failed
                        if (childpid == -1) {
                            perror("Failed to fork");
                            return 1;
                        }
                        //child process
                        else if (childpid==0)
                        {
                            if (execv(args[0], args) < 0) {
                                perror("Error on the exec call");
                                return 1;
                            }
                        }
                        //parent process
                        else {
                            //foreground process
                            if (background == 0) {
                                //if the process is not background process so it is foreground process
                                // wait for the child process to terminate
                                waitpid(childpid, NULL, 0);
                                printf("Foreground process %d terminated\n", childpid); //print the terminated process

                            }
                            //background process
                            else {
                                printf("Background process started with PID: %d\n", childpid);
                            }
                        }
}

void internalCommand(char *args[], int background, pid_t childpid){
    if (args[0] != NULL){
            if (strcmp(args[0], "^Z") == 0)
            {
                // Handle ^Z command
                stop(background,childpid);
            }
            else if (strcmp(args[0], "search") == 0)
            {
                // Handle search command
                const char *searchString = argv[1];
                int recursive = (argc > 2 && strcmp(argv[2], "-r") == 0);
                search(searchString, recursive);
            }
            else if (strcmp(args[0], "bookmark") == 0)
            {
                // Handle bookmark command

                bookmark(args, background);
            }
            else if (strcmp(args[0], "exit") == 0)
            {
                // Handle exit command

                exit(args, background);
            }
            else
            {
                // For non-internal commands, run the command as usual
                runCommand(args, background);
            }
        }
}

void stop ( int background, pid_t childpid){
    // Handle ^Z command
    // Stop the currently running foreground process
   if (background == 0)
    {
        // If the last process is running in the foreground
        if (childpid > 0)
        {
            stopForegroundProcess(childpid);
            printf("Foreground process %d stopped\n", childpid);
        }
        else
        {
            printf("No foreground process to stop\n");
        }
    }
    else
    {
        printf("Cannot stop background processes using ^Z\n");
    }
}
void stopForegroundProcess(pid_t childpid)
{
    // Stop the currently running foreground process
    if (kill(pid, SIGSTOP) == -1)
    {
        perror("Failed to stop the process");
        // Handle error as needed
    }
}
void search (const char *searchString, int recursive){

    searchFiles(".", searchString, recursive);
}

void searchFiles(const char *dirname, const char *searchString , int recursive){
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(dirname)) == NULL)
    {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        char path[MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name);

        if (entry->d_type == DT_DIR)
        {
            // Skip "." and ".." directories
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            if (recursive)
            {
                // Recursively search subdirectories
                searchFiles(path, searchString, recursive);
            }
        }
        else if (entry->d_type == DT_REG)
        {
            // Regular file, search its content
            FILE *file = fopen(path, "r");
            if (file)
            {
                char line[MAX_LINE]; //max path de olabilir emin degilim
                int lineNumber = 0;

                while (fgets(line, sizeof(line), file) != NULL)
                {
                    lineNumber++;

                    if (strstr(line, searchString) != NULL)
                    {
                        printf("%s:%d: %s", path, lineNumber, line);
                    }
                }

                fclose(file);
            }
            else
            {
                perror("fopen");
            }
        }
    }

    closedir(dir);
}

void bookmark(char *args[], int background){
    // Handle bookmark command
    if (args[1] != NULL)
    {
        if (strcmp(args[1], "-l") == 0)
        {
            // List bookmarks
            listbookmarks();
        }
        else if(strcmp(args[1], "-i") == 0 && args[2] != NULL){
            //Execute bookmark by index
            int index = atoi(args[2]);
            executeBookmark(index)

        }
        else if (strcmp(args[1], "-d") == 0 && args[2] != NULL)
        {
            // Delete bookmark
            int index = atoi(args[2]);
            deleteBookmark(index);
        }
        else
        {
            // Add a new bookmark
                char bookmarkCommand[MAX_COMMAND_LENGTH];
                strcpy(bookmarkCommand, args[1]);

                for (int i = 2; args[i] != NULL; i++)
                {
                    strcat(bookmarkCommand, " ");
                    strcat(bookmarkCommand, args[i]);
                }

                addBookmark(bookmarkCommand);
        }
    }
    else
    {
        printf("Missing bookmark name\n");
    }
}

void listbookmarks(){
    // List bookmarks

    //bir hata var ama ne oldugunu bulamadim
    printf("Bookmarks:\n");
    for (int i = 0; i < bookmarkCount; i++)
    {
        printf("%d: %s\n", i + 1, bookmarks[i]);
    }
}
void addBookmark(const char *command,char *args[], int background){
    // Add bookmark
    if (bookmarkCount < MAX_BOOKMARKS)
    {
        strcpy(bookmarks[bookmarkCount], args[1]);
        bookmarkCount++;
        printf("Bookmark added: %s\n", args[1]);
        printf("Bookmark added command: %s\n", command);
    }
    else
    {
        printf("Maximum number of bookmarks reached (%d)\n", MAX_BOOKMARKS);
    }
}

void deleteBookmark(int index){
    // Delete bookmark
    if (index > 0 && index <= bookmarkCount)
    {
        printf("Bookmark deleted %d: %s\n",index ,bookmarks[index - 1]);
        for (int i = index - 1; i < bookmarkCount - 1; i++)
        {
            strcpy(bookmarks[i], bookmarks[i + 1]);
        }
        bookmarkCount--;
    }
    else
    {
        printf("Invalid bookmark index\n");
    }
}

void executeBookmark(int index){
    // Execute bookmark by index
    if (index > 0 && index <= bookmarkCount)
    {
        printf("Executing bookmark %d: %s\n", index, bookmarks[index - 1]);
        char *args[MAX_LINE/2 + 1];
        int background = 0;
        setup(bookmarks[index - 1], args, &background);
        internalCommand(args, background, childpid);
    }
    else
    {
        printf("Invalid bookmark index\n");
    }
}