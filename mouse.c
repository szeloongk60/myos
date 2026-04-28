/* 鼠标控制代码 */

#include "bootpack.h"

struct FIFO32 *mousefifo;
int mousedata0;
void mouse_move()
{
	extern struct MOUSE_DEC mdec;
	extern struct SHEET*sht_mouse;
	extern struct SHEET*sht_win;
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	int fifobuf[128];
	int i=0;
	int mx,my;
	char buf[30];
	char buff[30];
	char bigbuf[512];
	
	struct TASK *task = task_now();
	fifo32_init(&task->fifo, 128, fifobuf, task);
	int i2=0;
	for (;;) {
	if (fifo32_status(&task->fifo) != 0)  {
			i = fifo32_get(&task->fifo);
	
				if (mouse_decode(&mdec, i - 512) != 0) {
					/* 已经收集了3字节的数据，移动光标 */
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) {
						mx = 0;
					}
					if (my < 0) {
						my = 0;
					}
					if (mx > binfo->scrnx - 1) {
						mx = binfo->scrnx - 1;
					}
					if (my > binfo->scrny - 1) {
						my = binfo->scrny - 1;
					}
					sheet_slide(sht_mouse, mx, my);/* 包含sheet_refresh含sheet_refresh */
					if ((mdec.btn & 0x01) != 0) { /* 按下左键、移动sht_win */
						sheet_slide(sht_win, mx - 8, my - 8);
					}
				}
	
	
	}
	}
				
}
void inthandler2c(int *esp)
/* 来自PS/2鼠标的中断 */
{
	int data;
	io_out8(PIC1_OCW2, 0x64); /* 把IRQ-12接收信号结束的信息通知给PIC1 */
	io_out8(PIC0_OCW2, 0x62); /* 把IRQ-02接收信号结束的信息通知给PIC0 */
	data = io_in8(PORT_KEYDAT);
	fifo32_put(mousefifo, data + mousedata0);
	return;
}

#define KEYCMD_SENDTO_MOUSE   0xd4
#define MOUSECMD_ENABLE     0xf4

void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec)
{
	/* 将FIFO缓冲区的信息保存到全局变量里 */
	mousefifo = fifo;
	mousedata0 = data0;
	/* 鼠标有效 */
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	/* 顺利的话，ACK(0xfa)会被发送*/
	mdec->phase = 0; /* 等待鼠标的0xfa的阶段*/
return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
	if (mdec->phase == 0) {
		/* 等待鼠标的0xfa的阶段 */
		if (dat == 0xfa) {
			mdec->phase = 1;
		}        
		return 0;
	}
	if (mdec->phase == 1) {
		/* 等待鼠标第一字节的阶段 */
		mdec->buf[0] = dat;
		mdec->phase = 2;
		return 0;
	}
	if (mdec->phase == 2) {
		/* 等待鼠标第二字节的阶段 */
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	if (mdec->phase == 3) {
		/* 等待鼠标第二字节的阶段 */
		mdec->buf[2] = dat;
		mdec->phase = 1;
		mdec->btn = mdec->buf[0] & 0x07;
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];
		if ((mdec->buf[0] & 0x10) != 0) {
			mdec->x |= 0xffffff00;
		}
		if ((mdec->buf[0] & 0x20) != 0) {
			mdec->y |= 0xffffff00;
		}      
		mdec->y = - mdec->y; /* 鼠标的y方向与画面符号相反 */  
		return 1;
	}
	/* 应该不可能到这里来 */
	return -1; 
}
