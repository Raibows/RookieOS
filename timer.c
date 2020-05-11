#include "bootpack.h"


#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040
#define TIMER_FLAGS_FREE 0 //未使用
#define TIMER_FLAGS_ALLOC 1 //定时器已配置
#define TIMER_FLAGS_USING 2 //定时器运行中

struct TimerControl timerctl;

void int_handler20(int* esp) {
    io_out8(PIC0_OCW2, 0x60); // 把irq-00接收的信号通知给PIC
    ++timerctl.count;
    if (timerctl.count < timerctl.next) return; //还没有到下一个timer时刻
    int i;
    timerctl.next = 0xffffffff;
    for (i=0; i<timerctl.using_num; ++i)
    {
        if (timerctl.timer_order[i]->timeout > timerctl.count) break;
        timerctl.timer_order[i]->flags = TIMER_FLAGS_ALLOC;
        fifo8_put(timerctl.timer_order[i]->fifo, timerctl.timer_order[i]->data);
    }
    timerctl.using_num -= i;
    int j;
    for (j=0; j<timerctl.using_num; ++j) timerctl.timer_order[j] = timerctl.timer_order[j+i];
    if (timerctl.using_num > 0) timerctl.next = timerctl.timer_order[0]->timeout;
    else timerctl.next = 0xffffffff;
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
    timerctl.next = 0xffffffff;
    timerctl.using_num = 0;
    int i;
    for (i=0; i < MAX_TIMER; ++i) timerctl.timers[i].flags = TIMER_FLAGS_FREE;
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
    timer->flags = TIMER_FLAGS_FREE;
    return;
}

void timer_init(struct Timer* timer, struct FIFO8* fifo, unsigned char data) {
    timer->data = data;
    timer->fifo = fifo;
    return;
}

void timer_settime(struct Timer* timer, unsigned int timeout) {
    int e, i, j;
    timer->flags = TIMER_FLAGS_USING;
    timer->timeout = timeout + timerctl.count;
    e = io_load_eflags();
    io_cli();
    for (i=0; i<timerctl.using_num; ++i)
    {
        if (timerctl.timer_order[i]->timeout > timer->timeout) break;
    }
    for (j=timerctl.using_num; j > i; --j) timerctl.timer_order[j] = timerctl.timer_order[j-1];
    ++timerctl.using_num;
    timerctl.timer_order[i] = timer;
    timerctl.next = timerctl.timer_order[0]->timeout;
    io_store_eflags(e);
    return;
}




















