/* bootpack */

#include "bootpack.h"
#include <stdio.h>
#include "lz4.h"
#define KEYCMD_LED		0xed
struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
struct SHEET*sht_mouse;
struct SHEET*sht_back;
struct SHEET*sht_win;
unsigned char *hzk_buf;
unsigned char *chat_buf;
struct MOUSE_DEC mdec;
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
struct TASK *task_a, *task_cons,*task_mouse;
struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHEET *sht_back,*sht_taskbar, *sht_cons;
	//struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	//unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_cons;
	//struct SHEET *sht_back, *sht_mouse, *sht_win, *sht_cons;
	//struct TASK *task_a, *task_cons;
	struct TIMER *timer;
unsigned int *my_mouse_mem;
unsigned int *win_mem;
unsigned int *TB_mem;

	int key_to = 0, key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;

	init_gdtidt();
	init_pic();
	io_sti(); /* IDT/PIC的初始化已经完成，于是开放CPU的中断 */
	 
	fifo32_init(&fifo, 128, fifobuf, 0);
	init_pit();
	init_keyboard(&fifo, 256);
	enable_mouse(&fifo, 512, &mdec);
	io_out8(PIC0_IMR, 0x98); /* 设定PIT和PIC1以及键盘为许可(11111000) f8 98=fl*/
	io_out8(PIC1_IMR, 0xa7); /* 开放鼠标中断(11101111) af硬盘*/
	
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
	task_cons = createTask(task_cons,&myconsole);
	task_mouse=createTask(task_mouse,&mouse_move);
	
	 FDinit();
	 init_ide_pci() ;
	 shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	 sht_back  = sheet_alloc(shtctl);
	 
	buf_back  = memman_alloc_4k(memman, (binfo->scrnx * binfo->scrny)*4);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); /* 无透明色 */
	sht_mouse = sheet_alloc(shtctl);
	sht_win=sheet_alloc(shtctl);
	win_mem=memman_alloc_4k(memman, (600 *600)*4);
	sheet_setbuf(sht_win,win_mem,600,600,0xffffff);
	
	sht_taskbar=sheet_alloc(shtctl);
	TB_mem=memman_alloc_4k(memman, (600 *600)*4);
	sheet_setbuf(sht_taskbar,TB_mem,600,600,0x0);
	 char *img=memget(300000);
	 int tmp;
	//io_sti();
	int test=0;
	if(test==1){
	tmp=readfile("1.lz4",img);
	hd_lba("1.lz4",img,tmp);
	}else{tmp=readfile_hd("1.lz4",img);}
	load_mx_img(img);
	//displayimage_sheet (img,sht_back,&win_mem) ;
	//memset(img, 0, 300000); // 将 img 指向的 300,000 字节全部设为 0
	if(test==1){
	tmp=readfile("cur.lz4",img);
	hd_lba("cur.lz4",img,tmp);
	}else{tmp=readfile_hd("cur.lz4",img);}
	displayimage_sheet (img,sht_mouse,&my_mouse_mem) ;
	hzk_buf=lz4read512k("HZK16.lz4");
	draw_sprite(sht_win, 0, 0, 704,0, 901,151,0);
	draw_sprite(sht_taskbar, 0, 0, 133,778, 417,813,0);
	//chat_buf=
	if(tmp>0){
	tmp=readfile("imgchat.dat",img);
	}
	showmsg(img, 25, sht_win, 16, 1);
	showmsg(img, 23, sht_win, 16, 30);
	showmsg(img, 24, sht_win, 16, 65);
	
	mx = (binfo->scrnx - 16) / 2;
my = (binfo->scrny - 16) / 2;

	sheet_slide(sht_back,  0,  0);
	sheet_slide(sht_taskbar,10,binfo->scrny-50);
	sheet_slide(sht_mouse, mx, my);
	sheet_slide(sht_win,0,0);
	sheet_updown(sht_back,  0);
	sheet_updown(sht_win, 1);
	sheet_updown(sht_mouse, 99);
	sheet_updown(sht_taskbar,2);
	sheet_refresh(sht_win, 0, 0, binfo->scrnx,binfo->scrny);
	//createfile("ABC.TXT", "ADD SHOW END");
	int data;
	
	
	sheet_refresh(sht_win, 0, 0, binfo->scrnx,binfo->scrny);
	init_e1000();
	init_e1000_tx();
	for (;;) {
	//	sheet_refresh(sht_back, 0, 0, binfo->scrnx,binfo->scrny);
	//sheet_refresh(sht_mouse, 0, 0, binfo->scrnx,binfo->scrny);
	check_packet();
		if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
			/*如果存在向键盘控制器发送的数据，则发送它 */
			keycmd_wait = fifo32_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}
		//io_cli();
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
						//fifo32_put(&task_cons->fifo, i);
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
				
					fifo32_put(&task_mouse->fifo, i);
					/* 已经收集了3字节的数据，移动光标 */
				
						//sheet_slide(sht_win, mx - 8, my - 8);
						goto out;
					
				
			}
		}
		out:
	}
	
}