//操作系统内核的入口文件
#include "bootpack.h"
#include "stdio.h"
#include <string.h>
extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;
extern struct TIMERCTL timerctl;

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
	char tempstr[64];
	char keybuf[32], mousebuf[128];
	struct MOUSE_DEC mdec;
	int i;
	int mx = 0, my = 0;
	struct SHTCTL *shtctl;
	struct SHEET *sht_back, *sht_mouse, *sht_win;
	unsigned char *buf_back, buf_mouse[16 * 16], *buf_win;
	init_gdtidt();
	init_pic();
	fifo8_init(&keyfifo, 32, keybuf);
	fifo8_init(&mousefifo, 128, mousebuf);
	init_pit();
	io_out8(PIC0_IMR, 0xf8); //开放PIC1和键盘中断
	io_out8(PIC1_IMR, 0xef); //开放鼠标中断
	io_sti();				 //IDT/PIC初始化已经完成,开放CPU中断
	init_keyboard();
	enable_mouse();
	struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
	unsigned int memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000);
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	init_palette();
	//timer设定开始
	char timerbuf[32];
	struct FIFO8 timerfifo;
	struct TIMER *timer1, *timer2, *timer3;
	fifo8_init(&timerfifo, 32, timerbuf);
	timer1 = timer_alloc();
	timer_init(timer1, &timerfifo, 10);
	timer_settime(timer1, 1000);
	timer2 = timer_alloc();
	timer_init(timer2, &timerfifo, 3);
	timer_settime(timer2, 300);
	timer3 = timer_alloc();
	timer_init(timer3, &timerfifo, 1);
	timer_settime(timer3, 50);
	//定时器设定结束

	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	sht_back = sheet_alloc(shtctl);
	sht_mouse = sheet_alloc(shtctl);
	sht_win = sheet_alloc(shtctl);
	buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	buf_win = (unsigned char *)memman_alloc_4k(memman, 160 * 52);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); //没有透明色
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	sheet_setbuf(sht_win, buf_win, 160, 52, -1);
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(buf_mouse, 99);

	make_window8(buf_win, 160, 68, "counter");

	sheet_slide(sht_back, 0, 0);
	mx = (binfo->scrnx - 16) / 2; /* 将鼠标移动到画面中央 */
	my = (binfo->scrny - 28 - 16) / 2;
	sheet_slide(sht_mouse, mx, my);
	sheet_slide(sht_win, 80, 72);
	sheet_updown(sht_back, 0);
	sheet_updown(sht_win, 1);
	sheet_updown(sht_mouse, 2);

	sprintf(tempstr, "total memory:%dMB   free:%d KB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfonts8_asc_sht(sht_back, 0, 16 * 2, COL8_FFFFFF, COL8_008484, tempstr);
	int dosth;
	for (;;)
	{
		sprintf(tempstr, "%10d", timerctl.count);
		boxfill8(buf_win, 160, COL8_C6C6C6, 40, 28, 119, 43);
		putfonts8_asc(buf_win, 160, 40, 28, COL8_000000, tempstr);

		sheet_refresh(sht_win, 40, 28, 120, 44);

		io_cli();
		dosth = fifo8_status(&keyfifo) + fifo8_status(&mousefifo) + fifo8_status(&timerfifo);
		if (dosth == 0)
		{
			io_sti();
		}
		else
		{
			if (fifo8_status(&keyfifo) != 0)
			{

				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf(tempstr, "%02X", i);
				putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, tempstr);
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
					putfonts8_asc_sht(sht_back, 32, 16, COL8_FFFFFF, COL8_008484, tempstr);
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
					if (mx > binfo->scrnx - 1)
					{
						mx = binfo->scrnx - 1;
					}
					if (my > binfo->scrny - 1)
					{
						my = binfo->scrny - 1;
					}
					sprintf(tempstr, "(%3d,%3d)", mx, my);
					putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, tempstr);
					sheet_slide(sht_mouse, mx, my);
				}
			}
			else if (fifo8_status(&timerfifo) != 0)
			{
				i = fifo8_get(&timerfifo);
				switch (i)
				{
				case 10:
					io_sti();
					putfonts8_asc_sht(sht_back, 0, 64, COL8_FFFFFF, COL8_008484, "10[sec]");
					break;
				case 3:
					io_sti();
					putfonts8_asc_sht(sht_back, 0, 80, COL8_FFFFFF, COL8_008484, "3[sec]");
				case 1:
				case 0:
					if (i != 0)
					{
						timer_init(timer3, &timerfifo, 0);
						boxfill8(buf_back, binfo->scrnx, COL8_FFFFFF, 8, 96, 15, 111);
					}
					else
					{
						timer_init(timer3, &timerfifo, 1);
						boxfill8(buf_back, binfo->scrnx, COL8_008484, 8, 96, 15, 111);
					}
					timer_settime(timer3, 50);
					sheet_refresh(sht_back, 8, 96, 16, 112);
					break;
				default:
					break;
				}
			}
		}
	}
}
