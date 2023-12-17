#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>

#include <mmaneg/mmaneg.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Max Abramov");
MODULE_DESCRIPTION("Memory management interface");
MODULE_VERSION("0.1");

//////////////////////////////////////////////////////////////////////

static void listvma(void)
{
	struct vma_iterator vmi;
	vma_iter_init(&vmi, current->mm, 0);
	struct vm_area_struct *vma;
	while ((vma = vma_next(&vmi))) {
		printk(KERN_INFO "[vma] [%lu, %lu)", vma->vm_start,
		       vma->vm_end);
	}
}

static void findpage(unsigned long addr)
{
	struct vm_area_struct *vma = find_vma(current->mm, addr);
	if (!vma) {
		printk(KERN_INFO "[vma] Could not translate %lu", addr);
		return;
	}

	printk(KERN_INFO "[vma] %lu -> %lu", addr, vma->vm_pgoff);
}

static void writeval(unsigned long addr, unsigned char val)
{
	size_t errors = copy_to_user((void *)addr, &val, sizeof(val));
	if (errors > 0) {
		printk(KERN_INFO "[vma] Failed to copy %d into %lu", val, addr);
	} else {
		printk(KERN_INFO "[vma] Copied %d into %lu", val, addr);
	}
}

//////////////////////////////////////////////////////////////////////

static struct proc_dir_entry *mmaneg;

int mmaneg_open(struct inode *inode, struct file *file)
{
	return 0;
}

int mmaneg_release(struct inode *inode, struct file *file)
{
	return 0;
}

ssize_t mmaneg_write(struct file *file, const char __user *buffer, size_t size,
		     loff_t *offset)
{
	if (size != sizeof(struct mmaneg_request)) {
		return -EINVAL;
	}

	struct mmaneg_request request;
	size_t errors = copy_from_user(&request, buffer, size);
	if (errors > 0) {
		return -EINVAL;
	}

	switch (request.op) {
	case mmaneg_listvma:
		listvma();
		break;
	case mmaneg_findpage:
		findpage(request.addr);
		break;
	case mmaneg_writeval:
		writeval(request.addr, request.val);
		break;
	default:
		return -EINVAL;
	}

	return size;
}

static struct proc_ops mmaneg_fops = {
	.proc_open = &mmaneg_open,
	.proc_release = &mmaneg_release,
	.proc_write = &mmaneg_write,
};

int __init mmaneg_init(void)
{
	mmaneg = proc_create("mmaneg", S_IRUSR, /*parent=*/NULL, &mmaneg_fops);
	if (!mmaneg) {
		return -ENOMEM;
	}

	printk(KERN_INFO "Entry /proc/mmaneg created");
	return 0;
}

void __exit mmaneg_exit(void)
{
	proc_remove(mmaneg);
	printk(KERN_INFO "Entry /proc/mmaneg removed");
}

module_init(mmaneg_init);
module_exit(mmaneg_exit);
