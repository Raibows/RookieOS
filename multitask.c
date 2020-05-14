#include "bootpack.h"

struct Timer* mt_timer;
int mt_tr;

void mt_init(void) {
    mt_timer = timer_alloc(); //此处没有必要写入fifo
    timer_settime(mt_timer, 0.02 * TIMER_COUNT_PER_SECOND);
    mt_tr = 3 * 8;
    return;
}

void mt_task_switch(void) {
    if (mt_tr == 3 * 8) mt_tr = 4 * 8;
    else mt_tr = 3 * 8;
    timer_settime(mt_timer, 0.02 * TIMER_COUNT_PER_SECOND);
    far_jmp(0, mt_tr);
    return;
}









