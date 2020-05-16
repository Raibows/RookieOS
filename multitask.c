#include "bootpack.h"

struct TaskControl* taskctl;
struct Timer* task_timer;

struct Task* task_init(struct MemMan* memman) {
    struct Task* task;
    struct SEGMENT_DESCRIPTOR* gdt = (struct SEGMENT_DESCRIPTOR*) ADR_GDT;
    taskctl = (struct TaskControl*)memman_alloc_4kB(memman, sizeof(struct TaskControl));
    int i;
    for (i = 0; i < MAX_TASKS; ++i)
    {
        taskctl->pool[i].flags = 0;
        taskctl->pool[i].gdt_id = (TASK_START_GDT + i) * 8;
        set_segmdesc(gdt + TASK_START_GDT + i, 103, (int) &taskctl->pool[i].tss, AR_TSS32);
    }
    for (i = 0; i < MAX_LEVELS; ++i)
    {
        taskctl->level[i].running_num = 0;
        taskctl->level[i].now_task = 0;
    }
    task = task_alloc();
    task->flags = 2;
    task->priority = 2;
    task->level = 0;
    task_add(task);
    task_switch_level();
    load_tr(task->gdt_id);
    task_timer = timer_alloc();
    timer_settime(task_timer, task->priority);
    return task;
}

struct Task* task_alloc(void) {
    int i;
    for (i=0; i<MAX_TASKS; ++i)
    {
        if (taskctl->pool[i].flags == 0)
        {
            taskctl->pool[i].flags = 1; // allocated
            taskctl->pool[i].tss.eflags = 0x00000202; // IF = 1允许响应中断
            taskctl->pool[i].tss.eax = 0; // 这里先置为0
            taskctl->pool[i].tss.ecx = 0;
            taskctl->pool[i].tss.edx = 0;
            taskctl->pool[i].tss.ebx = 0;
            taskctl->pool[i].tss.ebp = 0;
            taskctl->pool[i].tss.esi = 0;
            taskctl->pool[i].tss.edi = 0;
            taskctl->pool[i].tss.es = 0;
            taskctl->pool[i].tss.ds = 0;
            taskctl->pool[i].tss.fs = 0;
            taskctl->pool[i].tss.gs = 0;
            taskctl->pool[i].tss.ldtr = 0;
            taskctl->pool[i].tss.iomap = 0x40000000;
            return &taskctl->pool[i];
        }
    }
    return 0;
}

void task_run(struct Task* task, int level, int priority) {
    if (level < 0) level = task->level; // 注意level是从0开始的，因此如果不想改变task的等级，需指定负数
    if (priority > 0) task->priority = priority; // if priority == 0, do not change it's priority level
    
    if (task->flags == 2 && level != task->level) // change running task's level
    {
        task_remove(task);
    }
    if (task->flags != 2)
    {
        task->level = level;
        task_add(task);
    }
    taskctl->is_lv_change = 1; // 下次任务切换时检查level
    return;
}

void task_switch(void) {
    struct TaskLevel* tl = &taskctl->level[taskctl->now_lv];
    struct Task* now_task = tl->tasks[tl->now_task];
    tl->now_task += 1;
    if (tl->now_task == tl->running_num) tl->now_task = 0;
    if (taskctl->is_lv_change != 0)
    {
        task_switch_level();
        tl = &taskctl->level[taskctl->now_lv];
    }
    struct Task* next_task = tl->tasks[tl->now_task];
    timer_settime(task_timer, next_task->priority);
    if (next_task != now_task) far_jmp(0, next_task->gdt_id);
    return;
}

void task_sleep(struct Task* task) {
    if (task->flags != 2) return;
    struct Task* now_task = task_now();
    task_remove(task);
    if (task == now_task) // 如果要休眠的任务是正在运行的任务，则需要重新切换
    {
        task_switch_level();
        now_task = task_now();
        far_jmp(0, now_task->gdt_id);
    }
    return;
}

struct Task* task_now(void) {
    return taskctl->level[taskctl->now_lv].tasks[taskctl->level[taskctl->now_lv].now_task];
}

void task_add(struct Task* task) {
    struct TaskLevel* tl = &taskctl->level[task->level];
    if (tl->running_num < MAX_LEVEL_TASKS)
    {
        tl->tasks[tl->running_num++] = task;
        task->flags = 2; // running
    }
    return;
}

void task_remove(struct Task* task) {
    /*
     * 将task从对应的level中的tasks删除
     * 注意在真正存储的taskctl.pool中仍然保留
     * 此时 task.flags = 1
     */
    struct TaskLevel* tl = &taskctl->level[task->level];
    int i;
    for (i=0; i<tl->running_num; ++i)
    {
        if (tl->tasks[i] == task) break;
    }
    tl->running_num -= 1;
    if (i < tl->running_num) --tl->now_task;
    if (tl->now_task >= tl->running_num) tl->now_task = 0;
    task->flags = 1; // allocated or sleeping
    for (; i < tl->running_num; ++i) tl->tasks[i] = tl->tasks[i+1];
    return;
}

void task_switch_level(void) {
    int i;
    for (i = 0; i < MAX_LEVELS; ++i)
    {
        if (taskctl->level[i].running_num > 0) break;
    }
    taskctl->now_lv = i;
    taskctl->is_lv_change = 0;
    return;
}














