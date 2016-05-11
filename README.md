# Readme
#### Outline
This is a project for UIUC CS241 honor, and I build a **kernel module** to simulate the **Earliest Deadline First scheduling policy**. 

  - edf.c:  Implement most of the functions of the kernel module.
  - edf_task_list.c:  Implement the task list data structure for jobs and some helper functions.
  - userapp.c:  A user program to run and pass jobs to the kernel module.
  - pri_queue.c:  Try to use a priority queue to optimize the efficiency of scheduling jobs.

#### Setting Up The Environemnt, Compilation and Usage
  -  Since the project is building a kernel module, it's important to use a ** Linux virtual machine** so that the my own system will not crash if I encounter any errors.
  -  Ensure you have the **root permission** of the virtual machine.
  -  Download all the files of 241H from my repository.
  -  Compile all the files and get **userapp, edf_final.ko, edf_final.mod.c, edf_final.mod.o, edf_final.o, edf.o, modules.order, and Module.symvers**.
  -  ```sh
     $ Make
     ```
  - **Load** the edf module created to kernel
  - ```sh
     $ sudo insmod edf_final.ko
     ```
  - **Check** whether the edf module is loaded
  - ```sh
     $ lsmod
     ```
  - If the module is not loaded, **reboot** the virtual machine
  - ```sh
     $ shutdown -r 0
     ```
  - If the module is loaded, run multiple cases of the **userapp** to see jobs interleaving and preemption: for example, userapp 400 40 100 10000, userapp 1000 40 30 100000
  - ```sh
     $ userapp [period(ms)] [computation_time(ms)] [number of iterations] [length of each job]
    ```
  
#### Design Details
- In userapp.c, I record the wakeup time and the job finishing time for each iteration, and prints out each wakeup time and the time spent to do the job. I **pass the jobs** from the user space to the kernal through the ***proc file system*** and using function Register(simulated computation time, period, pid) to **transmit the job information**, function Yield(pid) to **tell the kernel a job finishes**, and function Deregister(pid) to **free the memory** and stop the threads doing work in the kernel. To help **compute the time used by each job**, I used some functions from  the ***library <sys/time.h>*** and a helper function timeval_substract.

- In edf_task_list.c, I implement a Linux ***task_list data structure*** which points to the current **process control block** of each job. Also I add the basic informations of the current job, including the ones passed from the userapp, like period, simulated computation time and pid, and the others that can help, like deadline, state(SLEEPING, RUNNING, READY), timer, and next_wake_up_time. Then I implement many task_list helper functions as follows. 
    ```sh
     edf_task_list_init: Initializes the task_list data structure
     edf_task_list_add: Adds a new entry for the given pid to the edf_task_list struct
     _edf_task_list_find: finds the entry for the given pid and returns a pointer to that entry
     edf_task_list_select_highest: Select the highest priority task in the list that has state Ready
     edf_task_list_delete_all: Delete all the entries in the list
     edf_task_list_to_str: Format the edf_task_list_struct to the string
     edf_task_list_remove: Remove the entry with given pid
    ```
- In edf.c, I have handle_register() to transmit the jobs data structure, handle_yield() to update the deadline and do the preemption or call dispatching thread, and handle_deregister() to remove a a job. Upon reads, I pass the pids of all registered tasks through on_open of the proc filesystem with a **sequence file**, to the user-space process. The **dispatching thread** is a thread running in the background which select the highest priority task in the list and does the **preemption** decision and action. I use **Linux scheuler API** to do the preemtion. **spin_lock_irqsave() and spin_lock_irqrestore()** are used in timer wakeup handler to avoid deadlocking.


### External Library
<linux/module.h> <linux/kernel.h> <linux/list.h> <linux/proc_fs.h> <linux/seq_file.h> <linux/string.h> <linux/timer.h> <linux/workqueue.h> <asm/uaccess.h> <linux/kthread.h> <linux/spinlock.h> <linxu/slab.h> <stdio.h> <stdlib.h> <sys/time.h> <errno.h> <string.h> <unistd.h>

