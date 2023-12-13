#include <err.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <phonebook/phonebook.h>
#include <uapi/asm-generic/unistd.h>

void test_chrdev() {
    printf("testing /dev/phonebook\n");

    int phonebook = open("/dev/phonebook", O_RDWR);
    if (phonebook == -1) {
        err(EXIT_FAILURE, "open");
    }

    struct phonebook_request request = {.opcode = phonebook_create,
                                        .payload = {.first_name = "Alistar",
                                                    .last_name = "Protossky",
                                                    .age = 14,
                                                    .phone_number = 69420}};
    if (write(phonebook, &request, sizeof(request)) != sizeof(request)) {
        err(EXIT_FAILURE, "phonebook create");
    }

    request.opcode = phonebook_get;
    if (write(phonebook, &request, sizeof(request)) != sizeof(request)) {
        err(EXIT_FAILURE, "phonebook get");
    }

    struct phonebook_user user = {};
    if (read(phonebook, &user, sizeof(user)) != sizeof(user)) {
        err(EXIT_FAILURE, "phonebook read");
    }

    if (memcmp(&request.payload, &user, sizeof(struct phonebook_user)) != 0) {
        fprintf(stderr, "got different response");
        exit(1);
    }

    printf("all good\n");
}

void test_syscalls() {
    syscall(__NR_pb_get_user);
    syscall(__NR_pb_add_user);
    syscall(__NR_pb_del_user);
}

int main() {
    test_syscalls();
    return 0;
}
