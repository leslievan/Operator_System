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
    char *ptype[4] = {"[I]", "[P]", "[S]", "[C]"};

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

    // Itself
    // 打印自身信息
    printk("%-10s%-20s%-6d%-6d\n", ptype[0], p->comm, p->pid, p->state);

    // Parent
    // 打印父进程信息
    printk("%-10s%-20s%-6d%-6d\n", ptype[1], p->real_parent->comm,
           p->real_parent->pid, p->real_parent->state);

    // Siblings
    // 遍历父进程的子，即我的兄弟进程，输出信息
    // “我”同样是父进程的子进程，所以当二者进程PID号一致时，跳过不输出
    list_for_each_entry(pos, &(p->real_parent->children), sibling)
    {
        if (pos->pid == pid)
            continue;
        printk("%-10s%-20s%-6d%-6d\n", ptype[2], pos->comm, pos->pid,
               pos->state);
    }

    // Children
    // 遍历”我“的子进程，输出信息
    list_for_each_entry(pos, &(p->children), sibling)
    {
        printk("%-10s%-20s%-6d%-6d\n", ptype[3], pos->comm, pos->pid,
               pos->state);
    }

    return 0;
}

static void __exit show_task_family_exit(void)
{
    printk("[ShowTaskFamily] Module Uninstalled.\n");
}

module_init(show_task_family_init);
module_exit(show_task_family_exit);