#include <linux/phonebook.h>
#include <linux/spinlock.h>
#include <linux/syscalls.h>

static struct phonebook *book;
static DEFINE_SPINLOCK(lock);

int register_phonebook(struct phonebook *pb)
{
	spin_lock(&lock);

	int status;
	if (!book) {
		book = pb;
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
	if (book == pb) {
		book = NULL;
		status = 0;
	} else {
		status = -EINVAL;
	}

	spin_unlock(&lock);
	return status;
}

EXPORT_SYMBOL_GPL(register_phonebook);
EXPORT_SYMBOL_GPL(unregister_phonebook);

//////////////////////////////////////////////////////////////////////

#undef get_user
SYSCALL_DEFINE3(pb_get_user, const char __user *, last_name, size_t, len,
				struct phonebook_user __user *, user)
{
	printk(KERN_INFO "Phonebook get user");

	if (!book) {
		return -ENOSYS;	
	}

	return book->get_user(last_name, len, user);
}

SYSCALL_DEFINE1(pb_add_user, const struct phonebook_user __user *, user)
{
	printk(KERN_INFO "Phonebook add user");

	if (!book) {
		return -ENOSYS;
	}

	return book->add_user(user);
}

SYSCALL_DEFINE2(pb_del_user, const char __user *, last_name, size_t, len)
{
	printk(KERN_INFO "Phonebook del user");

	if (!book) {
		return -ENOSYS;
	}

	return book->del_user(last_name, len);
}
