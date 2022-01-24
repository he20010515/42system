#include "bootpack.h"

struct FIFO32 *keyfifo;
int keydata0;
void init_keyboard(struct FIFO32 *fifo, int data0)
{
    //初始化键盘控制器电路
    keyfifo = fifo;
    keydata0 = data0;
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
    return;
}
void wait_KBC_sendready(void)
{
    //等待键盘控制器电路准备完毕
    for (;;)
    {
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0)
        {
            break;
        }
    }
    return;
}
void inthandler21(int *esp)
{
    unsigned char data;
    io_out8(PIC0_OCW2, 0x61); //通知PIC IRQ-01已经受理完毕
    data = io_in8(PORT_KEYDAT);
    fifo32_put(keyfifo, keydata0 + data);
    return;
}
