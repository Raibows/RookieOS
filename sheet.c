#include "bootpack.h"


struct SheetControl* sheetcontroll_init(struct MemMan* man, unsigned char* vram, int xsize, int ysize) {
    struct SheetControl* ctl;
    int i;
    ctl = (struct SheetControl*) memman_alloc_4kB(man, sizeof(struct SheetControl));
    if (ctl == 0) return ctl;
    ctl->map = (unsigned char*) memman_alloc_4kB(man, xsize * ysize);
    if (ctl->map == 0) return memman_free_4kB(man, (unsigned int)ctl, sizeof(*ctl));
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1; // 一个sheet都没有
    for (i=0; i<MAX_SHEETS; ++i)
    {
        ctl->sheets_pool[i].flags = 0; //标记为未使用
        ctl->sheets_pool[i].ctl = ctl;
    }
    return ctl;
}

struct Sheet* sheet_alloc(struct SheetControl* ctl) {
    int i;
    for (i=0; i<MAX_SHEETS; ++i)
    {
        if (ctl->sheets_pool[i].flags == 0)
        {
            ctl->sheets_pool[i].flags = SHEET_USE;
            ctl->sheets_pool[i].height = -1; //隐藏
            return &ctl->sheets_pool[i];
        }
    }
    return 0; //sheet分配失败，全部被使用了
}

void sheet_setbuf(struct Sheet* sht, unsigned char* buf, int xsize, int ysize, int col_inv) {
    sht->buf = buf;
    sht->bxsize = xsize;
    sht->bysize = ysize;
    sht->col_inv = col_inv;
    return;
}

