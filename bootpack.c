//操作系统内核的入口文件
#include "bootpack.h"
#include "stdio.h"

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *)0x0ff0;
	init_gdtidt();
	init_palette();
	char tempstr[64];
	char mouse[16 * 16];
	init_mouse_cursor8(mouse, COL8_008484);
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
	for (;;)
	{
		io_hlt();
	}
}
