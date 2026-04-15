#include "bootpack.h"
#include <stdio.h>
#include <string.h>
extern volatile int fdc_ready_flag;

void read_dma(void)
{
	
    io_out8(DMA_MASK_REG, 0x06);   /* 0x06 = 屏蔽通道2 (位2=1) */
    
    /* 2. 清除 DMA 触发标志 */
    io_out8(DMA_CLEAR_REG, 0x00);
    
    /* 3. 设置 DMA 模式 */
    /* 0x46 = 单通道模式, 读操作, 地址递增, 通道2 */
    io_out8(DMA_MODE_REG, 0x46);
    
    /* 4. 设置缓冲区地址（例如 0x8000）*/
    unsigned int buf_addr = 0x8000;
    io_out8(DMA_ADDR_CH2, buf_addr & 0xFF);         /* 低8位 */
    io_out8(DMA_ADDR_CH2, (buf_addr >> 8) & 0xFF);  /* 高8位 */
    io_out8(DMA_PAGE_CH2, (buf_addr >> 16) & 0xFF); /* 页面 */
    
    /* 5. 设置传输字节数（512字节 - 1）*/
    unsigned int size = 512;
    io_out8(DMA_COUNT_CH2, (size - 1) & 0xFF);      /* 低8位 */
    io_out8(DMA_COUNT_CH2, ((size - 1) >> 8) & 0xFF); /* 高8位 */
    
    /* 6. 解除 DMA 通道2的屏蔽 */
    io_out8(DMA_MASK_REG, 0x02);   /* 解除屏蔽通道2 */
}

