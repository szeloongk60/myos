/* 关于绘图部分的处理 */

#include "bootpack.h"

void init_palette(void)
{
	static unsigned char table_rgb[16 * 3] = {
		0x00, 0x00, 0x00,	/*  0:黑 */
		0xff, 0x00, 0x00,	/*  1:梁红 */
		0x00, 0xff, 0x00,	/*  2:亮绿 */
		0xff, 0xff, 0x00,	/*  3:亮黄 */
		0x00, 0x00, 0xff,	/*  4:亮蓝 */
		0xff, 0x00, 0xff,	/*  5:亮紫 */
		0x00, 0xff, 0xff,	/*  6:浅亮蓝 */
		0xff, 0xff, 0xff,	/*  7:白 */
		0xc6, 0xc6, 0xc6,	/*  8:亮灰 */
		0x84, 0x00, 0x00,	/*  9:暗红 */
		0x00, 0x84, 0x00,	/* 10:暗绿 */
		0x84, 0x84, 0x00,	/* 11:暗黄 */
		0x00, 0x00, 0x84,	/* 12:暗青 */
		0x84, 0x00, 0x84,	/* 13:暗紫 */
		0x00, 0x84, 0x84,	/* 14:浅暗蓝 */
		0x84, 0x84, 0x84	/* 15:暗灰 */
	};
	set_palette(0, 15, table_rgb);
	return;

	/* C语言中的static char语句只能用于数据，相当于汇编中的DB指令 */
}

void set_palette(int start, int end, unsigned char *rgb)
{
	int i, eflags;
	eflags = io_load_eflags();	/* 记录中断许可标志的值 */
	io_cli(); 					/* 将中断许可标志置为0,禁止中断 */
	io_out8(0x03c8, start);
	for (i = start; i <= end; i++) {
		io_out8(0x03c9, rgb[0] / 4);
		io_out8(0x03c9, rgb[1] / 4);
		io_out8(0x03c9, rgb[2] / 4);
		rgb += 3;
	}
	io_store_eflags(eflags);	/* 复原中断许可标志 */
	return;
}

void boxfill8(unsigned int *vram, int pitch, int color, int x0, int y0, int x1, int y1)
{
    int x, y;
    unsigned int *p;

    for (y = y0; y <= y1; y++) {
        // 【核心修改】
        // 1. (char *)vram：先转成字节单位
        // 2. y * pitch：跳过 y 行的实际字节数（含对齐字节）
        // 3. x0 * 4：跳过左边 x0 个像素（每个像素 4 字节）
        p = (unsigned int *)((char *)vram + y * pitch + x0 * 4);
        
        for (x = x0; x <= x1; x++) {
            // 此时 p[0] 对应 (x0, y)，p[1] 对应 (x0+1, y)
            // 因为 p 是 unsigned int*，索引增加 1，地址自动增加 4 字节
            p[x - x0] = color;
        }
    }
    return;
}
int conver(int color)
{
		unsigned int palette32[16] = {
    0x000000, 0xFF0000, 0x00FF00, 0xFFFF00,
    0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF,
    0xC6C6C6, 0x840000, 0x008400, 0x848400,
    0x000084, 0x840084, 0x008484, 0x848484
};int final_color;

    // 如果传入的是 0-15 之间的索引，就去查表
    if ( color >= 0 &&  color <= 15) {
        return palette32[ color];
    } else {
        // 如果传入的是 0x1C2D42 这种真彩色，就直接用
       return  color;
    }
}
void boxfill(int c, int x0, int y0, int x1, int y1)
{
	extern struct BOOTINFO *binfo;

	boxfill8(binfo->vram,binfo->pitch, conver(c),x0,y0,x1,y1);
	return;
}
void init_screen8(char *vram, int x, int y)
{
	boxfill8(vram, x, COL8_008484,  0,     0,      x -  1, y - 29);
	boxfill8(vram, x, COL8_C6C6C6,  0,     y - 28, x -  1, y - 28);
	boxfill8(vram, x, COL8_FFFFFF,  0,     y - 27, x -  1, y - 27);
	boxfill8(vram, x, COL8_C6C6C6,  0,     y - 26, x -  1, y -  1);

	boxfill8(vram, x, COL8_FFFFFF,  3,     y - 24, 59,     y - 24);
	boxfill8(vram, x, COL8_FFFFFF,  2,     y - 24,  2,     y -  4);
	boxfill8(vram, x, COL8_848484,  3,     y -  4, 59,     y -  4);
	boxfill8(vram, x, COL8_848484, 59,     y - 23, 59,     y -  5);
	boxfill8(vram, x, COL8_000000,  2,     y -  3, 59,     y -  3);
	boxfill8(vram, x, COL8_000000, 60,     y - 24, 60,     y -  3);

	boxfill8(vram, x, COL8_848484, x - 47, y - 24, x -  4, y - 24);
	boxfill8(vram, x, COL8_848484, x - 47, y - 23, x - 47, y -  4);
	boxfill8(vram, x, COL8_FFFFFF, x - 47, y -  3, x -  4, y -  3);
	boxfill8(vram, x, COL8_FFFFFF, x -  3, y - 24, x -  3, y -  3);
	return;
}

