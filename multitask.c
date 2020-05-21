#include "bootpack.h"

struct TaskControl* taskctl;
struct Timer* task_timer;


struct Task* task_init(struct MemMan* memman) {
    struct Task* task;
    struct Task* idle;
    struct SEGMENT_DESCRIPTOR* gdt = (struct SEGMENT_DESCRIPTOR*) ADR_GDT;
    taskctl = (struct TaskControl*) memman_alloc_4kB(memman, sizeof(struct TaskControl));
    int i;
    for (i = 0; i < MAX_TASKS; ++i)
    {
        // gdt [3, 1002]
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
    
    idle = task_alloc();
    idle->stack = memman_alloc_4kB(memman, 64 * 1024);
    idle->tss.esp = idle->stack + 64 * 1024;
    idle->tss.eip = (int) &task_idle;
    idle->tss.es = 1 * 8;
    idle->tss.cs = 2 * 8;
    idle->tss.ss = 1 * 8;
    idle->tss.ds = 1 * 8;
    idle->tss.fs = 1 * 8;
    idle->tss.gs = 1 * 8;
    task_run(idle, MAX_LEVELS - 1, 1);
    
    return task;
}

void task_ready(struct Task* task) {
    task->flags = 1; // allocated
    task->tss.eflags = 0x00000202; // IF = 1允许响应中断
    task->tss.eax = 0; // 这里先置为0
    task->tss.ecx = 0;
    task->tss.edx = 0;
    task->tss.ebx = 0;
    task->tss.ebp = 0;
    task->tss.esi = 0;
    task->tss.edi = 0;
    task->tss.es = 0;
    task->tss.ds = 0;
    task->tss.fs = 0;
    task->tss.gs = 0;
    task->tss.ldtr = 0;
    task->tss.iomap = 0x40000000;
    task->fifo.task = NULL;
    task->fifo.buf = NULL;
    task->fifo.size = 0;
    task->fifo.free = 0;
    task->fifo.r = 0;
    task->fifo.w = 0;
    return;
}

struct Task* task_alloc(void) {
    int i;
    for (i=0; i<MAX_TASKS; ++i)
    {
        if (taskctl->pool[i].flags == 0)
        {
            task_ready(&taskctl->pool[i]);
            return &taskctl->pool[i];
        }
    }
    return 0;
}

void task_run(struct Task* task, int level, int priority) {
    assert(task->flags != 0, "free task tried to run");
    if (task->flags == 0) return; // 已经free的不能再被运行
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
    struct TaskLevel* tl = &taskctl->level[taskctl->now_lv];
    return tl->tasks[tl->now_task];
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
    if (i < tl->now_task) --tl->now_task;
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

void task_idle(void) {
    /*
     * 闲置任务，确保level中没有一个running时不出bug
     * 作为哨兵
     */
    while (1) io_hlt();
}

void task_free(struct Task* task) {
    /*
     * 释放一个任务，并不保证task中的相关资源被释放
     * 如fifo等等
     * 请确保没有额外的fifo绑定了task，否则将会被唤醒（导致出错）
     */
    task_remove(task);
    task_ready(task);
    task->flags = 0;
    return;
}











