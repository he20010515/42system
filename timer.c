#include "bootpack.h"
#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040
struct TIMERCTL timerctl;

void init_pit(void)
{
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);
    io_out8(PIT_CNT0, 0x2e);
    timerctl.count = 0;
    int i;
    timerctl.next = 0xffffffff;
    timerctl.using = 0;
    for (i = 0; i < MAX_TIMER; i++)
    {
        timerctl.timers0[i].flags = 0; //未使用
    }
    return;
}

struct TIMER *timer_alloc(void)
{
    int i;
    for (i = 0; i < MAX_TIMER; i++)
    {
        if (timerctl.timers0[i].flags == 0)
        {
            timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timers0[i];
        }
    }
    return 0;
}

void timer_free(struct TIMER *timer)
{
    timer->flags = 0;
    return;
}

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, unsigned char data)
{
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout)
{
    int e, i, j;
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    e = io_load_eflags();
    io_cli();
    //搜索注册位置
    for (i = 0; i < timerctl.using; i++)
    {
        if (timerctl.timers[i]->timeout >= timer->timeout)
        {
            break;
        }
    }
    //i号之后全部后移一位
    for (j = timerctl.using; j > i; j--)
    {
        timerctl.timers[j] = timerctl.timers[j - 1];
    }
    timerctl.using ++;
    timerctl.timers[i] = timer;
    timerctl.next = timerctl.timers[0]->timeout;
    io_store_eflags(e);
    return;
}

void inthandler20(int *esp)
{
    io_out8(PIC0_OCW2, 0x60);
    timerctl.count++;
    if (timerctl.next > timerctl.count)
    {
        return; // 下一个时刻没有定时器要时间到,那么直接返回什么都不做
    }
    timerctl.next = 0xffffffff;
    int i, j;
    for (i = 0; i < timerctl.using; i++)
    {
        /* timers的定时器都处于动作中，所以不确认flags */
        if (timerctl.timers[i]->timeout > timerctl.count)
        {
            break;
        }
        //超时
        timerctl.timers[i]->flags = TIMER_FLAGS_ALLOC;
        fifo32_put(timerctl.timers[i]->fifo, timerctl.timers[i]->data);
    }
    //正好有i个定时器超时了,其余的进行移位
    timerctl.using -= i;
    for (j = 0; j < timerctl.using; j++)
    {
        timerctl.timers[j] = timerctl.timers[i + j];
    }
    if (timerctl.using > 0)
    {
        timerctl.next = timerctl.timers[0]->timeout;
    }
    else
    {
        timerctl.next = 0xffffffff;
    }

    return;
}