void putfont8(int *vram, int xsize, int x, int y, int c, char *font,int st)
{
	int i;
    unsigned int *p; // 重点：由 char* 改为 unsigned int*
    char d;
    int cc = conver(c); // 确保你的 conver 函数返回的是 0xRRGGBB

    for (i = 0; i < 16; i++) {
        // 这里的 p 每次加 1 都会自动跳 4 字节
       // p = vram + (y + i) * xsize + x; 
	  // 
	  if(st== 1){
	  p = (unsigned int *)((char *)vram + (y + i) * xsize * 4 + x * 4);
	  }else{
		  p = (unsigned int *)((char *)vram + (y + i) * xsize + x * 4);
	  }
        d = font[i];
        
        // 现在的 p[0] 到 p[7] 对应横向的 8 个完整像素
        if ((d & 0x80) != 0) { p[0] = cc; }
        if ((d & 0x40) != 0) { p[1] = cc; }
        if ((d & 0x20) != 0) { p[2] = cc; }
        if ((d & 0x10) != 0) { p[3] = cc; }
        if ((d & 0x08) != 0) { p[4] = cc; }
        if ((d & 0x04) != 0) { p[5] = cc; } 
        if ((d & 0x02) != 0) { p[6] = cc; }
        if ((d & 0x01) != 0) { p[7] = cc; }
    }
}

void putfonts8_asc(int *vram, int xsize, int x, int y, int c, unsigned char *s)
{
	extern char hankaku[4096];
	/* C语言中，字符串都是以0x00结尾 */
	for (; *s != 0x00; s++) {
		putfont8(vram, xsize, x, y, c, hankaku + *s * 16,0);
		x += 8;
	}
	return;
}

void putfonts8_ascAll(int x, int y,  unsigned char *s)
{
	extern char hankaku[4096];
	struct BOOTINFO *binfo2 = (struct BOOTINFO *) ADR_BOOTINFO;
	/* C语言中，字符串都是以0x00结尾 */
	for (; *s != 0x00; s++) {
		putfont8(binfo2->vram, binfo2->pitch, x, y, 15, hankaku + *s * 16,0);
		x += 8;
	}
	return;
}
void putfonts8_sht(struct SHEET *sht,int x, int y,  unsigned char *s,int st)
{
	extern char hankaku[4096];
	//struct BOOTINFO *binfo2 = (struct BOOTINFO *) ADR_BOOTINFO;
	/* C语言中，字符串都是以0x00结尾 */
	for (; *s != 0x00; s++) {
		putfont8(sht->buf, sht->bxsize, x, y, 0xffffff, hankaku + *s * 16,st);
		x += 8;
	}
	return;
}

void init_mouse_cursor8(unsigned int *mouse, char bc)
/* 鼠标的数据准备（16x16） */
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
		".............***"
	};
	int x, y;

	

    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16; x++) {
            if (cursor[y][x] == '*') {
                // 黑色 (32位格式：0x000000)
                mouse[y * 16 + x] = 0x000000; 
            } else if (cursor[y][x] == 'O') {
                // 白色 (32位格式：0xFFFFFF)
                mouse[y * 16 + x] = 0xFFFFFF;
            } else if (cursor[y][x] == '.') {
                // 透明色：在大侠现在的 32 位系统里，
                // 如果没有 Sheet，这里只能先填入背景色 bc
                // 或者填入你约定的“抠图色”，比如 0xFF00FF
                mouse[y * 16 + x] = 0xFF00FF;
			}
		}
	}
	return;
}

