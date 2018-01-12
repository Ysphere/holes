#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * We use double linked list to store holds
 * start is the start address of the hold
 * end is the end address of the hold
 **/
typedef struct Hold {
    int start;
    int end;
    struct Hold *prev;
    struct Hold *next;
} Hold;

typedef struct HoldList {
    struct Hold *first;
    struct Hold *last;

    struct Hold *next; /* used in next fit */
    int size;
} HoldList;

/**
 * Store the process information
 * Using double linked list to store it
 */
typedef struct Process {
    char id;
    int size;
    int times;
    Hold *hold;
    struct Process *prev;
    struct Process *next;
} Process;

typedef struct ProcessQueue {
    Process *first;
    Process *last;
    int size;
} ProcessQueue;

/**
 * Store the variables
 */
typedef struct Memory {
    HoldList hold_list;
    ProcessQueue in_queue;
    ProcessQueue wait_queue;

    int usage;
    double cumulative_total;
    int holds_total;
    int procs_total;
    int times;
} Memory;

/* read process in the file and return the memory */
Memory* init_memory(char *fn);

/* free the resource */
void destory_memory(Memory *mem);

/* push a hold to holdlist, need do merge */
void push_hold(HoldList *list, Hold *hold);
/* pop a hold from the hold */
Hold* pop_hold(HoldList *list, Hold *hold, int size);

/* push a process in a queue */
void push_process(ProcessQueue *queue, Process *proc);
/* pop a process from queue */
Process *pop_process(ProcessQueue *queue);

/* swap out the first process in the memory to wait queue */
void swap_out(Memory *mem);

/* for debug */
void debug(Memory *mem);


/* do the four fits */
void do_fit(char *fn, int type);
Hold *do_best_fit(Memory *mem, Process *proc);
Hold *do_worst_fit(Memory *mem, Process *proc);
Hold *do_next_fit(Memory *mem, Process *proc);
Hold *do_first_fit(Memory *mem, Process *proc);

int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input.txt>\n", argv[0]);
        exit(1);
    }

    printf("Start best fit ...\n");
    do_fit(argv[1], 0);

    printf("\nStart worst fit ...\n");
    do_fit(argv[1], 1);

    printf("\nStart next fit ...\n");
    do_fit(argv[1], 2);

    printf("\nStart first fit ...\n");
    do_fit(argv[1], 3);

	return 0;
}

Memory* init_memory(char *fn) {
    FILE *fp = fopen(fn, "rb");
    if (fp == NULL) {
        perror("fopen");
        exit(1);
    }

    Memory *mem = calloc(1, sizeof(Memory));
    char id[2];
    int size;
    Process *proc = NULL;
    while (fscanf(fp, "%s%d", id, &size) == 2) {
        /* read processes from the file */
        proc = calloc(1, sizeof(Process));
        proc->id = id[0];
        proc->size = size;
        push_process(&mem->wait_queue, proc);
    }
    fclose(fp);

    /* init the origin hold with 128MB */
    Hold *hold = calloc(1, sizeof(Hold));
    hold->start = 0;
    hold->end = 127;
    push_hold(&mem->hold_list, hold);
    return mem;
}

void destory_memory(Memory *mem) {
    Hold *hold = NULL, *hold_tmp = NULL;
    Process *proc = NULL, *proc_tmp = NULL;
    
    /* free the holds */
    hold = mem->hold_list.first;
    while (hold) {
        hold_tmp = hold;
        hold = hold->next;
        free(hold_tmp);
    }

    /* free the in processes */
    proc = mem->in_queue.first;
    while (proc) {
        proc_tmp = proc;
        proc = proc->next;
        free(proc_tmp->hold);
        free(proc_tmp);
    }

    /* free the wait processes */
    proc = mem->wait_queue.first;
    while (proc) {
        proc_tmp = proc;
        proc = proc->next;
        free(proc_tmp);
    }
    free(mem);
}

