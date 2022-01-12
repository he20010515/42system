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
    struct TIMER *t;
    int i;
    for (i = 0; i < MAX_TIMER; i++)
    {
        timerctl.timers0[i].flags = 0; //未使用
    }
    t = timer_alloc(); // 定时器哨兵
    t->timeout = 0xFFFFFFFF;
    t->flags = TIMER_FLAGS_USING;
    t->next_timer = 0;
    timerctl.t0 = t;
    timerctl.next_time = 0xffffffff;
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
    int e;
    struct TIMER *t, *s;
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    e = io_load_eflags();
    io_cli();

    t = timerctl.t0;
    if (timer->timeout <= t->timeout)
    {
        //插入最前面的情况下
        timerctl.t0 = timer;
        timer->next_timer = t;
        timerctl.next_time = timer->timeout;
        io_store_eflags(e);
        return;
    }
    for (;;)
    {
        s = t;
        t = t->next_timer;
        if (t == 0)
        {
            break; //最后面了
        }
        if (timer->timeout <= t->timeout)
        {
            //插入到s和t之间时:
            s->next_timer = timer;
            timer->next_timer = t;
            io_store_eflags(e);
            return;
        }
    }
    return;
}
extern struct TIMER *task_timer;

void inthandler20(int *esp)
{
    struct TIMER *timer;
    char ts = 0;
    io_out8(PIC0_OCW2, 0x60);
    timerctl.count++;
    if (timerctl.next_time > timerctl.count)
    {
        return; // 下一个时刻没有定时器要时间到,那么直接返回什么都不做
    }
    timer = timerctl.t0; //把最前面的地址赋给timer
    for (;;)
    {
        /* timers的定时器都处于动作中，所以不确认flags */
        if (timer->timeout > timerctl.count)
        {
            break;
        }
        //超时
        timer->flags = TIMER_FLAGS_ALLOC;
        if (timer != task_timer)
        {
            fifo32_put(timer->fifo, timer->data);
        }
        else
        {
            ts = 1; //mt_timer 超时
        }

        timer = timer->next_timer; //下一个定时器的地址赋给timer
    }
    //正好有i个定时器超时了,其余的进行移位
    timerctl.t0 = timer;
    //timerctl.next的设定
    timerctl.next_time = timerctl.t0->timeout;
    if (ts != 0)
    {
        task_switch();
    }

    return;
}
