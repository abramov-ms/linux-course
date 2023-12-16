#include <err.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <phonebook/phonebook.h>
#include <uapi/asm-generic/unistd.h>

static void test_chrdev()
{
	printf("testing /dev/phonebook\n");

	int phonebook = open("/dev/phonebook", O_RDWR);
	if (phonebook == -1) {
		err(EXIT_FAILURE, "open");
	}

	struct phonebook_request request = { .opcode = phonebook_create,
					     .payload = {
						     .first_name = "Alistar",
						     .last_name = "Protossky",
						     .age = 14,
						     .phone_number = 69420 } };
	if (write(phonebook, &request, sizeof(request)) != sizeof(request)) {
		err(EXIT_FAILURE, "phonebook_create");
	}

	request.opcode = phonebook_get;
	if (write(phonebook, &request, sizeof(request)) != sizeof(request)) {
		err(EXIT_FAILURE, "phonebook_get");
	}

	struct phonebook_user user = {};
	if (read(phonebook, &user, sizeof(user)) != sizeof(user)) {
		err(EXIT_FAILURE, "phonebook_read");
	}

	if (request.payload.age != user.age ||
	    strcmp(request.payload.first_name, user.first_name) != 0 ||
	    strcmp(request.payload.last_name, user.last_name) != 0 ||
	    request.payload.phone_number != user.phone_number) {
		fprintf(stderr, "phonebook_get: got unexpected response\n");
		exit(EXIT_FAILURE);
	}

	request.opcode = phonebook_remove;
	if (write(phonebook, &request, sizeof(request)) != sizeof(request)) {
		err(EXIT_FAILURE, "phonebook_remove");
	}

	request.opcode = phonebook_get;
	if (write(phonebook, &request, sizeof(request)) != sizeof(request)) {
		err(EXIT_FAILURE, "phonebook_get");
	}

	if (read(phonebook, &user, sizeof(user)) != 0) {
		fprintf(stderr, "phonebook_get: got deleted user\n");
		exit(EXIT_FAILURE);
	}

    close(phonebook);
	printf("all good\n");
}

//////////////////////////////////////////////////////////////////////

static int pb_get_user(const char *last_name, size_t len,
		       struct phonebook_user *user)
{
	return syscall(__NR_pb_get_user, last_name, len, user);
}

static int pb_add_user(const struct phonebook_user *user)
{
	return syscall(__NR_pb_add_user, user);
}

static int pb_del_user(const char *last_name, size_t len)
{
	return syscall(__NR_pb_del_user, last_name, len);
}

static void test_syscalls()
{
	printf("testing syscalls\n");

	struct phonebook_user alistar = { .first_name = "Alistar",
					  .last_name = "Protossky",
					  .age = 14,
					  .phone_number = 69420 };

	if (pb_add_user(&alistar) != 0) {
		err(EXIT_FAILURE, "pb_add_user");
	}

	struct phonebook_user user;
	char last_name[] = "Protossky";
	if (pb_get_user(last_name, sizeof(last_name), &user) != 0) {
		err(EXIT_FAILURE, "pb_get_user");
	}

	if (alistar.age != user.age ||
	    strcmp(alistar.first_name, user.first_name) != 0 ||
	    strcmp(alistar.last_name, user.last_name) != 0 ||
	    alistar.phone_number != user.phone_number) {
		fprintf(stderr, "pb_get_user: got unexpected response\n");
		fprintf(stderr,
			"age: %d, first_name: %s, last_name: %s, phone_number: %lld",
			user.age, user.first_name, user.last_name,
			user.phone_number);
		exit(EXIT_FAILURE);
	}

	if (pb_del_user(last_name, sizeof(last_name)) != 0) {
		err(EXIT_FAILURE, "pb_del_user");
	}

	if (pb_get_user(last_name, sizeof(last_name), &user) == 0) {
		fprintf(stderr, "pb_get_user: got deleted user\n");
		exit(EXIT_FAILURE);
	}

	printf("all good\n");
}

//////////////////////////////////////////////////////////////////////

int main()
{
	test_chrdev();
	test_syscalls();
	return 0;
}
