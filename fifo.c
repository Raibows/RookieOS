#include "bootpack.h"

#define FLAGS_OVERRUN 0x01


void fifo8_init(struct FIFO8* fifo, int size, unsigned char* buf) {
    /*
     * 初始化fifo缓冲区
     */
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size;
    fifo->flags = 0;
    fifo->w = 0;
    fifo->r = 0;
    return;
}

int fifo8_put(struct FIFO8* fifo, unsigned char data) {
    /*
     * 向缓冲区写入数据
     */
    if (fifo->free == 0)
    {
        //溢出
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->w] = data;
    if (++fifo->w == fifo->size) fifo->w = 0;
    --fifo->free;
    return 0;
}

int fifo8_get(struct FIFO8* fifo) {
    int data;
    if (fifo->free == fifo->size) return -1;
    data = fifo->buf[fifo->r];
    if (++fifo->r == fifo->size) fifo->r = 0;
    ++fifo->free;
    return data;
}

int fifo8_status(struct FIFO8* fifo) {
    return fifo->size - fifo->free;
}


void fifo32_init(struct FIFO32* fifo, int size, int* buf, struct Task* task) {
    /*
     * 初始化fifo缓冲区
     * task为要唤醒的任务，可设置为NULL
     */
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size;
    fifo->flags = 0;
    fifo->w = 0;
    fifo->r = 0;
    fifo->task = task;
    return;
}

int fifo32_put(struct FIFO32* fifo, int data) {
    /*
     * 向缓冲区写入数据
     */
    if (fifo->free == 0)
    {
        //溢出
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->w] = data;
    if (++fifo->w == fifo->size) fifo->w = 0;
    --fifo->free;
    if (fifo->task != NULL && fifo->task->flags != 2) task_run(fifo->task, -1, 0);
    return 0;
}

int fifo32_get(struct FIFO32* fifo) {
    int data;
    if (fifo->free == fifo->size) return -1;
    data = fifo->buf[fifo->r];
    if (++fifo->r == fifo->size) fifo->r = 0;
    ++fifo->free;
    return data;
}

int fifo32_status(struct FIFO32* fifo) {
    return fifo->size - fifo->free;
}