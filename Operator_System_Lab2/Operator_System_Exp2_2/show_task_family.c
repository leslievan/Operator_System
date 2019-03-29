// show_task_family.c
// created by 19-03-26
// Arcana
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/pid.h>
#include <linux/list.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");
static int pid;
module_param(pid, int, 0644);

static int __init show_task_family_init(void)
{
    struct pid *ppid;
    struct task_struct *p;
    struct task_struct *pos;

    // 通过进程的PID号pid一步步找到进程的进程控制块p
    ppid = find_get_pid(pid);
    if (ppid == NULL)
    {
        printk("[ShowTaskFamily] Error, PID not exists.\n");
        return -1;
    }
    p = pid_task(ppid, PIDTYPE_PID);

    // 格式化输出表头
    printk("%-10s%-20s%-6s%-6s\n", "Type", "Name", "PID", "State");
    printk("------------------------------------------\n");

    // Parent
    // 打印父进程信息
    printk("%-10s%-20s%-6d%-6d\n", "[P]", p->parent->comm, p->parent->pid,
           p->parent->state);

    // Siblings
    // 遍历父进程的子，即我的兄弟进程，输出信息
    // “我”同样是父进程的子进程，所以当二者进程PID号一致时，跳过不输出
    list_for_each_entry(pos, &(p->parent->children), sibling)
    {
        if (pos->pid == pid)
            continue;
        printk("%-10s%-20s%-6d%-6d\n", "[S]", pos->comm, pos->pid,
               pos->state);
    }

    // Children
    // 遍历”我“的子进程，输出信息
    list_for_each_entry(pos, &(p->children), sibling)
    {
        printk("%-10s%-20s%-6d%-6d\n", "[C]", pos->comm, pos->pid,
               pos->state);
    }
}

static void __exit show_task_family_exit(void)
{
    printk("[ShowTaskFamily] Module Uninstalled.\n");
}

module_init(show_task_family_init);
module_exit(show_task_family_exit);