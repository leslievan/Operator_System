# 杭州电子科技大学操作系统课程设计

- categories: ["Project"]
- tags: ["OS", "Lab", "HDU", "project"]
- keywords: ["杭电","杭州电子科技大学","HDU","操作系统实验","操作系统","实验","Linux","内核编译","进程管理"]
- alias: ["杭电操作系统实验", "HDU操作系统实验", "HDU操作系统"]



**实验列表**

- [x] [Lab1-Linux内核编译及添加系统调用](https://lsvm.xyz/2019/01/os-lab1/)
- [x] [Lab2-Linux内核模块编程](https://lsvm.xyz/2019/03/os-lab2/)
- [x] [Lab3-Linux进程管理（二）管道通信](https://lsvm.xyz/2019/04/os-lab-3-2/)
- [x] [Lab3-Linux进程管理（三）消息队列](https://github.com/leslievan/Operator_System/tree/master/Operator_System_Lab3)
- [x] [Lab3-Linux进程管理（四）共享内存](https://github.com/leslievan/Operator_System/tree/master/Operator_System_Lab5)
- [x] [Lab5-Linux文件系统](https://github.com/leslievan/Operator_System/tree/master/Operator_System_Lab5)

<!--more-->



## HDU-OS-Lab1-Linux内核编译及添加系统调用

添加一个系统调用，实现对指定进程的nice值的修改或读取功能，并返回进程最新的nice值及优先级prio。

> 视频教程地址：<https://www.bilibili.com/video/av47274857>
>
> 源码地址：<https://github.com/leslievan/Operator_System/tree/master/Operator_System_Lab1>

[阅读更多](https://lsvm.xyz/2019/01/os-lab1/)

## HDU-OS-Lab2-Linux内核模块编程

Linux内核采用了整体结构，上一个实验体会了编译内核时间的冗长与繁杂，一步错就要重新编译，这虽然提高了效率，但同时也让后续的维护变得困难，在这个基础上，Linux内核引入了动态模块机制加以改进。

> 视频教程地址：<https://www.bilibili.com/video/av47412869/>
>
> 源码地址：<https://github.com/leslievan/Operator_System/tree/master/Operator_System_Lab2>

[阅读更多](https://lsvm.xyz/2019/03/os-lab2)

## HDU-OS-Lab3-Linux进程管理（二）管道通信

实验三的知识点是进程通信，进程通信的方式多种多样，既包括锁机制、信号量机制在内的低级通信方式，低级在于其交换的信息量少且效率较低，又包括共享服务器、消息传递系统、管道通信以及客户-服务器系统通信在内的高级通信方式，本实验是实验三的第二个部分，介绍了管道通信方式的基本原理以及具体实现。

> 源码地址：<https://github.com/leslievan/Operator_System/tree/master/Operator_System_Lab3/Operator_System_Exp3_2>

[阅读更多](https://lsvm.xyz/2019/04/os-lab-3-2)