void push_process(ProcessQueue *queue, Process *proc) {
    if (queue->last == NULL) {
        proc->prev = proc->next = NULL;
        queue->first = queue->last = proc;
    } else {
        proc->prev = queue->last;
        proc->next = NULL;
        queue->last->next = proc;
        queue->last = proc;
    }
    queue->size++;
}

Process *pop_process(ProcessQueue *queue) {
    if (queue->first == NULL) {
        return NULL;
    }
    Process *proc = queue->first;
    queue->first = queue->first->next;
    if (queue->first == NULL) {
        queue->last = NULL;
    }
    queue->size--;
    proc->prev = proc->next = NULL;
    return proc;
}

void push_hold(HoldList *list, Hold *hold) {
    if (list->first == NULL) {
        /* just insert it */
        list->first = list->last = hold;
        list->next = hold;
        list->size = 1;
        return;
    }
    
    if (hold->end == list->first->start - 1) {
        /* just merge the hold to the list->first */
        list->first->start = hold->start;
        free(hold);
        return;
    }

    if (hold->end < list->first->start - 1) {
        /* insert at first */
        hold->prev = NULL;
        hold->next = list->first;
        list->first = hold;
        list->size++;
        return;
    }

    /* search the parent */
    Hold *parent = list->first;
    while (parent->next && parent->next->end < hold->start) {
        parent = parent->next;
    }

    /* check if need merge with parent */    
    if (parent->end == hold->start - 1) {
        /* merge to the parent */
        parent->end = hold->end;
        free(hold);
        /* then merge if parent should merge with parent->next */
        if (parent->next && parent->end == parent->next->start - 1) {
            hold = parent->next;
            parent->end = hold->end;
            parent->next = hold->next;
            if (list->next == hold) {
                list->next = parent;
            }
            free(hold);
            if (parent->next) {
                /* update the parent->next->prev */
                parent->next->prev = parent;
            } else {
                /* the parent is the last */
                list->last = parent;
            }
            list->size--;
        }
        return;
    }

    /* check if need merge with parent->next */
    if (parent->next && hold->end == parent->next->start - 1) {
        parent->next->start = hold->start;
        free(hold);
        return;
    }

    /* insert hold after parent */
    hold->prev = parent;
    hold->next = parent->next;
    if (parent->next) {
        parent->next->prev = hold;
    }
    parent->next = hold;
    list->size++;
}

Hold* pop_hold(HoldList *list, Hold *hold, int size) {
    Hold *tmp = NULL;
    if (hold->end - hold->start + 1 > size) {
        /* just create a new hold to return */
        tmp = calloc(1, sizeof(Hold));
        tmp->start = hold->start;
        tmp->end = tmp->start + size - 1;
        hold->start = tmp->end + 1;
        return tmp;
    }

    /* need delete the hold from list */
    if (hold->prev == NULL) {
        list->first = list->first->next;
        if (list->first == NULL) {
            list->last = NULL;
        }
    } else {
        hold->prev->next = hold->next;
    }
    if (hold->next == NULL) {
        list->last = list->last->prev;
        if (list->last == NULL) {
            list->first = NULL;
        }
    }
    if (hold == list->next) {
        list->next = list->next->next;
    }
    return hold;
}

void swap_out(Memory *mem) {
    /* first pop the process from in_queue */
    Process *proc = pop_process(&mem->in_queue);
    if (proc == NULL) {
        fprintf(stderr, "no process in memory to swap\n");
        exit(1);
    }
    push_hold(&mem->hold_list, proc->hold);
    proc->hold = NULL;
    proc->times++;

    mem->usage -= proc->size;

    if (proc->times < 3) {
        /* */
        push_process(&mem->wait_queue, proc);
    } else {
        /* end the process*/
        free(proc);
    }
}


