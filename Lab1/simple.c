// simple.c — Part-2: Process Representation in Linux
// Prints selected task_struct fields for the init (swapper/idle) task.

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/sched.h>         // task_struct
#include <linux/sched/task.h>    // init_task
#include <linux/printk.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple Module: print init_task fields");
MODULE_AUTHOR("You");

/* Return numeric state compatible with newer kernels where t->state is hidden */
static unsigned long task_state_num(const struct task_struct *t)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,0)
    /* __state exists; direct state is no longer exported */
    return READ_ONCE(t->__state);
#else
    return READ_ONCE(t->state);
#endif
}

static int __init simple_init(void)
{
    const struct task_struct *t = &init_task;  // PID 0 swapper/idle

    pr_info("Loading Module\n");
    pr_info("init_task pid:%d\n", t->pid);
    pr_info("init_task state:%lu\n", task_state_num(t));
    pr_info("init_task flags:%lu\n", t->flags);
    pr_info("init_task runtime priority:%u\n", t->rt_priority);
    pr_info("init_task process policy:%u\n", t->policy);
    pr_info("init_task tgid:%d\n", t->tgid);

    return 0;
}

static void __exit simple_exit(void)
{
    pr_info("Removing Module\n");
}

module_init(simple_init);
module_exit(simple_exit);
