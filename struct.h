#include "define.h"
struct BOOTINFO
{
    char cyls, leds, vmode, reserve;
    short scrnx, scrny; //画面x,y大小
    char *vram;         //显存起始地址
};
struct SEGMENT_DESCRIPTOR
{
    short limit_low, base_low;
    char base_mid, access_right;
    char limit_high, base_high;
};
struct GATE_DESCRIPTOR
{
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};
struct FIFO8
{
    unsigned char *buf;
    int p, q, size, free, flags;
};
struct FIFO32
{
    int *buf;
    int p, q, size, free, flags;
    struct TASK *task;
};
struct MOUSE_DEC
{
    unsigned char buf[3], phase;
    int x, y, btn;
};
struct FREEINFO
{
    unsigned int addr, size;
};
struct MEMMAN
{
    int frees, maxfrees, lostsize, losts;
    struct FREEINFO free[MEMMAN_FREES];
};
struct SHEET
{
    unsigned char *buf;
    int bxsize, bysize, vx0, vy0, col_inv, height, flags;
    struct SHTCTL *shtctl;
};
struct SHTCTL
{
    unsigned char *vram, *map;
    int xsize, ysize, top;
    struct SHEET *sheets[MAX_SHEETS];
    struct SHEET sheets0[MAX_SHEETS];
};
struct TIMER
{
    struct TIMER *next_timer; // 下一个超时的计时器地址
    unsigned int timeout, flags;
    struct FIFO32 *fifo;
    unsigned char data;
};
struct TIMERCTL
{
    unsigned int count, next_time; // 计数,下一个时刻,当前有几个定时器处于活动中
    struct TIMER timers0[MAX_TIMER];
    struct TIMER *t0;
};

struct TSS32
{
    int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;      // 任务设置相关信息
    int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi; // 32位寄存器 eip 是用来记录下一条需要执行的指令位于内存中的那个地址的寄存器 每执行一条指令,EPI中的值就回自动+1;
    int es, cs, ss, ds, fs, gs;                              // 16位寄存器
    int ldtr, iomap;                                         // 任务设置相关
};
#define MAX_TASKS 1000
#define MAX_TASKLEVELS 10
#define MAX_TASKS_LV 100
struct TASK
{
    int sel, flags;      // sel用来存放GDT的编号;
    int priority, level; // 优先级
    struct TSS32 tss;
    struct FIFO32 fifo;
};

struct TASKLEVEL
{
    int running; //正在运行的任务数量
    int now;     //记录当前运行的是那个任务
    struct TASK *tasks[MAX_TASKS_LV];
};

struct TASKCTL
{
    int now_lv;     //现在活动中的level
    char lv_change; //下次任务切换是否需要改变level
    struct TASKLEVEL level[MAX_TASKLEVELS];
    struct TASK tasks0[MAX_TASKS];
};
struct FILEINFO
{
    unsigned char name[8], ext[3], type;
    char reserve[10];
    unsigned short time, date, clustno;
    unsigned int size;
};
struct CONSOLE
{
    struct SHEET *sht;
    int cur_x, cur_y, cur_c;
};
