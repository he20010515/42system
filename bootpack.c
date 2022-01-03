//操作系统内核的入口文件
#include "bootpack.h"
#include "stdio.h"

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *)0x0ff0;
	init_palette();
	char tempstr[64];
	char mouse[16 * 16];
	init_mouse_cursor8(mouse, COL8_00FFFF);
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
	sprintf(tempstr, "location of binfo:0x%08x", binfo);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, 0, 0, mouse, 16);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_00FFFF, tempstr);
	for (;;)
	{
		io_hlt();
	}
}

void init_screen8(char *vram, int xsize, int ysize)
{
	boxfill8(vram, xsize, COL8_008484, 0, 0, xsize - 1, ysize - 29);
	boxfill8(vram, xsize, COL8_C6C6C6, 0, ysize - 28, xsize - 1, ysize - 28);
	boxfill8(vram, xsize, COL8_FFFFFF, 0, ysize - 27, xsize - 1, ysize - 27);
	boxfill8(vram, xsize, COL8_C6C6C6, 0, ysize - 26, xsize - 1, ysize - 1);

	boxfill8(vram, xsize, COL8_FFFFFF, 3, ysize - 24, 59, ysize - 24);
	boxfill8(vram, xsize, COL8_FFFFFF, 2, ysize - 24, 2, ysize - 4);
	boxfill8(vram, xsize, COL8_848484, 3, ysize - 4, 59, ysize - 4);
	boxfill8(vram, xsize, COL8_848484, 59, ysize - 23, 59, ysize - 5);
	boxfill8(vram, xsize, COL8_000000, 2, ysize - 3, 59, ysize - 3);
	boxfill8(vram, xsize, COL8_000000, 60, ysize - 24, 60, ysize - 3);

	boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 24, xsize - 4, ysize - 24);
	boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 23, xsize - 47, ysize - 4);
	boxfill8(vram, xsize, COL8_FFFFFF, xsize - 47, ysize - 3, xsize - 4, ysize - 3);
	boxfill8(vram, xsize, COL8_FFFFFF, xsize - 3, ysize - 24, xsize - 3, ysize - 3);
}

void init_palette(void)
{
	static unsigned char table_rgb[16 * 3] = {
		0x00, 0x00, 0x00, /*  0:黒 */
		0xff, 0x00, 0x00, /*  1:明るい赤 */
		0x00, 0xff, 0x00, /*  2:明るい緑 */
		0xff, 0xff, 0x00, /*  3:明るい黄色 */
		0x00, 0x00, 0xff, /*  4:明るい青 */
		0xff, 0x00, 0xff, /*  5:明るい紫 */
		0x00, 0xff, 0xff, /*  6:明るい水色 */
		0xff, 0xff, 0xff, /*  7:白 */
		0xc6, 0xc6, 0xc6, /*  8:明るい灰色 */
		0x84, 0x00, 0x00, /*  9:暗い赤 */
		0x00, 0x84, 0x00, /* 10:暗い緑 */
		0x84, 0x84, 0x00, /* 11:暗い黄色 */
		0x00, 0x00, 0x84, /* 12:暗い青 */
		0x84, 0x00, 0x84, /* 13:暗い紫 */
		0x00, 0x84, 0x84, /* 14:暗い水色 */
		0x84, 0x84, 0x84  /* 15:暗い灰色 */
	};
	set_palette(0, 15, table_rgb);
	return;
}

void set_palette(int start, int end, unsigned char *rgb)
{

	int i, eflags;
	eflags = io_load_eflags(); /* 加载eflags状态 */
	io_cli();				   /* 許可フラグを0にして割り込み禁止にする */
	io_out8(0x03c8, start);
	for (i = start; i <= end; i++)
	{
		io_out8(0x03c9, rgb[0] / 4);
		io_out8(0x03c9, rgb[1] / 4);
		io_out8(0x03c9, rgb[2] / 4);
		rgb += 3;
	}
	io_store_eflags(eflags); /* 割り込み許可フラグを元に戻す */
	return;
}

void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1)
{

	int x, y;
	for (y = y0; y <= y1; y++)
	{
		for (x = x0; x <= x1; x++)
			vram[y * xsize + x] = c;
	}
	return;
}

void putfont8(char *vram, int xsize, int x, int y, char c, char *font)
{
	int i;
	char *p, d /* data */;
	for (i = 0; i < 16; i++)
	{
		p = vram + (y + i) * xsize + x;
		d = font[i];
		if ((d & 0x80) != 0)
		{
			p[0] = c;
		}
		if ((d & 0x40) != 0)
		{
			p[1] = c;
		}
		if ((d & 0x20) != 0)
		{
			p[2] = c;
		}
		if ((d & 0x10) != 0)
		{
			p[3] = c;
		}
		if ((d & 0x08) != 0)
		{
			p[4] = c;
		}
		if ((d & 0x04) != 0)
		{
			p[5] = c;
		}
		if ((d & 0x02) != 0)
		{
			p[6] = c;
		}
		if ((d & 0x01) != 0)
		{
			p[7] = c;
		}
	}
	return;
}

void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s)
{
	extern char hankaku[4096];
	for (; *s != 0x00; s++)
	{
		putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
		x += 8;
	}
	return;
}

void init_mouse_cursor8(char *mouse, char bc)
// 向mouse指向的内存写入指针图案,bc是指背景颜色
{
	static char cursor[16][16] = {
		"**************..",
		"*OOOOOOOOOOO*...",
		"*OOOOOOOOOO*....",
		"*OOOOOOOOO*.....",
		"*OOOOOOOO*......",
		"*OOOOOOO*.......",
		"*OOOOOOO*.......",
		"*OOOOOOOO*......",
		"*OOOO**OOO*.....",
		"*OOO*..*OOO*....",
		"*OO*....*OOO*...",
		"*O*......*OOO*..",
		"**........*OOO*.",
		"*..........*OOO*",
		"............*OO*",
		".............***"};
	int x, y;

	for (y = 0; y < 16; y++)
	{
		for (x = 0; x < 16; x++)
		{
			if (cursor[y][x] == '*')
			{
				mouse[y * 16 + x] = COL8_000000;
			}
			if (cursor[y][x] == 'O')
			{
				mouse[y * 16 + x] = COL8_FFFFFF;
			}
			if (cursor[y][x] == '.')
			{
				mouse[y * 16 + x] = bc;
			}
		}
	}
	return;
}

void putblock8_8(char *vram, int vxsize, int pxsize,
				 int pysize, int px0, int py0, char *buf, int bxsize)
{
	//从buf取得数据,填充到vram指定位置
	//px/ysize是指指定要显示的图形的大小
	//bxsize buff宽度
	//vxsize vram宽度
	int x, y;
	for (y = 0; y < pysize; y++)
	{
		for (x = 0; x < pxsize; x++)
		{
			vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
		}
	}
	return;
}
