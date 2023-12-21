#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <stdbool.h>
#include <stdint.h>

#include <mmaneg/mmaneg.h>

int main()
{
	int mmaneg = open("/proc/mmaneg", O_WRONLY);
	if (mmaneg == -1) {
		err(EXIT_FAILURE, "open");
	}

	struct mmaneg_request request;
    request.op = mmaneg_listvma;
    if (write(mmaneg, &request, sizeof(request)) != sizeof(request)) {
        err(EXIT_FAILURE, "mmaneg_listvma");
    }

    uint8_t byte = 0;

    request.op = mmaneg_findpage;
    request.addr = (unsigned long)&byte;
    if (write(mmaneg, &request, sizeof(request)) != sizeof(request)) {
        err(EXIT_FAILURE, "mmaneg_findpage");
    }

    const unsigned char new_val = 42;
    request.op = mmaneg_writeval;
    request.val = new_val;
    if (write(mmaneg, &request, sizeof(request)) != sizeof(request)) {
        err(EXIT_FAILURE, "mmaneg_writeval");
    }

    if (byte != new_val) {
        fprintf(stderr, "mmaneg_writeval: %d != %d\n", byte, new_val);
        return EXIT_FAILURE;
    }

    printf("all good\n");

    close(mmaneg);
	return EXIT_SUCCESS;
}