void fdc_read(char *tobuf,int clust) {
	read_dma();
	int i;
	if(clust ==0){
		 putfonts8_ascAll(60,16,"o");
		 return;
	}
    static int c,h,s; // 静态数组，返回后依然有效
    int sector_number = 1 + 2*9 + 14 + (clust - 2); // reserved + 2*FAT + root + (cluster-2)
    c = sector_number / (2 * 18);          // C
    h = (sector_number / 18) % 2;          // H
   s = (sector_number % 18) + 1;    
   

    // 发 READ DATA 命令
  fdc_write_cmd(0xE6);
 fdc_write_cmd((h << 2) | 0x00);//floppy no
fdc_write_cmd(c);//2
fdc_write_cmd(h);//3
fdc_write_cmd(s);//4
fdc_write_cmd(2);//5
fdc_write_cmd(18);   // EOT
fdc_write_cmd(0x1B); // GAP3
fdc_write_cmd(0xFF); // DTL
while(!fdc_ready_flag);
putfonts8_ascAll(110,110,"read");
wait_RW_done();
char *tmp = (volatile char*)0x8000;
volatile char *src = (volatile char*)0x8000;
for (i = 0; i < 512; i++) {
    tobuf[i] = tmp[i]; // 循环拷贝整个扇区
}
	
}
void fdc_read_lba(char *tobuf,int lba) {
	read_dma();
	int i;
	
    static int c,h,s; // 静态数组，返回后依然有效
    c = lba / (2 * 18);
    h = (lba / 18) % 2;
    s = (lba % 18) + 1; // 物理扇区从 1 开始   
   

    // 发 READ DATA 命令
  fdc_write_cmd(0xE6);
 fdc_write_cmd((h << 2) | 0x00);//floppy no
fdc_write_cmd(c);//2
fdc_write_cmd(h);//3
fdc_write_cmd(s);//4
fdc_write_cmd(2);//5
fdc_write_cmd(18);   // EOT
fdc_write_cmd(0x1B); // GAP3
fdc_write_cmd(0xFF); // DTL
while(!fdc_ready_flag);
putfonts8_ascAll(110,110,"read");
wait_RW_done();
char *tmp = (volatile char*)0x8000;
volatile char *src = (volatile char*)0x8000;
for (i = 0; i < 512; i++) {
    tobuf[i] = tmp[i]; // 循环拷贝整个扇区
}
	
}
void readfile2(const char *filename,char *buf)
{
	char *p = buf;
	int c=findfile( filename);
	 fdc_read( p,c);
}
int readfile(const char *filename, char *buf)
{
	fdc_read_lba((unsigned char *) (ADR_DISKIMG + 0x002600),19);
struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
unsigned char *diskimg = (unsigned char *) ADR_DISKIMG; 
struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
fdc_read_lba((unsigned char *) (ADR_DISKIMG + 0x000200),1);
	int *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	int *savefat = (int *) memman_alloc_4k(memman, 4 * 2880);
file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200)); 

    // 1. 寻找文件，获取起始簇号
    int fi = findfile(filename);
	int c=finfo[fi].clustno;
    
    // 如果找不到文件，c 应该是 0 或者 -1（取决于你的 findfile 实现）
    if (c <= 0) {
        return 0; 
    }

    char *p = buf;
    
    // 2. 循环读取，直到遇到 FAT12 的结束标志
    // FAT12 结束标志通常大于等于 0x0ff8
    while (c < 0x0ff8 && c > 1) {
        
        // 读取当前簇（扇区）到当前指针 p 处
        fdc_read(p, c);
        
        // 3. 指针向后移动一个扇区的大小
        p += 512;
        
        // 4. 查 FAT 表，获取下一个簇号
        // 注意：这里的 fat 数组必须是全局变量或者你能访问到的地址
        c = fat[c]; 
    }
	return finfo[fi].size;
}
void fdc_write_dma()
{
	/* 1. 屏蔽通道2 */
    io_out8(DMA_MASK_REG, 0x06); 
    
    /* 2. 清除/重置触发器 (Flip-Flop) */
    io_out8(DMA_CLEAR_REG, 0x00);
    
    /* 3. 设置 DMA 模式 */
    /* 修改点：0x46 是从软盘读入内存。写入软盘需要 0x4A */
    /* 0x4A 详解: 
       位 7-6: 01 (单字节传输模式 Single Mode)
       位 5  : 0  (地址递增)
       位 4  : 0  (禁止自动初始化)
       位 3-2: 10 (读传输 - 从内存读出，发送到设备)
       位 1-0: 10 (通道2)
    */
    io_out8(DMA_MODE_REG, 0x4A); 
    
    /* 4. 设置缓冲区地址 */
    unsigned int buf_addr = 0x8000;
    io_out8(DMA_ADDR_CH2, (unsigned char)(buf_addr & 0xFF));         /* 偏移低8位 */
    io_out8(DMA_ADDR_CH2, (unsigned char)((buf_addr >> 8) & 0xFF));  /* 偏移高8位 */
    io_out8(DMA_PAGE_CH2, (unsigned char)((buf_addr >> 16) & 0xFF)); /* 页面寄存器 (64KB段) */
    
    /* 5. 设置传输字节数 (Count) */
    unsigned int size = 512;
    io_out8(DMA_COUNT_CH2, (unsigned char)((size - 1) & 0xFF));      /* 计数低8位 */
    io_out8(DMA_COUNT_CH2, (unsigned char)(((size - 1) >> 8) & 0xFF));/* 计数高8位 */
    
    /* 6. 解除屏蔽通道2 */
    io_out8(DMA_MASK_REG, 0x02);
}
void fdc_write(char *buf,int clust) {
	
	fdc_write_dma();
int i;
    static int c,h,s; // 静态数组，返回后依然有效
    int sector_number = 1 + 2*9 + 14 + (clust - 2); // reserved + 2*FAT + root + (cluster-2)
    c = sector_number / (2 * 18);          // C
    h = (sector_number / 18) % 2;          // H
   s = (sector_number % 18) + 1;          // R
char *tmp=(volatile char*)0x8000;
char sector_buf[512]; // 准备一个标准的软盘簇缓冲区

    // 全部刷 0
    memset(sector_buf, 0, 512);
	strncpy(sector_buf, buf, 512);
memcpy((void *)tmp, sector_buf, 512);

/*int size=sizeof(buf);
for(i = 0; i < size+1; i++) {
        tmp[i] =buf[i];  // 或者 *(a + i) = buf[i];
    }
*/
    // 发 write DATA 命令
 fdc_write_cmd(0xc5);
 fdc_write_cmd((h << 2) | 0x00);//floppy no
fdc_write_cmd(c);//2
fdc_write_cmd(h);//3
fdc_write_cmd(s);//4
fdc_write_cmd(2);//fix
fdc_write_cmd(18);   // EOT
fdc_write_cmd(0x1B); // GAP3
fdc_write_cmd(0xFF); // DTL
while(!fdc_ready_flag);
	putfonts8_ascAll(110,110,"write");
	 wait_RW_done();
	
}
void floppy_write_lba(int lba, char *buf) {
	fdc_write_dma();
    int c, h, s;
    
    // 1. LBA 转 CHS
    c = lba / (2 * 18);
    h = (lba / 18) % 2;
    s = (lba % 18) + 1; // 物理扇区从 1 开始

    // 2. 搬运数据到 DMA 缓冲区 (假设 0x8000)
    // 必须用 memcpy，不要用 strncpy，因为 FAT 和目录区有很多 0x00
    memcpy((void *)0x8000, buf, 512);

    // 3. 执行那 9 行死命令
    fdc_write_cmd(0xC5);
    fdc_write_cmd((h << 2) | 0); 
    fdc_write_cmd(c);
    fdc_write_cmd(h);
    fdc_write_cmd(s);
    fdc_write_cmd(2);    // 512 字节
    fdc_write_cmd(18);   // EOT
    fdc_write_cmd(0x1B); // GAP3
    fdc_write_cmd(0xFF); // DTL
	while(!fdc_ready_flag);
	putfonts8_ascAll(110,130,"LBA");
	 wait_RW_done();
   
}
void fdc_write_cmd(unsigned char data)
{
    while ((io_in8(0x3F4) & 0x80) == 0); // 等RQM=1
    io_out8(0x3F5, data);
}


