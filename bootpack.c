//操作系统内核的入口文件
#include "bootpack.h"
#include "stdio.h"
extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

#define PORT_KEYDAT 0x0060
#define PORT_KEYSTA 0x0064
#define PORT_KEYCMD 0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE 0x60
#define KBC_MODE 0x47
#define KEYCMD_SENDTO_MOUSE 0xd4
#define MOUSECMD_ENABLE 0xf4

void wait_KBC_sendready(void)
{
	//等待键盘控制器电路准备完毕
	for (;;)
	{
		if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0)
		{
			break;
		}
	}
	return;
}

void init_keyboard(void)
{
	//初始化键盘控制器电路
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}
void enable_mouse(void)
{
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	return;
}

struct MOUSE_DEC
{
	unsigned char buf[3], phase;
	int x, y, btn;
};
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data)
{
	switch (mdec->phase)
	{
	case 0:
		if (data == 0xfa)
		{
			mdec->phase = 1;
		}
		return 0;
	case 1:
		if ((data & 0xc8) == 0x08) //如果第一个字节正确
		{
			mdec->buf[0] = data;
			mdec->phase = 2;
			return 0;
		}
	case 2:
		mdec->buf[1] = data;
		mdec->phase = 3;
		return 0;
	case 3:
		mdec->buf[2] = data;
		mdec->phase = 1;
		mdec->btn = mdec->buf[0] & 0x07; //0b00000111 取出低三位的值
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];

		if ((mdec->buf[0] & 0x10) != 0)
		{
			mdec->x |= 0xffffff00;
		}
		if ((mdec->buf[0] & 0x20) != 0)
		{
			mdec->y |= 0xffffff00;
		}
		mdec->y = -mdec->y;
		return 1;
	default:
		return -1;
	}
}
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
