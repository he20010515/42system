//操作系统内核的入口文件
#include "bootpack.h"
#include "stdio.h"
#include <string.h>

void task_b_main(struct SHEET *sht_back);
extern struct TIMERCTL timerctl;
extern int keydata0;
extern int mousedata0;

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
	char tempstr[64];
	struct MOUSE_DEC mdec;
	int i;
	int mx = 0, my = 0;
	struct SHTCTL *shtctl;
	struct SHEET *sht_back, *sht_mouse, *sht_win, *sht_win_b[3];
	unsigned char *buf_back, buf_mouse[16 * 16], *buf_win;
	struct FIFO32 fifo;
	int fifobuf[128];
	struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
	static char keytable[0x54] = {
		0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0, 0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0, 0, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0, 0, ']', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.'};

	fifo32_init(&fifo, 128, fifobuf, 0);
	init_gdtidt();
	init_pic();
	io_sti(); //IDT/PIC初始化已经完成,开放CPU中断
	init_pit();

	init_keyboard(&fifo, 256);
	enable_mouse(&fifo, 512, &mdec);
	io_out8(PIC0_IMR, 0xf8); //开放PIC1和键盘中断
	io_out8(PIC1_IMR, 0xef); //开放鼠标中断
	unsigned int memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000);
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	init_palette();

	//timer设定开始
	struct TIMER *cursor_timer;
	cursor_timer = timer_alloc();
	timer_init(cursor_timer, &fifo, 1);
	timer_settime(cursor_timer, 50);
	//sht_ctl
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	//sht_back
	sht_back = sheet_alloc(shtctl);
	buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); //没有透明色
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);
	//sht_mouse
	sht_mouse = sheet_alloc(shtctl);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	init_mouse_cursor8(buf_mouse, 99);
	mx = (binfo->scrnx - 16) / 2; /* 将鼠标移动到画面中央 */
	my = (binfo->scrny - 28 - 16) / 2;
	sheet_slide(sht_mouse, mx, my);
	//sht_win
	buf_win = (unsigned char *)memman_alloc_4k(memman, 160 * 52);
	sht_win = sheet_alloc(shtctl);
	sheet_setbuf(sht_win, buf_win, 160, 52, -1);
	make_window8(buf_win, 160, 68, "window", 1);
	make_textbox8(sht_win, 8, 28, 144, 16, COL8_FFFFFF);

	//memory
	sprintf(tempstr, "total memory:%dMB   free:%d KB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfonts8_asc_sht(sht_back, 0, 16 * 2, COL8_FFFFFF, COL8_008484, tempstr);
	int cursor_x = 8, cursor_c = COL8_FFFFFF;
	//任务切换
	struct TASK *task_b[3], *task_a;
	task_a = task_init(memman); // 第一个任务
	fifo.task = task_a;
	//sht_win_b
	for (i = 0; i < 3; i++)
	{
		sht_win_b[i] = sheet_alloc(shtctl);
		unsigned char *buf_win_b = (unsigned char *)memman_alloc_4k(memman, 144 * 52);
		sheet_setbuf(sht_win_b[i], buf_win_b, 144, 52, -1); //无透明色
		sprintf(tempstr, "task_b%d", i);
		make_window8(buf_win_b, 144, 52, tempstr, 0);
		task_b[i] = task_alloc();
		task_b[i]->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
		task_b[i]->tss.eip = (int)&task_b_main;
		task_b[i]->tss.es = 1 * 8;
		task_b[i]->tss.cs = 2 * 8;
		task_b[i]->tss.ss = 1 * 8;
		task_b[i]->tss.ds = 1 * 8;
		task_b[i]->tss.fs = 1 * 8;
		task_b[i]->tss.gs = 1 * 8;
		*((int *)(task_b[i]->tss.eip + 4)) = (int)sht_win_b[i];
		task_run(task_b[i]);
	}

	//sheets slide
	sheet_slide(sht_back, 0, 0);
	sheet_slide(sht_win, 8, 56);
	sheet_slide(sht_win_b[0], 168, 56);
	sheet_slide(sht_win_b[1], 8, 116);
	sheet_slide(sht_win_b[2], 168, 116);
	//sheet updown
	sheet_updown(sht_back, 0);
	sheet_updown(sht_win, 4);
	sheet_updown(sht_win_b[0], 1);
	sheet_updown(sht_win_b[1], 2);
	sheet_updown(sht_win_b[2], 3);
	sheet_updown(sht_mouse, 5);
	for (;;)
	{
		io_cli();
		if (fifo32_status(&fifo) == 0)
		{
			task_sleep(task_a);
			io_sti();
		}
		else
		{
			i = fifo32_get(&fifo);
			io_sti();
			if (256 <= i AND i <= 511) // 键盘数据
			{
				i -= 256;
				sprintf(tempstr, "%02X", i);
				putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, tempstr);

				if (i < 0x54)
				{
					if (keytable[i] != 0 AND cursor_x < 144)
					{
						tempstr[0] = keytable[i];
						tempstr[1] = 0;
						putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, tempstr);
						cursor_x += 8;
					}
				}
				if (i == 0x0e AND cursor_x > 8)
				{
					putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ");
					cursor_x -= 8;
				}
				boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
				sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
			}
			else if (512 <= i AND i <= 767) //鼠标数据
			{
				i -= 512;
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

					//窗口移动
					if ((mdec.btn & 0x01) != 0)
					{
						sheet_slide(sht_win, mx - 80, my - 8);
					}
				}
			}
			else if (i == 2 OR i == 10 OR i == 1 OR i == 3 OR i == 0)
			{
				switch (i)
				{
				case 1:
				case 0:
					if (i != 0)
					{
						timer_init(cursor_timer, &fifo, 0);
						cursor_c = COL8_000000;
					}
					else
					{
						timer_init(cursor_timer, &fifo, 1);
						cursor_c = COL8_FFFFFF;
					}
					timer_settime(cursor_timer, 50);
					boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
					sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
					break;
				default:
					break;
				}
			}
		}
	}
}
void task_b_main(struct SHEET *sht_win_b)
{
	struct FIFO32 fifo;
	struct TIMER *timer_1s;
	int i, fifobuf[128], count = 0, count0 = 0;
	char s[12];

	fifo32_init(&fifo, 128, fifobuf, 0);
	timer_1s = timer_alloc();
	timer_init(timer_1s, &fifo, 100);
	timer_settime(timer_1s, 100);

	for (;;)
	{
		count++;
		io_cli();
		if (fifo32_status(&fifo) == 0)
		{
			io_sti();
		}
		else
		{
			i = fifo32_get(&fifo);
			io_sti();
			if (i == 100)
			{
				sprintf(s, "%11d", count - count0);
				putfonts8_asc_sht(sht_win_b, 24, 28, COL8_000000, COL8_C6C6C6, s);
				count0 = count;
				timer_settime(timer_1s, 100);
			}
		}
	}
}