void putblock8_8(char *vram, int vxsize, int pxsize,
	int pysize, int px0, int py0, char *buf, int bxsize)
{
	int x, y;
	for (y = 0; y < pysize; y++) {
		for (x = 0; x < pxsize; x++) {
			vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
		}
	}
	return;
}
void displayimage_sheet (unsigned char *lz4,struct SHEET* sheet,unsigned int **outbuf) {
    extern struct BOOTINFO *binfo;
    char *magic = (char *)lz4;
    unsigned int ori_size = *(unsigned int *)(lz4 + 4);
    unsigned int com_size = *(unsigned int *)(lz4 + 8);
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
    LZ4_decompress_safe(lz4 + 12, raw_buf, com_size, ori_size);

    // 3. 从解压后的数据开头拿到宽和高
    width  = *(unsigned short *)(raw_buf + 0);
    height = *(unsigned short *)(raw_buf + 2);
	
	unsigned int *new_mem = (unsigned int *)memget(width * height *4);

    // 5. 将申请好的地址赋值给传入的指针（把地址“带出去”）
    *outbuf = new_mem;
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
			new_mem[j * width + i] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }

sheet_setbuf(sheet, new_mem, width, height, 0xFFffFF);
/////////////////////////
    
	
}

void putfonts8_chinese(struct SHEET *sht, int x, int y,  unsigned char *gbk_str) {
    int i, j, k;
    unsigned char *font;
    unsigned char d;
    int qm, wm;
    unsigned long offset;
	 extern unsigned char *hzk_buf;

    // 获取区位码：GBK高位是区，低位是位
    // 算法：区码 = 第一字节 - 0xA0, 位码 = 第二字节 - 0xA0
    qm = gbk_str[0] - 0xa0;
    wm = gbk_str[1] - 0xa0;

    // 计算在 HZK16 中的偏移量
    // 每个区有 94 个位，每个字占 32 字节
    offset = ((qm - 1) * 94 + (wm - 1)) * 32;
    font = hzk_buf + offset;

    for (i = 0; i < 16; i++) { // 16行点阵
        // 每一行占 2 字节 (16位)
        for (k = 0; k < 2; k++) {
            d = font[i * 2 + k]; 
            for (j = 0; j < 8; j++) {
                // 检查每一位是否为 1
                if ((d & (0x80 >> j)) != 0) {
                    // 在指定位置画点。注意检查越界情况防止死机
                    if (x + k * 8 + j < sht->bxsize && y + i < sht->bysize) {
                        sht->buf[(y + i) * sht->bxsize + (x + k * 8 + j)] = 0xffffff;
                    }
                }
            }
        }
    }
}

void showmsg(char *file_buf, int index, struct SHEET *sht, int x, int y) {
    // 1. 获取总数（前4字节）
    unsigned int count = *((unsigned int *)file_buf);
    
    if (index < 1 || index > count) return;

    // 2. 获取对应的偏移量
    // 偏移量表从第4字节开始，每个占4字节
    unsigned int *offset_table = (unsigned int *)(file_buf + 4);
    unsigned int my_offset = offset_table[index - 1];

    // 3. 找到字符串并打印
    char *msg_ptr = file_buf + my_offset;
	int cur_x = x;
	int i;
    for ( i = 0; msg_ptr[i] != 0x00; ) {
        if ((unsigned char)msg_ptr[i] > 0x80) {
            // 是中文，调用你原本画单个字的函数
            // 注意：这里传的是 &msg_ptr[i]
            putfonts8_chinese(sht, cur_x, y, &msg_ptr[i]); 
            
            cur_x += 16; // 坐标右移一个汉字的宽度
            i += 2;      // 指针跳过 GBK 的 2 个字节
        } else {
            // 是英文/半角符号
            //putfonts8_asc(sht, cur_x, y, 15, &msg_ptr[i]);
            cur_x += 8;  // 坐标右移一个英文宽度
            i += 1;      // 指针跳过 1 个字节
        }
    }
    // -------------------------------
   // putfonts8_chinese(sht, x, y, msg_ptr);
}