void wait_RW_done(void) {
    int i;
    unsigned char msr;
unsigned char fdc_results[7];
    for (i = 0; i < 7; i++) {
        /* 1. 等待 RQM (Bit 7) 置 1，表示数据准备好了 */
        /* 同时检查 DIO (Bit 6) 必须为 1，表示方向是从 FDC 到 CPU */
        while (1) {
            msr = io_in8(0x3F4); // 读取主状态寄存器
            
            // 我们需要 RQM=1 (0x80) 且 DIO=1 (0x40)
            if ((msr & 0xC0) == 0xC0) {
                break; 
            }
            
            // 容错处理：如果 CB (Bit 4) 突然变成了 0，说明命令异常结束了
            if (!(msr & 0x10) && i > 0) {
                // 这里可以做个日志：FDC 提前退出了结果相位
                return;
            }
            
            // 这里可以加一个很小的延迟（如 io_delay()），防止 CPU 跑得太快
        }

        /* 2. 从数据端口读取一个结果字节 */
        fdc_results[i] = io_in8(0x3F5);
    }

    /* 3. 最后确认：此时 MSR 的 CB (Bit 4) 应该变回 0 了 */
    // 如果 CB 还是 1，说明 FDC 可能还有更多话要说，或者出错了
    while (io_in8(0x3F4) & 0x10); 
}
void FDinit()
{
	volatile int i;
	
	fdc_ready_flag=0;
					//fdc_write_cmd( 0x08);
					io_out8(0x3F2, 0x1C); // 开启马达
				for ( i = 0; i < 1000; i++) {
        // 循环体可以为空
    }
					io_out8(0x3F5, 0x03); 
					io_out8( 0x3F5, 0x26 ); 
					io_out8(0x3F5, 0x10) ;
					io_out8(FDC_DOR, 0x1c);   /* 使能控制器，选择驱动器0 */
					 
					//for (i2=0;i2<1000;i2++); // 等待模拟器内部处理
					fdc_write_cmd(0x07);
					fdc_write_cmd(0x00);
					while(!fdc_ready_flag);
					do08();
					 fdc_ready_flag =0;
					io_out8(0x3f5,0x0f);
					io_out8(0x3f5,0x00);
					io_out8(0x3f5,0x00);
					while(!fdc_ready_flag);
					do08();
					boxfill(2,100,16,150,64);
}

void inthandler26(int *esp)

{
	
	struct FIFO32 *keyfifo;
	
	 // while ((io_in8(0x3F4) & 0xC0) != 0xC0);

    unsigned char st0 = io_in8(0x3F5);
    unsigned char cyl = io_in8(0x3F5);

    fdc_ready_flag = 1;

   io_out8(PIC0_OCW2, 0x66);
	// fifo32_put(keyfifo, 0x10);
	putfonts8_ascAll(100,16,"int 26");
	return;
}
void do08(){
int i;
unsigned char st0;

for (i = 0; i < 4; i++) {
    // --- 第一步：等 FDC 准备好收命令 ---
    while ((io_in8(0x03f4) & 0x80) == 0); // 盯着 0x3f4 的第 7 位，直到它变成 1
    
    // --- 第二步：发 0x08 指令 ---
    io_out8(0x03f5, 0x08); 

    // --- 第三步：等 FDC 准备好吐数据 (ST0) ---
    while ((io_in8(0x03f4) & 0x80) == 0); // 再次等待 RQM 位
    st0 = io_in8(0x03f5);                 // 读出 ST0

    // --- 第四步：等 FDC 准备好吐数据 (PCN) ---
    while ((io_in8(0x03f4) & 0x80) == 0); 
    st0 = io_in8(0x03f5);                 // 读出 PCN
}
}