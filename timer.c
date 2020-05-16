#include "bootpack.h"


#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040
#define TIMER_FLAGS_FREE 0 //未使用
#define TIMER_FLAGS_ALLOC 1 //定时器已配置
#define TIMER_FLAGS_USING 2 //定时器运行中

struct TimerControl timerctl;
extern struct Timer* task_timer;

void int_handler20(int* esp) {
    io_out8(PIC0_OCW2, 0x60); // 把irq-00接收的信号通知给PIC
    ++timerctl.count;
    if (timerctl.count < timerctl.next_expire_time) return; //还没有到下一个timer时刻
    struct Timer* temp = timerctl.guard_timer->next;
    char ts_flag = 0;
    while (temp != NULL)
    {
        if (temp->timeout > timerctl.count) break;
        if (temp != task_timer)
        {
            temp->flags = TIMER_FLAGS_ALLOC;
            fifo32_put(temp->fifo, temp->data);
        }
        else ts_flag = 1;
        temp = temp->next;
    }
    timerctl.guard_timer->next = temp;
    if (temp == NULL) timerctl.next_expire_time = timerctl.guard_timer->timeout;
    else timerctl.next_expire_time = temp->timeout;
    if (ts_flag == 1) task_switch();
    return;
}

void init_pit(void) {
    /*
     * programmable interval timer
     * 2e9c = 11932，大约每秒100次计时器中断
     */
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);
    io_out8(PIT_CNT0, 0x2e);
    timerctl.count = 0;
    timerctl.next_expire_time = 0xffffffff;
    int i;
    for (i=0; i < MAX_TIMER; ++i) timerctl.timers[i].flags = TIMER_FLAGS_FREE;
    timerctl.guard_timer = timer_alloc();
    timerctl.guard_timer->flags = TIMER_FLAGS_USING;
    timerctl.guard_timer->timeout = 0xffffffff;
    return;
}

struct Timer* timer_alloc(void) {
    int i;
    for (i=0; i < MAX_TIMER; ++i)
    {
        if (timerctl.timers[i].flags == TIMER_FLAGS_FREE)
        {
            timerctl.timers[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timers[i];
        }
    }
    return 0; //failed
}

void timer_free(struct Timer* timer) {
    /*
     * 只有处于allocated的timer才可以被free
     * 想象一下假如你的timer还没有timeout，就free掉，显然会矛盾
     * 如果处于还没expire的状态，那么必须从guard那里遍历寻找，然后强制free【未实现】
     */
    if (timer->flags == TIMER_FLAGS_ALLOC) timer->flags = TIMER_FLAGS_FREE;
    return;
}

void timer_init(struct Timer* timer, struct FIFO32* fifo, int data) {
    timer->data = data;
    timer->fifo = fifo;
    return;
}

void timer_settime(struct Timer* timer, unsigned int timeout) {
    int e;
    timeout = timeout > 0 ? timeout : 1;
    timer->flags = TIMER_FLAGS_USING;
    timer->timeout = timeout + timerctl.count;
    e = io_load_eflags();
    io_cli();
    struct Timer* temp = timerctl.guard_timer;
    while (temp->next != NULL)
    {
        if (temp->next->timeout > timer->timeout) break;
        temp = temp->next;
    }
    timer->next = temp->next;
    temp->next = timer;
    timerctl.next_expire_time = timerctl.guard_timer->next->timeout;
    io_store_eflags(e);
    return;
}




