void sheet_updown(struct Sheet* sht, int height) {
    /*
     * 改变图层的高度，堆叠状态
     */
    struct SheetControl* ctl = sht->ctl;
    int h, old = sht->height; // old 存储该图层当前的高度
    height = height > (ctl->top + 1) ? ctl->top + 1 : height;
    height = height < -1 ? -1 : height;
    sht->height = height; //调整新高度
    
    if (old > height) //比以前低
    {
        if (height >= 0) //那就把介于height和old之间的向前移动
        {
            for (h=old; h>height; --h)
            {
                ctl->sheets[h] = ctl->sheets[h-1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
            sheet_refreshmap(ctl, sht->vx, sht->vy, sht->vx+sht->bxsize, sht->vy+sht->bysize, sht->height + 1);
            sheet_refreshsub(ctl, sht->vx, sht->vy, sht->vx+sht->bxsize, sht->vy+sht->bysize, sht->height + 1, old);
        }
        else //隐藏
        {
            if (ctl->top > old)
            {
                for (h=old; h<ctl->top; ++h)
                {
                    ctl->sheets[h] = ctl->sheets[h+1];
                    ctl->sheets[h]->height = h;
                }
            }
            --ctl->top;
            sheet_refreshmap(ctl, sht->vx, sht->vy, sht->vx+sht->bxsize, sht->vy+sht->bysize, 0);
            sheet_refreshsub(ctl, sht->vx, sht->vy, sht->vx+sht->bxsize, sht->vy+sht->bysize, 0, old - 1);
        }
    }
    else if (old < height) //比以前高
    {
        if (old >= 0) //将介于old和height的往前移动
        {
            for (h=old; h<height; ++h)
            {
                ctl->sheets[h] = ctl->sheets[h+1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        }
        else // 原来是隐藏的，要插入
        {
            ++ctl->top;
            for (h=ctl->top; h>height; --h) //先腾出来地方
            {
                ctl->sheets[h] = ctl->sheets[h-1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        }
        sheet_refreshmap(ctl, sht->vx, sht->vy, sht->vx+sht->bxsize, sht->vy+sht->bysize, sht->height);
        sheet_refreshsub(ctl, sht->vx, sht->vy, sht->vx+sht->bxsize, sht->vy+sht->bysize, sht->height, sht->height);
    }
    
    return;
}

void sheet_refreshall(struct SheetControl* ctl) {
    int h, bx, by, vx, vy;
    unsigned char c;
    for (h=0; h <= ctl->top; ++h)
    {
        for (by=0; by < ctl->sheets[h]->bysize; ++by)
        {
            vy = ctl->sheets[h]->vy + by;
            for (bx=0; bx < ctl->sheets[h]->bxsize; ++bx)
            {
                vx = ctl->sheets[h]->vx + bx;
                c = ctl->sheets[h]->buf[by * ctl->sheets[h]->bxsize + bx]; //buf是绘制图形的内容
                if (c != ctl->sheets[h]->col_inv) ctl->vram[vy * ctl->xsize + vx] = c; //将非透明的元素进行绘制
            }
        }
    }
    return;
}

void sheet_refreshsub(struct SheetControl* ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1) {
    //仅对大于h0高度的窗口进行刷新
    int h, bx, by, vx, vy;
    int bx0, bx1, by0, by1;
    unsigned char sid, c;
    if (vx0 < 0) vx0 = 0;
    if (vy0 < 0) vy0 = 0;
    if (vx1 > ctl->xsize) vx1 = ctl->xsize;
    if (vy1 > ctl->ysize) vy1 = ctl->ysize;
    for (h=h0; h <= h1; ++h)
    {
        sid = ctl->sheets[h] - ctl->sheets_pool;
        // vx = sht.vx + bx -> bx = vx - sht.vx
        bx0 = vx0 - ctl->sheets[h]->vx;
        bx1 = vx1 - ctl->sheets[h]->vx;
        by0 = vy0 - ctl->sheets[h]->vy;
        by1 = vy1 - ctl->sheets[h]->vy;
        if (bx0 < 0) bx0 = 0;
        if (by0 < 0) by0 = 0;
        if (bx1 > ctl->sheets[h]->bxsize) bx1 = ctl->sheets[h]->bxsize;
        if (by1 > ctl->sheets[h]->bysize) by1 = ctl->sheets[h]->bysize;
        for (by=by0; by<by1; ++by)
        {
            vy = ctl->sheets[h]->vy + by;
            for (bx=bx0; bx<bx1; ++bx)
            {
                vx = ctl->sheets[h]->vx + bx;
                if (ctl->map[vy * ctl->xsize + vx] == sid) //map中记录的该像素的主人是该图层sheet
                {
                    c = ctl->sheets[h]->buf[by * ctl->sheets[h]->bxsize + bx];
                    ctl->vram[vy * ctl->xsize + vx] = c;
                }
            }
        }
    }
    return;
}

void sheet_slide(struct Sheet* sht, int vx0, int vy0) {
    /*
     * 不改变图层高度，仅上下左右移动
     */
    int old_vx = sht->vx, old_vy = sht->vy;
    sht->vx = vx0;
    sht->vy = vy0;
    if (sht->height >= 0) //如果非隐藏状态，则刷新改变
    {
        sheet_refreshmap(sht->ctl, old_vx, old_vy, old_vx+sht->bxsize, old_vy+sht->bysize, 0);
        sheet_refreshmap(sht->ctl, vx0, vy0, vx0+sht->bxsize, vy0+sht->bysize, sht->height);
        //原位置的图像移走后，漏出来的图像进行刷新
        sheet_refreshsub(sht->ctl, old_vx, old_vy, old_vx+sht->bxsize, old_vy+sht->bysize, 0, sht->height-1);
        //新位置的图像移过去后，重绘该层图像即可
        sheet_refreshsub(sht->ctl, vx0, vy0, vx0+sht->bxsize, vy0+sht->bysize, sht->height, sht->height); //新位置的图像进行刷新
    }
    return;
}

void sheet_free(struct Sheet* sht) {
    /*
     * 释放已使用的图层的内存
     */
    if (sht->height >= 0) sheet_updown(sht, -1); //先将其隐藏
    sht->flags = 0; //标记未使用
    return;
}

void sheet_refresh(struct Sheet* sht, int bx0, int by0, int bx1, int by1) {
    if (sht->height >= 0) //not hiding
    {
        sheet_refreshsub(sht->ctl, bx0+sht->vx, by0+sht->vy, bx1+sht->vx, by1+sht->vy, sht->height, sht->height);
    }
    return;
}

void sheet_refreshmap(struct SheetControl* ctl, int vx0, int vy0, int vx1, int vy1, int h0) {
    int h, bx, by, vx, vy, bx0, by0, bx1, by1;
    unsigned char sid;
    
    if (vx0 < 0) vx0 = 0;
    if (vy0 < 0) vy0 = 0;
    if (vx1 > ctl->xsize) vx1 = ctl->xsize;
    if (vy1 > ctl->ysize) vy1 = ctl->ysize;
    
    for (h = h0; h <= ctl->top; h++) {
        sid = ctl->sheets[h] - ctl->sheets_pool; /* 将进行了减法计算的地址作为图层号码使用 */
        bx0 = vx0 - ctl->sheets[h]->vx;
        by0 = vy0 - ctl->sheets[h]->vy;
        bx1 = vx1 - ctl->sheets[h]->vx;
        by1 = vy1 - ctl->sheets[h]->vy;
        if (bx0 < 0) bx0 = 0;
        if (by0 < 0) by0 = 0;
        if (bx1 > ctl->sheets[h]->bxsize) bx1 = ctl->sheets[h]->bxsize;
        if (by1 > ctl->sheets[h]->bysize) by1 = ctl->sheets[h]->bysize;
        for (by = by0; by < by1; by++)
        {
            vy = ctl->sheets[h]->vy + by;
            for (bx = bx0; bx < bx1; bx++)
            {
                vx = ctl->sheets[h]->vx + bx;
                if (ctl->sheets[h]->buf[by * ctl->sheets[h]->bxsize + bx] != ctl->sheets[h]->col_inv)
                {
                    ctl->map[vy * ctl->xsize + vx] = sid;
                }
            }
        }
    }
    return;
}



















