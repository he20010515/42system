#include "bootpack.h"
void init_pic(void)
/* PIC初始化 */
{
    io_out8(PIC0_IMR, 0xff); //禁止所有中断
    io_out8(PIC1_IMR, 0xff); //禁止所有中断

    io_out8(PIC0_ICW1, 0x11);   //边沿触发模式
    io_out8(PIC0_ICW2, 0x20);   // PIC0-7由IRQ20-27接收
    io_out8(PIC0_ICW3, 1 << 2); /*PIC1由IRQ2连接*/
    io_out8(PIC0_ICW4, 0x01);   /* 无缓冲区模式 */

    io_out8(PIC1_ICW1, 0x11); /* エッジトリガモード */
    io_out8(PIC1_ICW2, 0x28); /* IRQ8-15由INT28-2f接受 */
    io_out8(PIC1_ICW3, 2);    /* PIC1由IRQ2链接 */
    io_out8(PIC1_ICW4, 0x01); /* 无缓冲区模式 */

    io_out8(PIC0_IMR, 0xfb); /* 11111011 PIC1以外全部禁止 */
    io_out8(PIC1_IMR, 0xff); /* 11111111 禁止所有中断 */

    return;
}

void inthandler21(int *esp)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, "INT 21(IRQ-1):PS/2 keyboard");
    for (;;)
    {
        io_hlt();
    }
}

void inthandler27(int *esp)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, "INT 27(IRQ-1):???");
    for (;;)
    {
        io_hlt();
    }
}
void inthandler2c(int *esp)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, "INT 2C(IRQ-1):PS/2 Mouse");
    for (;;)
    {
        io_hlt();
    }
}