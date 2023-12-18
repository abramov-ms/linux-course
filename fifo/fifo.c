#include <linux/module.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/wait.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Max Abramov");
MODULE_DESCRIPTION("FIFO device driver");
MODULE_VERSION("0.1");

static char ring_buffer[PAGE_SIZE * 16];
static size_t ring_head, ring_tail;

static ssize_t ring_read(char __user *to, size_t size)
{
	size_t read;
	size_t errors;
	if (ring_head >= ring_tail) {
		read = min(size, ring_head - ring_tail);
		errors = copy_to_user(to, ring_buffer + ring_tail, read);
	} else {
		errors = 0;
		size_t read1 = min(size, sizeof(ring_buffer) - ring_tail);
		errors += copy_to_user(to, ring_buffer + ring_tail, read1);
		size_t read2 = min(size - read1, ring_head);
		errors += copy_to_user(to + read1, ring_buffer, read2);
		read = read1 + read2;
	}

	if (errors) {
		return -EFAULT;
	}

	ring_tail = (ring_tail + read) % sizeof(ring_buffer);
	return read;
}

static ssize_t ring_write(const char __user *from, size_t size)
{
	size_t written;
	size_t errors;
	if (ring_head < ring_tail) {
		written = min(size, ring_tail - ring_head - 1);
		errors = copy_from_user(ring_buffer + ring_head, from, written);
	} else {
		errors = 0;
		size_t written1 =
			min(size, sizeof(ring_buffer) - ring_head - !ring_tail);
		errors +=
			copy_from_user(ring_buffer + ring_head, from, written1);
		size_t written2 = ring_tail ? min(size - written1, ring_tail) :
					      0;
		errors +=
			copy_from_user(ring_buffer, from + written1, written2);
		written = written1 + written2;
	}

	if (errors) {
		return -EFAULT;
	}

	ring_head = (ring_head + written) % sizeof(ring_buffer);
	return written;
}

//////////////////////////////////////////////////////////////////////

static DEFINE_MUTEX(fifo_mutex);
static DECLARE_WAIT_QUEUE_HEAD(open_queue);
static DECLARE_WAIT_QUEUE_HEAD(read_queue);
static DECLARE_WAIT_QUEUE_HEAD(write_queue);
static int fifo_writers, fifo_readers;
static int monotonic_writers;

static void fifo_lock(void)
{
	mutex_lock(&fifo_mutex);
}

static void fifo_unlock(void)
{
	mutex_unlock(&fifo_mutex);
}

static bool fifo_can_open_read(int snapshot)
{
	int writers = READ_ONCE(fifo_writers);
	int monotonic = READ_ONCE(monotonic_writers);
	return writers || monotonic > snapshot;
}

static void fifo_open_read(void)
{
	++fifo_readers;
	wake_up_all(&open_queue);
	int snapshot = monotonic_writers;
	while (!fifo_can_open_read(snapshot)) {
		fifo_unlock();
		wait_event(open_queue, fifo_can_open_read(snapshot));
		fifo_lock();
	}
}

static void fifo_open_write(void)
{
	++fifo_writers;
	++monotonic_writers;
	wake_up_all(&open_queue);
	while (!fifo_readers) {
		fifo_unlock();
		wait_event(open_queue, fifo_readers);
		fifo_lock();
	}
}

static int fifo_open(struct inode *inode, struct file *file)
{
	fifo_lock();

	int ret;
	unsigned int mode = file->f_mode;
	if (!(mode & (FMODE_READ | FMODE_WRITE)) ||
	    (mode & FMODE_READ & FMODE_WRITE)) {
		ret = -EPERM;
		goto out;
	}

	if (mode & FMODE_READ) {
		fifo_open_read();
	} else {
		fifo_open_write();
	}

	ret = 0;

out:
	fifo_unlock();
	return ret;
}

static int fifo_release(struct inode *inode, struct file *file)
{
	fifo_lock();

	unsigned int mode = file->f_mode;
	if (mode & FMODE_READ) {
		--fifo_readers;
		wake_up_all(&write_queue);
	} else {
		--fifo_writers;
		wake_up_all(&read_queue);
	}

	fifo_unlock();
	return 0;
}

static bool fifo_empty(size_t head, size_t tail)
{
	return head == tail;
}

static bool fifo_readable(void)
{
	size_t head = READ_ONCE(ring_head);
	size_t tail = READ_ONCE(ring_tail);
	size_t writers = READ_ONCE(fifo_writers);
	return !fifo_empty(head, tail) || !writers;
}

static void fifo_wait_readable(void)
{
	fifo_unlock();
	wait_event(read_queue, fifo_readable());
	fifo_lock();
}

static ssize_t fifo_read(struct file *file, char __user *buffer, size_t size,
			 loff_t *offset)
{
	fifo_lock();

	while (!fifo_readable()) {
		fifo_wait_readable();
	}

	size_t read = ring_read(buffer, size);
	wake_up_all(&write_queue);

	fifo_unlock();
	return read;
}

static bool fifo_full(size_t head, size_t tail)
{
	return head + 1 == tail;
}

static bool fifo_writeable(void)
{
	size_t head = READ_ONCE(ring_head);
	size_t tail = READ_ONCE(ring_tail);
	int readers = READ_ONCE(fifo_readers);
	return !fifo_full(head, tail) || !readers;
}

static void fifo_wait_writeable(void)
{
	fifo_unlock();
	wait_event(write_queue, fifo_writeable());
	fifo_lock();
}

static ssize_t fifo_write(struct file *file, const char __user *buffer,
			  size_t size, loff_t *offset)
{
	fifo_lock();

	while (!fifo_writeable()) {
		fifo_wait_writeable();
	}

	ssize_t written;
	if (!fifo_readers) {
		written = -EPIPE;
		goto out;
	}

	written = ring_write(buffer, size);
	wake_up_all(&read_queue);

out:
	fifo_unlock();
	return written;
}

//////////////////////////////////////////////////////////////////////

static const char *CLS_NAME = "fifo";
static const char *DEV_NAME = "fifo";
static const struct class *cls;
static const struct device *dev;
static int major;

static struct file_operations fifo_fops = {
	.open = &fifo_open,
	.release = &fifo_release,
	.read = &fifo_read,
	.write = &fifo_write,
};

static int __init init_fifo(void)
{
	major = register_chrdev(0, DEV_NAME, &fifo_fops);
	if (major < 0) {
		printk(KERN_ALERT
		       "[fifo] Could not register character device\n");
		return major;
	}

	int ret;

	cls = class_create(CLS_NAME);
	if (IS_ERR(cls)) {
		ret = PTR_ERR(cls);
		goto err_cls;
	}

	dev = device_create(cls, /*parent=*/NULL, MKDEV(major, 0),
			    /*drvdata=*/NULL, DEV_NAME);
	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		goto err_dev;
	}

	init_waitqueue_head(&open_queue);
	init_waitqueue_head(&read_queue);
	init_waitqueue_head(&write_queue);

	printk(KERN_INFO
	       "[fifo] Created fifo device with major number %d at /dev/fifo\n",
	       major);
	return 0;

err_dev:
	class_destroy(cls);
err_cls:
	unregister_chrdev(major, DEV_NAME);
	return ret;
}

static void __exit exit_fifo(void)
{
	device_destroy(cls, MKDEV(major, 0));
	class_destroy(cls);
	unregister_chrdev(major, DEV_NAME);
	printk(KERN_INFO "[fifo] Removed fifo device\n");
}

module_init(init_fifo);
module_exit(exit_fifo);
