#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/keyboard.h>
#include <linux/i8042.h>
#include <asm/io.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Max Abramov");
MODULE_DESCRIPTION("Simple keylogger");
MODULE_VERSION("0.1");

#define I8042_KBD_IRQ 1
#define I8042_KBD_PORT 0x60
#define TIMEOUT_SECONDS 60

static volatile int anchor;

static int64_t keypresses;
static struct timer_list timer;

void reset_timer(void) {
    mod_timer(&timer, jiffies + TIMEOUT_SECONDS * HZ);
}

void log_keypresses(struct timer_list* timers) {
    printk(KERN_INFO "Got %lld keypresses last second\n", keypresses);
    reset_timer();
    keypresses = 0;
}

static irqreturn_t handle_keypress(int irq, void* cookie) {
    u16 scancode = inb(I8042_KBD_PORT);
    if (scancode & I8042_STR_PARITY) {
        ++keypresses;
    }

    return IRQ_HANDLED;
}

static int __init keylogger_init(void) {
    int error = request_irq(I8042_KBD_IRQ, &handle_keypress, IRQF_SHARED,
                            "keyboard", (void*)&anchor);
    if (error) {
        printk(KERN_ALERT "Could not load keylogger\n");
        return error;
    }

    timer_setup(&timer, &log_keypresses, /*flags=*/0);
    reset_timer();

    printk(KERN_INFO "Successfully loaded keylogger\n");
    return 0;
}

static void __exit keylogger_exit(void) {
    free_irq(I8042_KBD_IRQ, (void*)&anchor);
    del_timer_sync(&timer);
    printk(KERN_INFO "Unloaded keylogger");
}

module_init(keylogger_init);
module_exit(keylogger_exit);