void do_fit(char *fn, int type) {
    Memory *mem = init_memory(fn);
    
    Process *proc = NULL;
    Hold *hold = NULL;

    /* debug(mem); */

    double memusage = 0.0;
    while ((proc = pop_process(&mem->wait_queue)) != NULL) {
        /* debug(mem); */
        while (1) {
            if (type == 0) {
                hold = do_best_fit(mem, proc);
            } else if (type == 1) {
                hold = do_worst_fit(mem, proc);
            } else if (type == 2) {
                hold = do_next_fit(mem, proc);
            } else {
                hold = do_first_fit(mem, proc);
            }
            if (hold != NULL) {
                break;
            }
            /* can't found, swap out */
            swap_out(mem);
        }
        proc->hold = pop_hold(&mem->hold_list, hold, proc->size);
        push_process(&mem->in_queue, proc);
        mem->usage += proc->size;

        
        mem->times ++;
        memusage = mem->usage / 128.0 * 100;
        mem->cumulative_total += memusage;
        mem->holds_total += mem->hold_list.size;
        mem->procs_total += mem->in_queue.size;

        printf("%c loaded, #processes = %d, #holes = %d, %%memusage = %.2lf, cumulative %%mem = %.2lf\n",
                proc->id, mem->in_queue.size, mem->hold_list.size, memusage, mem->cumulative_total/mem->times);

    }
    printf("Total loads = %d, average #processes = %.2lf, average #holes = %.2lf, cumulative %mem = %.2lf\n",
            mem->times, (double)(mem->procs_total)/mem->times, (double)(mem->holds_total)/mem->times,
             mem->cumulative_total/mem->times);

    destory_memory(mem);
}

Hold *do_best_fit(Memory *mem, Process *proc) {
    Hold *fit = NULL;
    Hold *hold = mem->hold_list.first;
    while (hold) {
        if (hold->end - hold->start + 1 >= proc->size) {
            if (fit == NULL || hold->end - hold->start < fit->end - fit->start) {
                fit = hold;
            }
        }
        hold = hold->next;
    }
    return fit;
}

Hold *do_worst_fit(Memory *mem, Process *proc) {
    Hold *fit = NULL;
    Hold *hold = mem->hold_list.first;
    while (hold) {
        if (hold->end - hold->start + 1 >= proc->size) {
            if (fit == NULL || hold->end - hold->start > fit->end - fit->start) {
                fit = hold;
            }
        }
        hold = hold->next;
    }
    return fit;
}

Hold *do_next_fit(Memory *mem, Process *proc) {
    int i = 0;
    for (i = 0; i < mem->hold_list.size; i++) {
        if (mem->hold_list.next->end - mem->hold_list.next->start + 1 >= proc->size) {
            return mem->hold_list.next;
        }
        mem->hold_list.next = mem->hold_list.next->next;
        if (mem->hold_list.next == NULL) {
            mem->hold_list.next = mem->hold_list.first;
        }
    }
    return NULL;
}

Hold *do_first_fit(Memory *mem, Process *proc) {
    Hold *hold = mem->hold_list.first;
    while (hold) {
        if (hold->end - hold->start + 1 >= proc->size) {
            return hold;
        }
        hold = hold->next;
    }
    return NULL;
}

/* for debug */
void debug(Memory *mem) {
    printf("\nDEBUG *****\n");
    printf("Holds: %d\n", mem->hold_list.size);
    Hold *hold = mem->hold_list.first;
    while (hold) {
        printf("start=%d end=%d\n", hold->start, hold->end);
        hold = hold->next;
    }
    printf("\nIn queues: %d\n", mem->in_queue.size);
    Process *proc = mem->in_queue.first;
    while (proc) {
        printf("id=%c size=%d start=%d end=%d\n", proc->id, proc->size,
               proc->hold->start, proc->hold->end);
        proc = proc->next;
    }
    printf("\nWait queues: %d\n", mem->wait_queue.size);
    proc = mem->wait_queue.first;
    while (proc) {
        printf("id=%c size=%d\n", proc->id, proc->size);
        proc = proc->next;
    }
    printf("DEBUG *****\n\n");
}
