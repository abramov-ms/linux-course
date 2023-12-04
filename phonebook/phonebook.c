#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/rbtree.h>
#include <linux/mutex.h>

#include "phonebook.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Max Abramov");
MODULE_DESCRIPTION("Simple phonebook");
MODULE_VERSION("0.1");

static const char* CLS_NAME = "phonebook";
static const char* DEV_NAME = "phonebook";
static struct class* cls;
static struct device* dev;
static int dev_major;

//////////////////////////////////////////////////////////////////////

struct intrusive_phonebook_user {
    struct rb_node node;
    struct phonebook_user data;
};

static DEFINE_MUTEX(mutex);
struct rb_root users = RB_ROOT;
struct intrusive_phonebook_user* last_user;

struct intrusive_phonebook_user* search_user(const char* last_name) {
    struct rb_node* node = users.rb_node;
    while (node) {
        struct intrusive_phonebook_user* user =
            container_of(node, struct intrusive_phonebook_user, node);
        int ordering =
            strncmp(last_name, user->data.last_name, PHONEBOOK_NAME_MAX);
        if (ordering < 0) {
            node = node->rb_left;
        } else if (ordering > 0) {
            node = node->rb_right;
        } else {
            return user;
        }
    }

    return NULL;
}

bool insert_user(struct intrusive_phonebook_user* user) {
    struct rb_node** new = &users.rb_node;
    struct rb_node* parent = NULL;
    while (*new) {
        struct intrusive_phonebook_user* this =
            container_of(*new, struct intrusive_phonebook_user, node);
        int ordering = strncmp(user->data.last_name, this->data.last_name,
                               PHONEBOOK_NAME_MAX);
        parent = *new;
        if (ordering < 0) {
            new = &(*new)->rb_left;
        } else if (ordering > 0) {
            new = &(*new)->rb_right;
        } else {
            return false;
        }
    }

    rb_link_node(&user->node, parent, new);
    rb_insert_color(&user->node, &users);
    return true;
}

//////////////////////////////////////////////////////////////////////

static int phonebook_open(struct inode* inode, struct file* file) {
    mutex_lock(&mutex);
    return 0;
}

static int phonebook_release(struct inode* inode, struct file* file) {
    mutex_unlock(&mutex);
    return 0;
}

static ssize_t phonebook_read(struct file* file, char __user* buffer,
                              size_t size, loff_t* offset) {
    if (size != sizeof(struct phonebook_user)) {
        return -EINVAL;
    }

    if (!last_user) {
        return 0;
    }

    size_t errors =
        copy_to_user(buffer, &last_user->data, sizeof(struct phonebook_user));
    if (errors > 0) {
        return -EFAULT;
    }

    return sizeof(struct phonebook_user);
}

static ssize_t phonebook_write(struct file* file, const char __user* buffer,
                               size_t size, loff_t* offset) {
    if (size != sizeof(struct phonebook_request)) {
        return -EINVAL;
    }

    struct phonebook_request request;
    size_t errors = copy_from_user(&request, buffer, size);
    if (errors > 0) {
        printk(KERN_ALERT "Got bad phonebook request");
        return -EINVAL;
    }

    switch (request.opcode) {
        case phonebook_get: {
            printk(KERN_INFO "Got phonebook 'get' request\n");
            last_user = search_user(request.payload.last_name);
            break;
        }

        case phonebook_create: {
            printk(KERN_INFO "Got phonebook 'create' request\n");
            struct intrusive_phonebook_user* user =
                kmalloc(sizeof(struct intrusive_phonebook_user), GFP_KERNEL);
            memcpy(&user->data, &request.payload,
                   sizeof(struct phonebook_user));
            if (!insert_user(user)) {
                kfree(user);
                return -EEXIST;
            }
            break;
        }

        case phonebook_remove: {
            printk(KERN_INFO "Got phonebook 'remove' request\n");
            struct intrusive_phonebook_user* user =
                search_user(request.payload.last_name);
            if (user == NULL) {
                return -ENOENT;
            }
            rb_erase(&user->node, &users);
            kfree(user);
            break;
        }

        default: {
            printk(KERN_ALERT "Got bad phonebook opcode\n");
            return -EINVAL;
        }
    }

    return size;
}

//////////////////////////////////////////////////////////////////////

static struct file_operations fops = {.open = &phonebook_open,
                                      .release = &phonebook_release,
                                      .read = &phonebook_read,
                                      .write = &phonebook_write};

static int __init phonebook_init(void) {
    dev_major = register_chrdev(0, DEV_NAME, &fops);
    if (dev_major < 0) {
        printk(KERN_ALERT "Could not register phonebook device\n");
        return dev_major;
    }

    int err;

    cls = class_create(CLS_NAME);
    if (IS_ERR(cls)) {
        printk(KERN_ALERT "Could not create phonebook device class\n");
        err = PTR_ERR(cls);
        goto err_cls;
    }

    dev = device_create(cls, /*parent=*/NULL, MKDEV(dev_major, /*minor=*/0),
                        /*drvdata=*/NULL, DEV_NAME);
    if (IS_ERR(dev)) {
        printk(KERN_ALERT "Could not create phonebook device\n");
        err = PTR_ERR(dev);
        goto err_dev;
    }

    printk(KERN_INFO "Created /dev/phonebook with major number %d\n",
           dev_major);
    return 0;

err_dev:
    class_destroy(cls);
err_cls:
    unregister_chrdev(dev_major, DEV_NAME);
    return err;
}

static void __exit phonebook_exit(void) {
    device_destroy(cls, MKDEV(dev_major, /*minor=*/0));
    class_destroy(cls);
    unregister_chrdev(dev_major, DEV_NAME);
    printk(KERN_INFO "Removed /dev/phonebook\n");
}

module_init(phonebook_init);
module_exit(phonebook_exit);
