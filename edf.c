#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>

#include "edf_task_list.h"
#include "edf_find.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("16");
MODULE_DESCRIPTION("CS-423 edf");

#define DEBUG 1

static struct proc_dir_entry* edf_entry;
static struct proc_dir_entry* status_entry;
static struct edf_task_list_struct edf_task_list;
static struct edf_task_struct* running_task;
static struct task_struct* dispatching_thread;
static spinlock_t spinlock;


/**
 * The wakeup_timer interrupt handler.
 */
static void timer_wakeup_handler(unsigned long pid)
{
    unsigned long flags;
    struct edf_task_struct* calling_task;
    spin_lock_irqsave(&spinlock, flags);

    calling_task = _edf_task_list_find(pid, &edf_task_list);
    if (calling_task) {
        calling_task->state = READY;
    }

    spin_unlock_irqrestore(&spinlock, flags);
    wake_up_process(dispatching_thread);
}

/**
 * The dispatching thread implementation.
 */
static int dispatch(void* ptr)
{
    struct edf_task_struct* highest_ready;

#ifdef DEBUG
    printk(KERN_ALERT "Dispatching_thread created\n");
#endif

    while (1) {
        if (kthread_should_stop()) {
            return 0;
        }

#ifdef DEBUG
    printk(KERN_ALERT "Dispatching_thread iterating\n");
#endif

        highest_ready = edf_task_list_select_highest(&edf_task_list);

        if (running_task && (!highest_ready || highest_ready->deadline < running_task->deadline)) {
            struct sched_param sparam;
            sparam.sched_priority=0;
            sched_setscheduler(running_task->task, SCHED_NORMAL, &sparam);

            if (running_task->state == RUNNING) {
                running_task->state = READY;
            }
            running_task = NULL;
        }

        if (highest_ready && !running_task) {
            struct sched_param sparam;
            wake_up_process(highest_ready->task);
            sparam.sched_priority=90;
            sched_setscheduler(highest_ready->task, SCHED_FIFO, &sparam);

            running_task = highest_ready;
            running_task->state = RUNNING;
        }

        set_current_state(TASK_UNINTERRUPTIBLE);

#ifdef DEBUG
    printk(KERN_ALERT "Dispatching_thread going to sleep\n");
#endif

        schedule();
    }
    return 0;
}

/**
 * The helper function that registers a userspace process.
 */
static void handle_register(unsigned long pid, unsigned long period, unsigned long computation_time)
{
#ifdef DEBUG
    printk(KERN_ALERT "Handling register\n");
#endif

    printk(KERN_ALERT "Registration admitted\n");

    edf_task_list_add(pid, period, computation_time, timer_wakeup_handler, &edf_task_list);
    
}

/**
 * The helper function that handles the YIELD call of a userspace process.
 */
static void handle_yield(unsigned long pid)
{
    struct edf_task_struct* calling_task = _edf_task_list_find(pid, &edf_task_list);
    if (!calling_task) {
        return;
    }
    calling_task -> deadline += msecs_to_jiffies(calling_task -> period);
    
    if (calling_task->next_wakeup > jiffies) {printk("true\n");
        mod_timer(&calling_task->timer, calling_task->next_wakeup);

        calling_task->state = SLEEPING;
        wake_up_process(dispatching_thread);
        set_task_state(calling_task, TASK_UNINTERRUPTIBLE);
    }
    
    calling_task->next_wakeup += msecs_to_jiffies(calling_task->period);
}

/**
 * The helper function that de-registers a userspace process.
 */
static void handle_deregister(unsigned long pid)
{
    edf_task_list_remove(pid, &edf_task_list);
}

/**
 * Format the list to the string and print it to the give seq_file *.
 */
static int proc_show(struct seq_file* m, void* v)
{
    char* buffer = edf_task_list_to_str(&edf_task_list);
    seq_printf(m, buffer);
    kfree(buffer);
    return 0;
}

/**
 * Handles open. Returns a file descripter to the sequence file.
 */
static int on_open(struct inode *inode, struct file* file)
{
#ifdef DEBUG
    printk("on_open\n");
#endif
    return single_open(file, proc_show, NULL);
}

/**
 * This function parses the string written to the file. If the string is a valid pid, add an entry to the proc info list.
 */
static ssize_t on_write(struct file* file, const char __user *buff, size_t len, loff_t* pos)
{
    char input_str[80] = "";
    unsigned long pid, period, computation_time;

    if (len >= 80) {
        return -ENOSPC;
    }

    if (copy_from_user(&input_str, buff, len)) {
        return -EFAULT;
    }

    input_str[len] = '\0';
#ifdef DEBUG
    printk("on write: ");
    printk(input_str);
    printk("\n");
#endif

    if (3 == sscanf(input_str, "R %lu %lu %lu", &pid, &period, &computation_time)) {
        handle_register(pid, period, computation_time);
    } else if (1 == sscanf(input_str, "Y %lu", &pid)) {
        handle_yield(pid);
    } else if (1 == sscanf(input_str, "D %lu", &pid)) {
        handle_deregister(pid);
    }

    return len;
}

static const struct file_operations fops =
{
    .owner = THIS_MODULE,
    .open = on_open,
    .read = seq_read,
    .write = on_write,
    .llseek = seq_lseek,
    .release = single_release,
};

// edf_init - Called when module is loaded
int __init edf_init(void)
{
    struct sched_param sparam;

#ifdef DEBUG
    printk(KERN_ALERT "edf MODULE LOADING\n");
#endif

    // Create /proc/edf dir
    edf_entry = proc_mkdir("edf", NULL);
    if (edf_entry==NULL) return 1;

    // Create /proc/edf/status file
    status_entry = proc_create("status", 0666, edf_entry, &fops);
    if (status_entry==NULL) return 1;

    spin_lock_init(&spinlock);
    edf_task_list_init(&edf_task_list);
    running_task = NULL;

    dispatching_thread = kthread_run(dispatch, NULL, "dispatching_thread");
    if(!dispatching_thread)
    {
        printk(KERN_ALERT "kthread creation failed\n");
        return -ENOMEM;
    }
    printk(KERN_ALERT "kthread creation succeeded\n");
    sparam.sched_priority=99;
    sched_setscheduler(dispatching_thread, SCHED_FIFO, &sparam);

#ifdef DEBUG
    printk(KERN_ALERT "edf MODULE LOADED\n");
#endif
    return 0;
}

// edf_exit - Called when module is unloaded
void __exit edf_exit(void)
{
#ifdef DEBUG
    printk(KERN_ALERT "edf MODULE UNLOADING\n");
#endif

    remove_proc_entry("status", edf_entry); // Remove /proc/edf/status file
    remove_proc_entry("edf", NULL); // Remove /proc/edf dir
    edf_task_list_delete_all(&edf_task_list);
    kthread_stop(dispatching_thread);

#ifdef DEBUG
    printk(KERN_ALERT "edf MODULE UNLOADED\n");
#endif
}

// Register init and exit funtions
module_init(edf_init);
module_exit(edf_exit);
