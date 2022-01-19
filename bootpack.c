//操作系统内核的入口文件
#include "bootpack.h"
#include "stdio.h"
#include <string.h>
// tasks
void console_task(struct SHEET *sheet);
void HariMain(void);
// extern data
extern struct TIMERCTL timerctl;
// data
static char keytable0[0x54] = {
	0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0, 0,
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0, 0, 'A', 'S',
	'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0, 0, ']', 'Z', 'X', 'C', 'V',
	'B', 'N', 'M', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
	'2', '3', '0', '.'};
static char keytable1[0x80] = {
	0, 0, '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0, 0,
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0, 0, 'A', 'S',
	'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0, 0, '}', 'Z', 'X', 'C', 'V',
	'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
	'2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, '_', 0, 0, 0, 0, 0, 0, 0, 0, 0, '|', 0, 0};
// in
void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
	char tempstr[64];
	struct MOUSE_DEC mdec;
	int i;
	int mx = 0, my = 0;
	struct SHTCTL *shtctl;
	struct SHEET *sht_back, *sht_mouse, *sht_win_taska, *sht_win_console;
	unsigned char *buf_back, buf_mouse[16 * 16], *buf_win;
	struct FIFO32 fifo;
	int fifobuf[128];
	struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;

	fifo32_init(&fifo, 128, fifobuf, 0);
	init_gdtidt();
	init_pic();
	io_sti(); // IDT/PIC初始化已经完成,开放CPU中断
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

	// sht_ctl
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	//任务切换
	struct TASK *task_console, *task_a;
	task_a = task_init(memman); // 第一个任务
	task_run(task_a, 1, 2);
	fifo.task = task_a;
	// sht_back
	sht_back = sheet_alloc(shtctl);
	buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); //没有透明色
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);
	// sht_win_console
	unsigned char *buf_win_console;
	sht_win_console = sheet_alloc(shtctl);
	buf_win_console = (unsigned char *)memman_alloc_4k(memman, 256 * 165);
	sheet_setbuf(sht_win_console, buf_win_console, 256, 165, -1); //无透明色
	sprintf(tempstr, "console");
	make_window8(buf_win_console, 256, 165, tempstr, 0);
	make_textbox8(sht_win_console, 8, 28, 240, 128, COL8_000000);
	task_console = task_alloc();
	task_console->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
	task_console->tss.eip = (int)&console_task;
	task_console->tss.es = 1 * 8;
	task_console->tss.cs = 2 * 8;
	task_console->tss.ss = 1 * 8;
	task_console->tss.ds = 1 * 8;
	task_console->tss.fs = 1 * 8;
	task_console->tss.gs = 1 * 8;
	*((int *)(task_console->tss.esp + 4)) = (int)sht_win_console;
	task_run(task_console, 2, 2);
	// sht_win_taska
	buf_win = (unsigned char *)memman_alloc_4k(memman, 144 * 52);
	sht_win_taska = sheet_alloc(shtctl);
	sheet_setbuf(sht_win_taska, buf_win, 144, 52, -1);
	make_window8(buf_win, 144, 52, "task_a", 1);
	make_textbox8(sht_win_taska, 8, 28, 128, 16, COL8_FFFFFF);
	int cursor_x = 8, cursor_c = COL8_FFFFFF;
	struct TIMER *cursor_timer;
	cursor_timer = timer_alloc();
	timer_init(cursor_timer, &fifo, 1);
	timer_settime(cursor_timer, 50);

	// sht_mouse
	sht_mouse = sheet_alloc(shtctl);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	init_mouse_cursor8(buf_mouse, 99);
	mx = (binfo->scrnx - 16) / 2; /* 将鼠标移动到画面中央 */
	my = (binfo->scrny - 28 - 16) / 2;
	// sheets slide
	sheet_slide(sht_mouse, mx, my);
	sheet_slide(sht_back, 0, 0);
	sheet_slide(sht_win_taska, 8, 56);
	sheet_slide(sht_win_console, 168, 56);
	// sheet updown
	sheet_updown(sht_back, 0);
	sheet_updown(sht_win_taska, 4);
	sheet_updown(sht_win_console, 1);
	sheet_updown(sht_mouse, 5);
	// memory
	sprintf(tempstr, "(%3d, %3d)", mx, my);
	putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, tempstr);
	sprintf(tempstr, "total memory:%dMB   free:%d KB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfonts8_asc_sht(sht_back, 0, 16 * 2, COL8_FFFFFF, COL8_008484, tempstr);
	// key control
	int key_to = 0, key_shift, key_leds = (binfo->leds >> 4) & 7;

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
				if (i < 0x80)
				{
					if (key_shift == 0)
					{
						tempstr[0] = keytable0[i];
					}
					else
					{
						tempstr[0] = keytable1[i];
					}
				}
				else
				{
					tempstr[0] = 0;
				}
				if (tempstr[0] >= 'A' AND tempstr[0] <= 'Z') //如果输入字母是英文字母时
				{
					if (((key_leds & 4) == 0 AND key_shift == 0) OR((key_leds & 4) != 0 AND key_shift != 0))
					{
						tempstr[0] += 'a' - 'A';
					}
				}

				if (tempstr[0] != 0) // 字母键-一般字符
				{
					if (key_to == 0)
					{
						if (keytable0[i] != 0 AND cursor_x < 128)
						{
							tempstr[1] = 0;
							putfonts8_asc_sht(sht_win_taska, cursor_x, 28, COL8_000000, COL8_FFFFFF, tempstr);
							cursor_x += 8;
						}
					}
					else
					{
						fifo32_put(&task_console->fifo, tempstr[0] + 256);
					}
				}
				if (i == 0x0e) // 退格键
				{
					if (key_to == 0 AND cursor_x > 8)
					{
						putfonts8_asc_sht(sht_win_taska, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ");
						cursor_x -= 8;
					}
					else
					{
						fifo32_put(&task_console->fifo, 8 + 256);
					}
				}
				if (i == 0x0f) // tab键
				{
					switch (key_to)
					{
					case 0:
						key_to = 1;
						make_wtitle8(buf_win, sht_win_taska->bxsize, "task_a", 0);
						make_wtitle8(sht_win_console->buf, sht_win_console->bxsize, "console", 1);
						break;
					case 1:
						key_to = 0;
						make_wtitle8(buf_win, sht_win_taska->bxsize, "task_a", 1);
						make_wtitle8(sht_win_console->buf, sht_win_console->bxsize, "console", 0);
						break;
					}
					sheet_refresh(sht_win_taska, 0, 0, sht_win_taska->bxsize, 21);
					sheet_refresh(sht_win_console, 0, 0, sht_win_console->bxsize, 21);
				}
				switch (i) // shift键控制
				{
				case 0x2a: //左shift ON
					key_shift |= 1;
					break;
				case 0x36: //右shifht ON
					key_shift |= 2;
					break;
				case 0xaa: //左shift OFF
					key_shift &= ~1;
					break;
				case 0xb6: //右shifht OFF
					key_shift &= ~2;
					break;
				}
				boxfill8(sht_win_taska->buf, sht_win_taska->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
				sheet_refresh(sht_win_taska, cursor_x, 28, cursor_x + 8, 44);
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
						sheet_slide(sht_win_taska, mx - 80, my - 8);
					}
				}
			}
			else if (i == 2 OR i == 10 OR i == 1 OR i == 3 OR i == 0) // 光标定时器
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
					boxfill8(sht_win_taska->buf, sht_win_taska->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
					sheet_refresh(sht_win_taska, cursor_x, 28, cursor_x + 8, 44);
					break;
				default:
					break;
				}
			}
		}
	}
}
void console_task(struct SHEET *sheet)
{
	struct TIMER *cursor_timer;
	char s[100];
	int i, fifobuf[128], cursor_x = 16, cursor_c = COL8_000000;
	struct TASK *task = task_now();
	fifo32_init(&task->fifo, 128, fifobuf, task);
	// cursor
	cursor_timer = timer_alloc();
	timer_init(cursor_timer, &task->fifo, 1);
	timer_settime(cursor_timer, 50); // 0.5s光标闪烁

	putfonts8_asc_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, ">");
	for (;;)
	{
		io_cli();
		if (fifo32_status(&task->fifo) == 0)
		{
			task_sleep(task);
			io_sti();
		}
		else
		{
			i = fifo32_get(&task->fifo);
			io_sti();
			if (i <= 1) // i = 1 or 0
			{
				switch (i)
				{
				case 0:
					timer_init(cursor_timer, &task->fifo, 1);
					cursor_c = COL8_000000;
					break;
				case 1:
					timer_init(cursor_timer, &task->fifo, 0);
					cursor_c = COL8_FFFFFF;
					break;
				default:
					break;
				}
				timer_settime(cursor_timer, 50);
				boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
				sheet_refresh(sheet, cursor_x, 28, cursor_x + 8, 44);
			}
			if (256 <= i AND i <= 511) //来自taska的键盘数据
			{
				i -= 256;
				if (i == 8)
				{
					if (cursor_x > 16)
					{
						putfonts8_asc_sht(sheet, cursor_x, 28, COL8_FFFFFF, COL8_000000, " ");
						cursor_x -= 8;
					}
				}
				else
				{
					if (cursor_x < 240)
					{
						s[0] = i;
						s[1] = 0;
						putfonts8_asc_sht(sheet, cursor_x, 28, COL8_FFFFFF, COL8_000000, s);
						cursor_x += 8;
					}
				}
			}
			boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
			sheet_refresh(sheet, cursor_x, 28, cursor_x + 8, 44);
		}
	}
}
