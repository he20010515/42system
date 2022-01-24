#include "bootpack.h"

void memman_init(struct MEMMAN *self)
{
    self->frees = 0;    //可用信息条数
    self->maxfrees = 0; //观察可用状况,frees的最大值
    self->lostsize = 0; //释放失败的内存大小总和
    self->losts = 0;    //释放失败次数
    return;
}

unsigned int memman_total(struct MEMMAN *self)
{
    unsigned int i, t = 0;
    for (i = 0; i < self->frees; i++)
    {
        t += self->free[i].size;
    }
    return t;
}

unsigned int memman_alloc(struct MEMMAN *self, unsigned int size)
{
    unsigned int i, addr;
    for (i = 0; i < self->frees; i++)
    {
        if (self->free[i].size >= size)
        {
            addr = self->free[i].addr;

            self->free[i].addr += size;
            self->free[i].size -= size;
            if (self->free[i].size == 0)
            {
                //如果free[i]的size变成了0,那么减掉一条可用信息
                self->frees--;
                for (; i < self->frees; i++)
                {
                    self->free[i] = self->free[i + 1]; //代入结构体
                }
            }
            return addr;
        }
    }
    return 0;
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
/* 释放 */
{
    int i, j;
    /* 为便于归纳内存，将free[]按照addr的顺序排列 */
    /* 所以，先决定应该放在哪里 */
    for (i = 0; i < man->frees; i++)
    {
        if (man->free[i].addr > addr)
        {
            break;
        }
    }
    /* free[i - 1].addr < addr < free[i].addr */
    if (i > 0)
    {
        /* 前面有可用内存 */
        if (man->free[i - 1].addr + man->free[i - 1].size == addr)
        {
            /* 可以与前面的可用内存归纳到一起 */
            man->free[i - 1].size += size;
            if (i < man->frees)
            {
                /* 后面也有 */
                if (addr + size == man->free[i].addr)
                {
                    /* 也可以与后面的可用内存归纳到一起 */
                    man->free[i - 1].size += man->free[i].size;
                    /* man->free[i]删除 */
                    /* free[i]变成0后归纳到前面去 */
                    man->frees--;
                    for (; i < man->frees; i++)
                    {
                        man->free[i] = man->free[i + 1]; /* 结构体赋值 */
                    }
                }
            }
            return 0; /* 成功完成 */
        }
    }
    /* 不能与前面的可用空间归纳到一起 */
    if (i < man->frees)
    {
        /* 后面还有 */
        if (addr + size == man->free[i].addr)
        {
            /* 可以与后面的内容归纳到一起 */
            man->free[i].addr = addr;
            man->free[i].size += size;
            return 0; /* 成功完成 */
        }
    }
    /* 既不能与前面归纳到一起，也不能与后面归纳到一起 */
    if (man->frees < MEMMAN_FREES)
    {
        /* free[i]之后的，向后移动，腾出一点可用空间 */
        for (j = man->frees; j > i; j--)
        {
            man->free[j] = man->free[j - 1];
        }
        man->frees++;
        if (man->maxfrees < man->frees)
        {
            man->maxfrees = man->frees; /* 更新最大值 */
        }
        man->free[i].addr = addr;
        man->free[i].size = size;
        return 0; /* 成功完成 */
    }
    /* 不能往后移动 */
    man->losts++;
    man->lostsize += size;
    return -1; /* 失败 */
}

unsigned int memtest(unsigned int start, unsigned int end)
{
    char flg486 = 0;
    unsigned int eflg, cr0, i;
    //确认CPU是386还是486以上的
    eflg = io_load_eflags();
    eflg |= EFLAGS_AC_BIT; //AC-bit = 1;
    io_store_eflags(eflg);
    eflg = io_load_eflags();
    if ((eflg & EFLAGS_AC_BIT) != 0)
    {
        flg486 = 1;
    }
    eflg &= ~EFLAGS_AC_BIT;
    io_store_eflags(eflg);

    if (flg486 != 0) // 禁止缓存
    {
        cr0 = load_cr0();
        cr0 |= CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }
    i = memtest_sub(start, end);
    if (flg486 != 0) // 禁止缓存
    {
        cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }
    return i;
}

unsigned memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
    size = (size + 0xfff) & 0xfffff000;
    return memman_alloc(man, size);
}

unsigned memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
    size = (size + 0xfff) & 0xfffff000;
    return memman_free(man, addr, size);
}
