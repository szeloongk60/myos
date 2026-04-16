/* bootpack */

#include "bootpack.h"
#include <stdio.h>
#include "lz4.h"
#define KEYCMD_LED		0xed
struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
struct SHEET*sht_mouse;
struct SHEET*sht_back;
unsigned char *hzk_buf;
unsigned char *chat_buf;
void HariMain(void)
{
	////////////////////////
	
	////////////////////
	
	struct SHTCTL *shtctl;
	char s[40];
	char ss[40];
	struct FIFO32 fifo, keycmd;
	int fifobuf[128], keycmd_buf[32];
	int mx, my, i, cursor_x, cursor_c;
	unsigned int memtotal;
	struct MOUSE_DEC mdec;
	struct FIFO32 fifo2;
	unsigned int *buf_back,buf_mouse[256];
struct TASK *task_a, *task_cons;
struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHEET *sht_back,*sht_win, *sht_cons;
	//struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	//unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_cons;
	//struct SHEET *sht_back, *sht_mouse, *sht_win, *sht_cons;
	//struct TASK *task_a, *task_cons;
	struct TIMER *timer;
unsigned int *my_mouse_mem;
unsigned int *win_mem;

	int key_to = 0, key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;

	init_gdtidt();
	init_pic();
	io_sti(); /* IDT/PIC的初始化已经完成，于是开放CPU的中断 */
	fifo32_init(&fifo, 128, fifobuf, 0);
	init_pit();
	init_keyboard(&fifo, 256);
	enable_mouse(&fifo, 512, &mdec);
	io_out8(PIC0_IMR, 0x98); /* 设定PIT和PIC1以及键盘为许可(11111000) f8 98=fl*/
	io_out8(PIC1_IMR, 0xef); /* 开放鼠标中断(11101111) */
	boxfill(0,0,0,binfo->scrnx,binfo->scrny);
putfonts8_asc(binfo->vram,binfo->pitch,0,0,0xff0000,"R");
	putfonts8_asc(binfo->vram,binfo->pitch,50,0,0x00ff00,"G");
	putfonts8_asc(binfo->vram,binfo->pitch,90,0,0x0000ff,"B");
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free( 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	memman_free( 0x00400000, memtotal - 0x00400000);
	//init_palette();
	//init_palette();
task_a = task_init(memman);
	fifo.task = task_a;
	task_run(task_a, 1, 2);
//	timer = timer_alloc();
	//timer_init(timer, &fifo, 1);
	//timer_settime(timer, 50);
	/*为了避免和键盘当前状态冲突，在一开始先进行设置*/
	fifo32_put(&keycmd, KEYCMD_LED);
	fifo32_put(&keycmd, key_leds);
	task_cons = task_alloc();
	task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
	task_cons->tss.eip = (int) &myconsole;
	task_cons->tss.es = 1 * 8;
	task_cons->tss.cs = 2 * 8;
	task_cons->tss.ss = 1 * 8;
	task_cons->tss.ds = 1 * 8;
	task_cons->tss.fs = 1 * 8;
	task_cons->tss.gs = 1 * 8;
	//*((int *) (task_cons->tss.esp + 4)) = (int)fifo2;
	//*((int *) (task_cons->tss.esp + 8)) = memtotal;
	task_run(task_cons, 1, 2); /* level=2, priority=2 */
	
	 FDinit();
	 shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	 sht_back  = sheet_alloc(shtctl);
	 
	buf_back  = memman_alloc_4k(memman, (binfo->scrnx * binfo->scrny)*4);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); /* 无透明色 */
	sht_mouse = sheet_alloc(shtctl);
	sht_win=sheet_alloc(shtctl);
	win_mem=memman_alloc_4k(memman, (600 *600)*4);
	sheet_setbuf(sht_win,win_mem,600,600,0xffffff);
	 char *img=memget(300000);
	 int tmp;
	 
	tmp=readfile("1.lz4",img);
	
	load_mx_img(img);
	//displayimage_sheet (img,sht_back,&win_mem) ;
	
	tmp=readfile("cur.lz4",img);
	displayimage_sheet (img,sht_mouse,&my_mouse_mem) ;
	hzk_buf=lz4read512k("HZK16.lz4");
	draw_sprite(sht_win, 0, 0, 704,0, 901,151,0);
	//chat_buf=
	tmp=readfile("imgchat.dat",img);
	showmsg(img, 25, sht_win, 16, 1);
	showmsg(img, 23, sht_win, 16, 30);
	showmsg(img, 24, sht_win, 16, 65);
	unsigned char *msg = lz4read512k("msotxt.LZ4");
	if (msg != 0) { 
    int i = 0;
    int cur_x = 100;
    int y = 0;
	char txt[2];
    // 4. 必须写循环！TXT 是 GBK 编码，每字跳 2 字节
    while (msg[i] != 0x00) {
        // 调用单字渲染
		if( msg[i]== 0x0d&&msg[i+1]==0x0a)
			{
				i=i+2;
				cur_x=100;
				y+=24;
				goto tout;
			}
		if(msg[i] ==0x20){
			i++;
			goto tout;
			}
		if ((unsigned char)msg[i] > 0x80) {
        putfonts8_chinese(sht_back, cur_x,y, &msg[i]);
        
        cur_x += 16; // 坐标右移
		
			 i += 2;      // 指针跳过 GBK 的 2 个字节
		}else {
			
        // 【英文/数字】
        // 这里调你原本显示 ASCII 的函数，比如 putfonts8_asc
		//(int *vram, int xsize, int x, int y, int c, unsigned char *s)
		txt[0]=msg[i];
		txt[1]=0x0;
		//sprintf(txt,"%s",&msg[i]);
		//putfonts8_ascAll(100, y, &msg[i]);
		putfonts8_sht(sht_back ,cur_x, y, txt,1);
       // putfonts8_asc(sht_back,sht_back->bxsize ,cur_x, y, 0xFFFF00, "a");
        cur_x += 8;  // 英文宽 8
        i += 1;      // 英文只跳 1 字节
		}
		if(cur_x>binfo->scrnx-100){
			y+=24;
			cur_x=100;
			}
			tout:
       
    }
	}
	mx = (binfo->scrnx - 16) / 2;
my = (binfo->scrny - 16) / 2;

	sheet_slide(sht_back,  0,  0);
	sheet_slide(sht_mouse, mx, my);
	sheet_slide(sht_win,0,0);
	sheet_updown(sht_back,  0);
	sheet_updown(sht_win, 1);
	sheet_updown(sht_mouse, 2);
	sheet_refresh(sht_win, 0, 0, binfo->scrnx,binfo->scrny);
	//createfile("ABC.TXT", "ADD SHOW END");
	int data;
	
	for (;;) {
	//	sheet_refresh(sht_back, 0, 0, binfo->scrnx,binfo->scrny);
	//sheet_refresh(sht_mouse, 0, 0, binfo->scrnx,binfo->scrny);
		if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
			/*如果存在向键盘控制器发送的数据，则发送它 */
			keycmd_wait = fifo32_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}
		io_cli();
		if (fifo32_status(&fifo) == 0) {
			io_sti();
		} else {
			i = fifo32_get(&fifo);
			io_sti();
			///////////////////
/*			data = io_in8(0x3f4);
			sprintf(ss,"%02X",data);
boxfill8(binfo->vram,binfo->scrnx,0,16,1,100,100);
	putfonts8_asc(binfo->vram,binfo->scrnx,20,20,15,ss);*/
	//////
			if (256 <= i && i <= 511) { /* 键盘数据*/
				if (i < 0x80 + 256) { /*将按键编码转换为字符编码*/
					//	boxfill8(binfo->vram,binfo->scrnx,0,0,0,100,100);
						fifo32_put(&task_cons->fifo, i);
						goto out;
				}
			}
			if (i == 256 + 0xfa) { /*键盘成功接收到数据*/
					keycmd_wait = -1;
					goto out;
				}
			if (i == 256 + 0xfe) { /*键盘没有成功接收到数据*/
				wait_KBC_sendready();
				io_out8(PORT_KEYDAT, keycmd_wait);
				goto out;
			}
			 if (512 <= i && i <= 767) { /* 鼠标数据*/
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
						goto out;
					}
				}
			}
		}
		out:
	}
	
}