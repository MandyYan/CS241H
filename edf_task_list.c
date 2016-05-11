#include "edf_task_list.h"
#include <linux/string.h>

struct task_struct* find_task_by_pid(unsigned int nr);

void edf_task_list_init(struct edf_task_list_struct* edf_task_list)
{
    edf_task_list->size = 0;
    INIT_LIST_HEAD(&edf_task_list->head);
    mutex_init(&edf_task_list->lock);
    edf_task_list->cache = (struct kmem_cache*)kmem_cache_create("cache", sizeof(struct edf_task_struct), 0, GFP_KERNEL ,NULL);
}


void edf_task_list_add(unsigned long pid, unsigned long period, unsigned long computation_time, 
    void (*timer_wakeup_handler)(unsigned long), struct edf_task_list_struct* edf_task_list)
{
    struct edf_task_struct * new_task = kmem_cache_alloc(edf_task_list->cache, GFP_KERNEL);
    INIT_LIST_HEAD(&new_task->list);
    init_timer(&new_task->timer);
    setup_timer(&new_task->timer, timer_wakeup_handler, pid);
    new_task->task = find_task_by_pid(pid);
    new_task->pid = pid;
    new_task->state = SLEEPING;
    new_task->deadline = jiffies + msecs_to_jiffies(period);
    new_task->period = period;
    new_task->computation_time = computation_time;
    new_task->next_wakeup = jiffies + msecs_to_jiffies(period);
    mod_timer(&new_task->timer, new_task->next_wakeup);
    new_task->next_wakeup += msecs_to_jiffies(period);

    mutex_lock(&edf_task_list->lock);

    list_add(&new_task->list, &edf_task_list->head);
    edf_task_list->size += 1;

    mutex_unlock(&edf_task_list->lock);
}

/*Iterate through the list and find the job by its pid*/
struct edf_task_struct* _edf_task_list_find(unsigned long pid, struct edf_task_list_struct* edf_task_list)
{
    struct edf_task_struct* entry;

    list_for_each_entry(entry, &edf_task_list->head, list)
    {
        if (entry->pid == pid) {
            return entry;
        }
    }

    return NULL;
}

// compare the deadline
struct edf_task_struct* edf_task_list_select_highest(struct edf_task_list_struct* edf_task_list)
{
    struct edf_task_struct* entry;
    struct edf_task_struct* highest = NULL;

    mutex_lock(&edf_task_list->lock);

    list_for_each_entry(entry, &edf_task_list->head, list)
    {
	/*
        if (entry->state == READY && (!highest || highest->period > entry->period)) {
            highest = entry;
        }*/
	
	if(entry -> state == READY && (!highest || highest -> deadline > entry -> deadline)){
		highest = entry;
	}
	
    }

    mutex_unlock(&edf_task_list->lock);

    return highest;
}


void edf_task_list_delete_all(struct edf_task_list_struct* edf_task_list)
{
    struct edf_task_struct* entry;
    struct edf_task_struct* entry_next;

    mutex_lock(&edf_task_list->lock);

    list_for_each_entry_safe(entry, entry_next, &edf_task_list->head, list)
    {
        list_del(&entry->list);
        del_timer(&entry->timer);
        kmem_cache_free(edf_task_list->cache, (void*)entry);
    }
    edf_task_list->size = 0;

    kmem_cache_destroy(edf_task_list->cache);

    mutex_unlock(&edf_task_list->lock);
}


char* edf_task_list_to_str(struct edf_task_list_struct* edf_task_list)
{
    struct edf_task_struct* entry;
    size_t buffer_size = 1 + edf_task_list->size * 10;

    char* buffer;
    buffer = kmalloc(buffer_size, GFP_KERNEL);
    buffer[0] = '\0';

    mutex_lock(&edf_task_list->lock);

    list_for_each_entry(entry, &edf_task_list->head, list)
    {
        char line[10] = {0}; // temporary storage for line, initialized to 0.
        sprintf(line, "%lu\n", entry->pid);
        strcat(buffer, line);
    }

    mutex_unlock(&edf_task_list->lock);

    return buffer;
}

void edf_task_list_remove(unsigned long pid, struct edf_task_list_struct* edf_task_list)
{
    struct edf_task_struct* entry;
    struct edf_task_struct* entry_next;

    mutex_lock(&edf_task_list->lock);

    list_for_each_entry_safe(entry, entry_next, &edf_task_list->head, list)
    {
        if (entry->pid == pid)
        {
            list_del(&entry->list);
            del_timer(&entry->timer);
            kmem_cache_free(edf_task_list->cache, (void*)entry);
            edf_task_list->size -= 1;

            mutex_unlock(&edf_task_list->lock);
            return;
        }
    }

    mutex_unlock(&edf_task_list->lock);
}


