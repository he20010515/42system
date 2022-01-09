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
    struct SHTCTL *shtctl
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
    unsigned int timeout, flags;
    struct FIFO32 *fifo;
    unsigned char data;
};
struct TIMERCTL
{
    unsigned int count, next, using; // 计数,下一个时刻,当前有几个定时器处于活动中
    struct TIMER timers0[MAX_TIMER];
    struct TIMER *timers[MAX_TIMER];
};