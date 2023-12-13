#ifndef LINUX_PHONEBOOK_H
#define LINUX_PHONEBOOK_H

#include <phonebook/phonebook.h>

struct phonebook {
	int (*get_user)(const char __user *last_name,
			struct phonebook_user __user *user);
	int (*add_user)(struct phonebook_user __user *user);
	int (*del_user)(const char __user *last_name);
};

int register_phonebook(struct phonebook *phonebook);
int unregister_phonebook(struct phonebook *phonebook);

#endif
