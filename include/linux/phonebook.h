#ifndef LINUX_PHONEBOOK_H
#define LINUX_PHONEBOOK_H

#include <linux/types.h>

#include <phonebook/phonebook.h>

struct phonebook {
	int (*get_user)(const char __user *last_name, size_t len,
			struct phonebook_user __user *user);
	int (*add_user)(const struct phonebook_user __user *user);
	int (*del_user)(const char __user *last_name, size_t len);
};

int register_phonebook(struct phonebook *phonebook);
int unregister_phonebook(struct phonebook *phonebook);

#endif // LINUX_PHONEBOOK_H
