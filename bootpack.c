//操作系统内核的入口文件
#include "bootpack.h"
#include "stdio.h"
#include <string.h>

void task_b_main(struct SHEET *sht_back);
extern struct TIMERCTL timerctl;
extern int keydata0;
extern int mousedata0;

struct TSS32
{
	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;		 // 任务设置相关信息
	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi; // 32位寄存器 eip 是用来记录下一条需要执行的指令位于内存中的那个地址的寄存器 每执行一条指令,EPI中的值就回自动+1;
	int es, cs, ss, ds, fs, gs;								 // 16位寄存器
	int ldtr, iomap;										 // 任务设置相关
};

#define AR_TSS32 0x0089
struct SHEET *SHEET_BACK;
void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
	char tempstr[64];
	struct MOUSE_DEC mdec;
	int i;
	int mx = 0, my = 0;
	struct SHTCTL *shtctl;
	struct SHEET *sht_back, *sht_mouse, *sht_win;
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

	fifo32_init(&fifo, 128, fifobuf);
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
	struct TIMER *timer1, *timer2, *timer3;
	timer1 = timer_alloc();
	timer_init(timer1, &fifo, 10);
	timer_settime(timer1, 1000);
	timer2 = timer_alloc();
	timer_init(timer2, &fifo, 3);
	timer_settime(timer2, 300);
	timer3 = timer_alloc();
	timer_init(timer3, &fifo, 1);
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

	make_window8(buf_win, 160, 68, "window");
	make_textbox8(sht_win, 8, 28, 144, 16, COL8_FFFFFF);

	sheet_slide(sht_back, 0, 0);
	mx = (binfo->scrnx - 16) / 2; /* 将鼠标移动到画面中央 */
	my = (binfo->scrny - 28 - 16) / 2;
	sheet_slide(sht_mouse, mx, my);
	sheet_slide(sht_win, 80, 72);
	sheet_updown(sht_back, 0);
	sheet_updown(sht_win, 1);
	sheet_updown(sht_mouse, 2);
	SHEET_BACK = sht_back;
	sprintf(tempstr, "total memory:%dMB   free:%d KB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfonts8_asc_sht(sht_back, 0, 16 * 2, COL8_FFFFFF, COL8_008484, tempstr);
	int cursor_x = 8, cursor_c = COL8_FFFFFF;
	//任务切换
	struct TSS32 tss_a, tss_b;
	tss_a.ldtr = 0, tss_a.iomap = 0x40000000;
	tss_b.ldtr = 0, tss_b.iomap = 0x40000000;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)ADR_GDT;
	set_segmdesc(gdt + 3, 103, (int)&tss_a, AR_TSS32);
	set_segmdesc(gdt + 4, 103, (int)&tss_b, AR_TSS32);
	load_tr(3 * 8);

	int task_b_esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024; //为进程B准备的栈地址,
	tss_b.eip = (int)&task_b_main;
	tss_b.eflags = 0x00000202;
	tss_b.eax = 0;
	tss_b.ecx = 0;
	tss_b.edx = 0;
	tss_b.ebx = 0;
	tss_b.esp = task_b_esp;
	tss_b.ebp = 0;
	tss_b.esi = 0;
	tss_b.edi = 0;
	tss_b.es = 1 * 8;
	tss_b.cs = 2 * 8;
	tss_b.ss = 1 * 8;
	tss_b.ds = 1 * 8;
	tss_b.fs = 1 * 8;
	tss_b.gs = 1 * 8;
	//多任务切换初始化
	*((int *)(task_b_esp + 4)) = (int)sht_back;
	mt_init();

	for (;;)
	{
		io_cli();
		if (fifo32_status(&fifo) == 0)
		{
			io_stihlt();
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
				case 10:
					putfonts8_asc_sht(sht_back, 0, 64, COL8_FFFFFF, COL8_008484, "10[sec]");
					break;
				case 3:
					putfonts8_asc_sht(sht_back, 0, 80, COL8_FFFFFF, COL8_008484, "3[sec]");
					break;
				case 1:
				case 0:
					if (i != 0)
					{
						timer_init(timer3, &fifo, 0);
						cursor_c = COL8_000000;
					}
					else
					{
						timer_init(timer3, &fifo, 1);
						cursor_c = COL8_FFFFFF;
					}
					timer_settime(timer3, 50);
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

void task_b_main(struct SHEET *sht_back)
{
	struct FIFO32 fifo;
	struct TIMER *timer_put, *timer_1s;
	int i, fifobuf[128], count = 0, count0 = 0;
	char s[12];
	sht_back = SHEET_BACK;

	fifo32_init(&fifo, 128, fifobuf);
	timer_put = timer_alloc();
	timer_init(timer_put, &fifo, 1);
	timer_settime(timer_put, 1);
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
			if (i == 1)
			{
				sprintf(s, "%11d", count);
				putfonts8_asc_sht(sht_back, 0, 144, COL8_FFFFFF, COL8_008484, s);
				timer_settime(timer_put, 1);
			}
			else if (i == 100)
			{
				sprintf(s, "%11d", count - count0);
				putfonts8_asc_sht(sht_back, 0, 128, COL8_FFFFFF, COL8_008484, s);
				count0 = count;
				timer_settime(timer_1s, 100);
			}
		}
	}
}