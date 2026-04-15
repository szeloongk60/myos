#include "bootpack.h"
#include <stdio.h>
#include <string.h>
#include "lz4.h"

extern volatile int fdc_ready_flag;
void test_lz4_compression() {
    // 1. 准备原始数据 (例如一段重复率高的文本，压缩效果更好)
    const char* test_data ;
	readfile("MAKE.BAT",test_data);
	int src_size = sizeof(test_data); // 自动获取
   // int src_size = 50; // 这里的长度要包含 \0 或你指定的长度

    // 2. 准备压缩缓冲区
    // LZ4_compressBound 可以告诉你压缩这串数据最大需要多少空间
    char compressed_buffer[100]; 
    
    // 3. 执行压缩
    int compressed_size = LZ4_compress_default(
        test_data, 
        compressed_buffer, 
        src_size, 
        100
    );

    if (compressed_size <= 0) {
        putfonts8_ascAll(100,100,"Compression failed!");
        return;
    }
	createfile("ABC.TXT",  compressed_buffer);
	
    // 4. 立即进行解压测试 (自测循环)
    char verify_buffer[100];
    int decompressed_size = LZ4_decompress_safe(
        compressed_buffer, 
        verify_buffer, 
        compressed_size, 
        100
    );

    // 5. 验证结果
    if (decompressed_size == src_size) {
        // 比较原始数据和还原后的数据
        if (memcmp(test_data, verify_buffer, src_size) == 0) {
            putfonts8_ascAll(100,100,"Compression & Decompression: OK!");
        }
    }
}
void displayimage(unsigned char *buf) {
    extern struct BOOTINFO *binfo;
	extern struct SHEET*sht_mouse;
    char *magic = (char *)buf;
    unsigned int ori_size = *(unsigned int *)(buf + 4);
    unsigned int com_size = *(unsigned int *)(buf + 8);
    unsigned char *raw_buf;
    unsigned short width, height;
    unsigned char *src, *vram;
    int x, y, i, j; // 在函数开头声明所有变量，完美适配老编译器

    if (magic[0] != 'L' || magic[1] != 'Z') {
        return; // 如果不是 LZ4 格式直接退出
    }

    // 1. 动态申请内存 (memman)
    raw_buf = (unsigned char *)memget(ori_size);
    
    // 2. 解压 (跳过 12 字节头)
    LZ4_decompress_safe(buf + 12, raw_buf, com_size, ori_size);

    // 3. 从解压后的数据开头拿到宽和高
    width  = *(unsigned short *)(raw_buf + 0);
    height = *(unsigned short *)(raw_buf + 2);
	unsigned int *mouse=memget(width*height);
    // 4. 指针初始化 (跳过宽高占用的 4 字节)
    src = raw_buf + 4; 
    vram = (unsigned char *)binfo->vram;

    // 5. 设置显示坐标 (比如居中或显示在 100, 120)
    x = 0; 
    y = 0;
	unsigned char r ;
unsigned char g  ;
unsigned char b  ;
unsigned char a ; // 强制不透明

// 合成一个 32 位整数存入 buf
// 注意：如果屏幕显示颜色反了（红蓝颠倒），就把 R 和 B 的位置换一下

    // 6. 绘图循环
    for (j = 0; j < height; j++) {
        // 防止垂直越界
        if (y + j >= binfo->scrny) break; 
	
        for (i = 0; i < width; i++) {
            // 定位显存索引：永远是 * 4 (因为屏幕是 32位 BGRA)
            int v_idx = (y + j) * binfo->pitch + (x + i) * 4;

            // 定位素材索引：因为大侠删了 Alpha，所以是 * 3 (RGB)
            int img_idx = (j * width + i) * 3; 

            // 检查水平越界
            if (x + i >= binfo->scrnx) break;
			
            // 搬运像素 (从素材的 RGB 到显存的 BGRA)
            // 既然你没有 Alpha，我们就强行补一个 0xFF 让它不透明
				 r = src[img_idx + 0];
 g = src[img_idx + 1];
 b = src[img_idx + 2];
 a = 0xFF; // 强制不透明
           // vram[v_idx + 0] = src[img_idx + 2]; // B
           // vram[v_idx + 1] = src[img_idx + 1]; // G
            //vram[v_idx + 2] = src[img_idx + 0]; // R
           // vram[v_idx + 3] = 0xFF;             // A (强制不透明)
			mouse[j * width + i] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }

sheet_setbuf(sht_mouse, mouse, width, height, 0xFFffFF);
/////////////////////////
    
	
}


void dir()
{
	int x,y;
	char s[30];
	char c[10];
	int yy=16;
	putfonts8_ascAll(0,0,"test");
	struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
	for (x = 0; x < 224; x++) {
		if (finfo[x].name[0] == 0x00) {
			break;
		}
		if (finfo[x].name[0] != 0xe5) {
				if ((finfo[x].type & 0x18) == 0) {
						sprintf(s, "filename.ext %7d", finfo[x].size);
						sprintf(c, "%7d", finfo[x].clustno);
						for (y = 0; y < 8; y++) {
								s[y] = finfo[x].name[y];
						}
						s[ 9] = finfo[x].ext[0];
						s[10] = finfo[x].ext[1];
						s[11] = finfo[x].ext[2];
						s[12]="|";
						putfonts8_ascAll(0,yy,s);
						putfonts8_ascAll(200,yy,c);
						yy+=16;
				}
		}
	}
}
typedef enum {
    PUSH, POP,ADD, SUB, PRINT, HALT,JMP
} OpCode;

void execute_bf2(const char* code) {
	int x=150;
	int save[256];    // 虚拟机的数据栈
    int sp = 0;       // 栈指针 (Stack Pointer)
    int pc = 0;        // 程序计数器 (Program Counter)
   int running =1;
char s[10];
    while (running) {
        int opcode = code[pc]; // 1. 取指令
		
        switch (opcode) {           // 2. 解码并执行
            case PUSH:
                save[sp] =  code[pc+1]; 
				sp++;
				pc++;
                break;
			case POP:
				sp--;
				break;
            case ADD:
                save[sp]+= code[pc+1];
                pc++;
                break;
            case SUB:
                save[sp]-= code[pc+1];
                pc++;   
                break;
            case PRINT:
			sprintf(s,"%02X",save[sp]);
               putfonts8_ascAll(x, 100, s);
			   x += 8; 
                break;
            case HALT:
                running = 0;
                break;
			case JMP:
				pc= code[pc+1];
				pc++;
				break;
        }
		pc++;
    }
    return 0;
}



static unsigned char memory[1000] = {0}; 
void execute_bf(const char* code,struct TASK *task,char *keytable) {
     // 模拟 BF 的“纸带”
    unsigned char* ptr = memory;          // 数据指针
    const char* pc = code;                // 程序计数器
	int x=110;
	int y =60;
	char str1[20] = "C programming";
  char str2[20];
char count=0;
char c2=0;
  // copying str1 to str2
   char s[10];
				//	 sprintf(s,"%s",code);
				//	 putfonts8_ascAll(60,16,s);
// }
    while (*pc) {
        switch (*pc) {
			case '/':createfile("ABCD.TXT", ptr);break;
            case '>': ptr++; c2++;break;
            case '<': ptr--;c2--; break;
            case '+': (*ptr)++; break;
			case '=': (*ptr) += 10; break;
            case '-': (*ptr)--; break;
			case '_': (*ptr) -= 10; break;
			case '!': count=(*ptr); break;
			case '#':ptr = memory;break;
			case '@':
			sprintf(s,"%02X",c2);
			putfonts8_ascAll(x,y+16,s);
			break;
         case '.':putfonts8_ascAll(x, y, ptr);x += 8; break;
        //    case ',': *ptr = getchar(); break;
           case '[':
    if (count <= 0) { // 如果次数用完了，直接跳到结尾
        int depth = 1;
        while (depth > 0) {
            pc++;
            if (*pc == '[') depth++;
            else if (*pc == ']') depth--;
        }
    }
    // 如果 count > 0，什么都不做，pc 继续往后走进入循环体
    break;

case ']':
    if (count > 1) {  // 注意这里是 > 1，因为回到 [ 后还要跑最后一次
        count--;      // 跑完一轮，次数减 1
        
        // 执行跳转：找回匹配的 [
        int depth = 1;
        while (depth > 0) {
            pc--;
            if (*pc == ']') depth++;
            else if (*pc == '[') depth--;
        }
    } else {
        // count 为 1 或 0 时，说明次数跑够了，不回跳，直接结束
        count = 0; 
    }
    break;
            case ',': 
    for (;;) {
        if (fifo32_status(&task->fifo) == 0) {
            task_sleep(task);
            io_sti();
        } else {
            int i = fifo32_get(&task->fifo);
            io_sti();
            
            // 过滤掉不必要的按键数据（比如按键松开的断码）
            //if (i < 256) { 
                // 假设 keytable 是你的扫描码转 ASCII 表
                char ascii = keytable[i - 256]; 
                if (ascii != 0) {
                   *ptr = ascii; 
                    break;
                }
            
        }
    }
    break;
        default:
            // 遇到空格、换行或 rule110 里的注释字符，直接忽略
            break;
				
        }
        pc++;
    }
}

void myconsole()
{
	
	static char keytable0[0x80] = {
		0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0,   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
	};
	
	struct BOOTINFO *binfo2 = (struct BOOTINFO *) ADR_BOOTINFO;
	char  s[30],cmdline[30];
	int i;
	int fifobuf[128];
	int ii=0;
	int x,y;
	char buf[30];
	char buff[30];
	char bigbuf[512];
	x=0;
	y=16;
	struct TASK *task = task_now();
	fifo32_init(&task->fifo, 128, fifobuf, task);
	int i2=0;
	for (;;) {
		if (fifo32_status(&task->fifo) != 0)  {
			i = fifo32_get(&task->fifo);
			 if (i == 0x11c | i==0x1c) {
				// putfonts8_ascAll(x,y,"enter");
				 if (strcmp(cmdline, "DIR") == 0){
					 dir();
					 
					 char buu[10];
					 buu[0]=0x02;
					 buu[1]=0x45;
					 buu[2]=0x4;
					 buu[3]=0x5;
					// boxfill(0,0,0,900,900);
					 // execute_bf2(buu);
					  goto out;
					 //putfonts8_ascAll(x,y,"test123");
				 }
				 if(strcmp(cmdline,"R")==0){
					// readfile("RULE110.BF",bigbuf);
					// execute_bf("=![,.>]#/",task,keytable0);
					extern struct BOOTINFO *binfo;
					sprintf(s,"%d",binfo->scrnx);
			putfonts8_ascAll(90,90,s);
				
					  goto out;
				 }
				 if(strcmp(cmdline, "F") == 0){
					
					boxfill(0,0,0,900,900);
					char *img=memget(300000);
					readfile("cur.lz4",img);
					
					displayimage(img);
					//test_lz4_compression();
					 goto out;
					
				 }
				  if(strcmp(cmdline, "FA") == 0){
					 readfile("ABCD.TXT",buff);
					 char s[10];
					 sprintf(s,"%s",buff);
					 putfonts8_ascAll(60,100,s);
					   goto out;
					//fdc_ready_flag=0;
					  //io_out8(FDC_DOR, 0x00);   /* 使能控制器，选择驱动器0 */  
				 }
				  if(strncmp(cmdline, "READ ", 5) == 0){
					  int ii;
					  char s2[10];
					  char s3[10];
					  int t=0;
					  for(ii=3;cmdline[ii]!=0x0;ii++){
					  s2[t] = cmdline[ii];
					  t++;
					  }
					  
					  readfile(s2,buf);
				 
					 sprintf(s,"%s",buf);
					 putfonts8_ascAll(90,16,s);
					 
					 goto out;
				 }
				 out:
				 cmdline[0]=0x0;
				 cmdline[1]=0x0;
				 cmdline[2]=0x0;
				 cmdline[3]=0x0;
				 ii=0;
				 y+=16;
				 x=0;//enter
			 }else if(i == 0x10e){
				  x-=8;
				 boxfill(0,x,y,x+8,y+16);
				 ii--;
				
			 }else{//normal
			s[0] = keytable0[i - 256];
		//	s[0] = i - 256;
			s[1] = 0;
			cmdline[ii] = keytable0[i - 256];
			putfonts8_ascAll(x,y,s);
			//sprintf(s,"%02X",i);
			//putfonts8_ascAll(x,y+16,s);
			//putfonts8_asc(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s, 1);
			ii++;
			x+=8;
			 }//else enter
			
		}
	}
			
			
}


int cons_newline(int cursor_y, struct SHEET *sheet)
{
    int x, y;

    // ===== 可修改变量 =====
    int win_left   = 8;           // 文本区域左边界
    int win_top    = 28;          // 文本区域上边界
    int win_width  = 440;         // 文本区域宽度
    int win_height = 208;         // 文本区域高度（原来112）
    int line_h     = 16;          // 每行高度（原来16）

    if (cursor_y < win_top + win_height) {
        cursor_y += line_h;       // 换行
    } else {
        /* 滚动 */
        for (y = win_top; y < win_top + win_height; y++) {
            for (x = win_left; x < win_left + win_width; x++) {
                sheet->buf[x + y * sheet->bxsize] =
                    sheet->buf[x + (y + line_h) * sheet->bxsize];
            }
        }
        /* 清空最后一行 */
        for (y = win_top + win_height; y < win_top + win_height + line_h; y++) {
            for (x = win_left; x < win_left + win_width; x++) {
                sheet->buf[x + y * sheet->bxsize] = COL8_000000;
            }
        }
        sheet_refresh(sheet,
                      win_left,
                      win_top,
                      win_left + win_width,
                      win_top + win_height + line_h);
    }
    return cursor_y;
}