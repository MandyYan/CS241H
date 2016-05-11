#ifndef _EDF_TASK_LIST_H
#define _EDF_TASK_LIST_H

#include <linux/list.h>
#include <linux/slab.h>
#include <linux/mutex.h>

// States
#define RUNNING 0
#define READY 1
#define SLEEPING 2

/**
 * The edf_task_list data structure contains information of tasks.
 */
struct edf_task_list_struct
{
    struct list_head head;
    size_t size;
    struct kmem_cache* cache;
    struct mutex lock;
};

/**
 * List entry.
 */
struct edf_task_struct
{
    struct timer_list timer;
    struct list_head list;
    struct task_struct * task;
    unsigned long pid;
    int state;
    unsigned long deadline;	// new
    unsigned long period;
    unsigned long computation_time;
    unsigned long next_wakeup;
};

/**
 * This function initializes the data structure.
 */
void edf_task_list_init(struct edf_task_list_struct* edf_task_list);

/**
 * This function adds a new entry for the given pid to the edf_task_list_struct struct
 * if the given pid does not exit in the list, do nothing otherwise.
 */
//new
void edf_task_list_add(unsigned long pid, unsigned long period, unsigned long computation_time, 
    void (*timer_wakeup_handler)(unsigned long), struct edf_task_list_struct* edf_task_list);

/**
 * This private function finds the entry for the given pid and returns a pointer to
 * that entry. If no entry is found for the given pid, return NULL.
 */
struct edf_task_struct* _edf_task_list_find(unsigned long pid, struct edf_task_list_struct* edf_task_list);

/**
 * Select the highest priority task in the list that has state READY.
 * Returns NULL if no task has state READY.
 */
struct edf_task_struct* edf_task_list_select_highest(struct edf_task_list_struct* edf_task_list);

/**
 * Delete all entries in the list.
 */
void edf_task_list_delete_all(struct edf_task_list_struct* edf_task_list);

/**
 * Format the edf_task_list_struct to the string. User should free the buffer using kfree.
 */
char* edf_task_list_to_str(struct edf_task_list_struct* edf_task_list);

/**
 * Remove the entry with given pid in the list.
 */
void edf_task_list_remove(unsigned long pid, struct edf_task_list_struct* edf_task_list);

#endif
