SYSCALL_DEFINE5(mysetnice, pid_t, pid, int, flag, int, nicevalue, void __user *,
                prio, void __user *, nice)
{
    int cur_prio, cur_nice;
    struct pid *ppid;
    struct task_struct *pcb;

    ppid = find_get_pid(pid);
    if (ppid == NULL)
        return EFAULT;

    pcb = pid_task(ppid, PIDTYPE_PID);

    if (flag == 1)
    {
        set_user_nice(pcb, nicevalue);
    }
    else if (flag != 0)
    {
        return EFAULT;
    }

    cur_prio = task_prio(pcb);
    cur_nice = task_nice(pcb);

    copy_to_user(prio, &cur_prio, sizeof(cur_prio));
    copy_to_user(nice, &cur_nice, sizeof(cur_nice));
    return 0;
}