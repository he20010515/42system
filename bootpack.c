//操作系统内核的入口文件
#include "bootpack.h"
#include "stdio.h"
extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;
void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;

	char tempstr[64];
	char mcoursor[16 * 16], keybuf[32], mousebuf[128];
	struct MOUSE_DEC mdec;
	int i;
	int mx = 0, my = 0;
	fifo8_init(&keyfifo, 32, keybuf);
	fifo8_init(&mousefifo, 128, mousebuf);
	init_mouse_cursor8(mcoursor, COL8_008484);
	init_gdtidt();
	init_pic();
	init_keyboard();
	enable_mouse();

	io_sti(); //IDT/PIC初始化已经完成,开放CPU中断
	init_palette();
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

	io_out8(PIC0_IMR, 0xf9); //开放PIC1和键盘中断
	io_out8(PIC1_IMR, 0xef); //开放鼠标中断
	unsigned int memtotal = memtest(0x00400000, 0xbfffffff);
	sprintf(tempstr, "total memory:%dMB", memtotal / (1024 * 1024));
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16 * 3, COL8_FFFFFF, tempstr);
	for (;;)
	{
		io_cli();
		if ((fifo8_status(&keyfifo) + fifo8_status(&mousefifo)) == 0)
		{
			io_stihlt();
		}
		else
		{
			if (fifo8_status(&keyfifo) != 0)
			{

				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf(tempstr, "%02X", i);
				boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
				putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, tempstr);
			}
			else if (fifo8_status(&mousefifo) != 0)
			{
				i = fifo8_get(&mousefifo);
				io_sti();
				if (mouse_decode(&mdec, i) != 0)
				{
					sprintf(tempstr, "[lcr %4d %4d]", mdec.x, mdec.y);
					if ((mdec.btn & 0x01) != 0)
					{
						tempstr[1] = 'L';
					}
					if ((mdec.btn & 0x02) != 0)
					{
						tempstr[3] = 'R';
					}
					if ((mdec.btn & 0x04) != 0)
					{
						tempstr[2] = 'C';
					}
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
					putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, tempstr);
					//鼠标指针的移动
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0)
					{
						mx = 0;
					}
					if (my < 0)
					{
						my = 0;
					}
					if (mx > binfo->scrnx - 16)
					{
						mx = binfo->scrnx - 16;
					}
					if (my > binfo->scrny - 16)
					{
						my = binfo->scrny - 16;
					}
					sprintf(tempstr, "(%3d,%3d)", mx, my);
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15);
					putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, tempstr);
					putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcoursor, 16);
				}
			}
		}
	}
}
