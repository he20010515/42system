//操作系统内核的入口文件
#include "bootpack.h"
#include "stdio.h"
extern struct FIFO8 keyfifo;

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;

	char tempstr[64];
	char mouse[16 * 16], keybuf[32];
	int i;
	fifo8_init(&keyfifo, 32, keybuf);
	init_mouse_cursor8(mouse, COL8_008484);
	init_gdtidt();
	init_pic();
	io_sti(); //IDT/PIC初始化已经完成,开放CPU中断
	init_palette();
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

	io_out8(PIC0_IMR, 0xf9); //开放PIC1和键盘中断
	io_out8(PIC1_IMR, 0xef); //开放鼠标中断

	for (;;)
	{
		io_cli();
		if (fifo8_status(&keyfifo) == 0)
		{
			io_stihlt();
		}
		else
		{
			i = fifo8_get(&keyfifo);
			io_sti();
			sprintf(tempstr, "%02X", i);
			boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
			putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, tempstr);
		}
	}
}
