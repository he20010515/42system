//操作系统内核的入口文件
#include "bootpack.h"
#include "stdio.h"

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;

	char tempstr[64];
	char mouse[16 * 16];
	init_mouse_cursor8(mouse, COL8_008484);
	init_gdtidt();
	init_pic();
	io_sti();
	init_palette();
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

	io_out8(PIC0_IMR, 0xf9);
	io_out8(PIC1_IMR, 0xef);

	for (;;)
	{
		io_hlt();
	}
}
