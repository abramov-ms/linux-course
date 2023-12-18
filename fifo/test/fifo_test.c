#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <fcntl.h>
#include <sys/wait.h>

static void writer()
{
	int fifo = open("/dev/fifo", O_WRONLY);
	if (fifo < 0) {
		err(EXIT_FAILURE, "open");
	}

	char buffer[128];
	const char message[] = "ping #%d\n";
	for (int i = 0; i < 10; ++i) {
		sprintf(buffer, message, i);
		if (write(fifo, buffer, strlen(buffer) + 1) < 0) {
			err(EXIT_FAILURE, "write");
		}

		// Zzz.....
		usleep(300000);
	}

	exit(EXIT_SUCCESS);
}

static void print_all(const char *buffer, size_t size)
{
	size_t done = 0;
	while (done < size) {
		ssize_t bytes =
			write(STDOUT_FILENO, buffer + done, size - done);
		if (bytes < 0) {
			err(EXIT_FAILURE, "write");
		}
		done += bytes;
	}
}

static void reader()
{
	int fifo = open("/dev/fifo", O_RDONLY);
	if (fifo < 0) {
		err(EXIT_FAILURE, "open");
	}

	char buffer[4096];
	ssize_t bytes;
	while ((bytes = read(fifo, buffer, sizeof(buffer))) > 0) {
		print_all(buffer, bytes);
	}

	if (bytes < 0) {
		err(EXIT_FAILURE, "read");
	}

	exit(EXIT_SUCCESS);
}

int main()
{
	pid_t child;

	child = fork();
	if (!child) {
		reader();
	}

	child = fork();
	if (!child) {
		writer();
	}

	for (size_t i = 0; i < 2; ++i) {
		int status;
		wait(&status);
		if (!WIFEXITED(status) || WEXITSTATUS(status)) {
			fprintf(stderr, "Child exited with non-zero status\n");
			exit(EXIT_FAILURE);
		}
	}

	printf("Must have received 10 pings by now\n");

	return 0;
}
