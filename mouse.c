#include "bootpack.h"

struct FIFO32 *mousefifo;
int mousedata0;

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

void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *dec)
{
	mousefifo = fifo;
	mousedata0 = data0;
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	dec->phase = 0;
	return;
}

void inthandler2c(int *esp)
{
	unsigned char data;
	io_out8(PIC1_OCW2, 0x64);
	io_out8(PIC0_OCW2, 0x62); //中断受理完毕
	data = io_in8(PORT_KEYDAT);
	fifo32_put(mousefifo, data + mousedata0);
	return;
}
