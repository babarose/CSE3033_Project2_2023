#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#define CREATE_FLAGS (O_WRONLY | O_CREAT | O_APPEND)
#define CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

int main(void) {
	int fd;

	fd = open("my.file", CREATE_FLAGS, CREATE_MODE);

	if (fd == -1) {
		perror("Failed to open my.file");
		return 1;
	}

	if (dup2(fd, STDOUT_FILENO) == -1) {
		perror("Failed to redirect standard output");
		return 1;
	}

	if (close(fd) == -1) {
		perror("Failed to close the file");
		return 1;
	}

	printf("Output will be seen in my.file\n");
	return 0;
}
