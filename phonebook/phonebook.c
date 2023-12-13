#include <linux/phonebook.h>
#include <linux/spinlock.h>
#include <linux/syscalls.h>

static struct phonebook *phonebook;
static DEFINE_SPINLOCK(lock);

int register_phonebook(struct phonebook *pb)
{
	spin_lock(&lock);

	int status;
	if (!phonebook) {
		phonebook = pb;
		status = 0;
	} else {
		status = -EBUSY;
	}

	spin_unlock(&lock);
	return status;
}

int unregister_phonebook(struct phonebook *pb)
{
	spin_lock(&lock);

	int status;
	if (phonebook == pb) {
		phonebook = NULL;
		status = 0;
	} else {
		status = -EINVAL;
	}

	spin_unlock(&lock);
	return status;
}
