#include "bootpack.h"




unsigned int memtest(unsigned int start, unsigned int end) {
    char flag486 = 0; //only >= 486 use cpu cache
    unsigned int eflag, cr0, i;
    eflag = io_load_eflags();
    if ((eflag & EFLAGS_AC_BIT) != 0) flag486 = 1; // 如果是386，即使设定AC=1，AC的值还会自动回到0
    eflag &= ~EFLAGS_AC_BIT; // AC-bit = 0
    io_store_eflags(eflag);
    
    if (flag486 != 0) //486
    {
        cr0 = load_cr0();
        cr0 |= CR0_CACHE_DISABLE; /* 禁止缓存 */
        store_cr0(cr0);
    }
    
    i = memtest_sub(start, end);
    
    if (flag486 != 0) {
        cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE; /* 允许缓存 */
        store_cr0(cr0);
    }
    
    return i;
}


void memman_init(struct MemMan* man) {
    man->frees = 0;
    man->maxfrees = 0;
    man->losts = 0;
    man->lostsize = 0;
    return;
}

unsigned int memman_total(struct MemMan* man) {
    /*
     * 返回可用内存大小
     */
    unsigned int i = 0, t=0;
    for (; i<man->frees; ++i) t += man->pool[i].size;
    return t;
}

unsigned int memman_alloc(struct MemMan* man, unsigned int size) {
    unsigned int i, a;
    for (i=0; i<man->frees; ++i)
    {
        if (man->pool[i].size >= size)
        {
            a = man->pool[i].addr;
            man->pool[i].addr += size;
            man->pool[i].size -= size;
            if (man->pool[i].size == 0)
            {
                --man->frees;
                for (; i<man->frees; ++i) man->pool[i] = man->pool[i+1];
            }
            return a;
        }
    }
    return 0;
}

int memman_free(struct MemMan* man, unsigned int addr, unsigned int size) {
    int i, j;
    for (i=0; i<man->frees; ++i)
    {
        if (man->pool[i].addr > addr) break;
    }
    // pool[i-1].addr < addr < pool[i].addr
    if (i > 0) // 前面有i-1
    {
        if (man->pool[i-1].addr + man->pool[i-1].size == addr)
        {
            man->pool[i-1].size += size;
            if (i < man->frees) // 后面也有i
            {
                if (addr + size == man->pool[i].size)
                {
                    man->pool[i-1].size += man->pool[i].size;
                    --man->frees;
                    for (; i<man->frees; ++i) man->pool[i] = man->pool[i+1];
                }
            }
            return 0; // success
        }
    }
    if (i < man->frees) // 不能与前面的空间连接
    {
        if (addr + size == man->pool[i].addr)
        {
            man->pool[i].addr = addr;
            man->pool[i].size += size;
            return 0;
        }
    }
    if (i < MEMMAN_FREES) // 既不能与前面，也不能与后面的连接，腾地方插入
    {
        for (j=man->frees; j>i; --j) man->pool[j] = man->pool[j-1];
        ++man->frees;
        man->maxfrees = man->maxfrees > man->frees ? man->maxfrees : man->frees;
        man->pool[i].addr = addr;
        man->pool[i].size = size;
        return 0;
    }
    
    // 也不能往后移动，说明碎片化严重，只能放弃了
    ++man->losts;
    man->lostsize += size;
    return -1; // failed
}

unsigned int memman_alloc_4kB(struct MemMan* man, unsigned int size) {
    // 以4kB为最小单位向上舍入，防止碎片化严重
    size = (size + 0xfff) & 0xfffff000;
    return memman_alloc(man, size);
}

int memman_free_4kB(struct MemMan* man, unsigned int addr, unsigned int size) {
    size = (size + 0xfff) & 0xfffff000; //我觉得有问题，原始内存小，强行补齐（变大），很可能越界
//    size &= 0xfffff000; //应该是向下取整
// 因为你allocate的时候默认是4kB补齐的，但是使用者默认以为size还是那么大，当他想要释放的时候，你必须同样补齐size
// 补齐后的大小才是真正的，否则内存泄漏
    return memman_free(man, addr, size);
}







