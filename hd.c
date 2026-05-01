// 定义硬盘端口
#define ATA_DATA        0x1F0
#define ATA_SECTOR_CNT  0x1F2
#define ATA_LBA_LOW     0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HIGH    0x1F5
#define ATA_DRIVE_SEL   0x1F6
#define ATA_COMMAND     0x1F7
#define ATA_STATUS      0x1F7
#include "bootpack.h"
struct HDINFO {
	unsigned char name[8], ext[3], type;
	unsigned int reserve[10];
	unsigned int size;
};
//uint8_t bitmap[1024 / 8];  

int is_free(unsigned char *bitmap,int i) {
    return !(bitmap[i / 8] & (1 << (i % 8)));
}

void set_used(unsigned char *bitmap,int i) {
    bitmap[i / 8] |= (1 << (i % 8));
}

void set_free(unsigned char *bitmap,int i) {
    bitmap[i / 8] &= ~(1 << (i % 8));
}

// ------------------ 分配block ------------------
int alloc_block(unsigned char *bitmap) {
	int i;
    for ( i = 0; i < 5120; i++) {
        if (is_free(bitmap,i)) {
            set_used(bitmap,i);
            return i+3;
        }
    }
    return -1;
}
struct HDINFO *hdf;
unsigned char *bitmap;
char bitread;
void readbit()
{
	if(bitread == 0){
		
		hdf=memget(900*32);
		bitmap=memget(2097152/8);
		ide_dma_read(1,1,bitmap);
		ide_dma_read(2,1,hdf);
		bitread=1;
	
	}
	
}

int findfile_hd(const char *filename)
{
	readbit();
	 char s[11];
    int x, y;

    // --- 第一步：把 "make.bat" 转成 "MAKE    BAT" ---
    for (y = 0; y < 11; y++) s[y] = ' '; // 填充空格
    y = 0;
    for (x = 0; y < 11 && filename[x] != 0; x++) {
        if (filename[x] == '.' && y <= 8) {
            y = 8; // 强制跳到扩展名位置
        } else {
            char c = filename[x];
            if ('a' <= c && c <= 'z') c -= 0x20; // 转大写
            s[y] = c;
            y++;
        }
    }

    // --- 第二步：在磁盘根目录里找这个名字 ---
    for (x = 0; x < 224; x++) {
        if (hdf[x].name[0] == 0x00) break; // 目录结束
        if (hdf[x].name[0] == 0xe5) continue; // 已删除文件
        
        // 忽略卷标和非普通文件
        if ((hdf[x].type & 0x18) != 0) continue; 

        // 比较 11 字节
        int match = 1;
        for (y = 0; y < 11; y++) {
            if (hdf[x].name[y] != s[y]) {
                match = 0;
                break;
            }
        }
        
        if (match) return x; //finfo[x].clustno; // 找到了！返回簇号
    }

    return 0; // 没找到
	
}
int readfile_hd(const char *filename, char *buf)
{
	int x= findfile_hd(filename);
	int i;
	if(hdf[x].reserve[0] ==0){return 0;}
	 char *p = buf;
	int st=hdf[x].reserve[0];
	int en=hdf[x].reserve[1];
	for(i=st;i<en;i++)
	{
	//	void ide_dma_read(unsigned int lba, unsigned char count, char *buf)
		ide_dma_read(i,1,p);
		p+=512;
	}
	
}
void hd_lba(char *filename,unsigned char *buf,int sizee)
{
	
	int i;
	int j,x;
	readbit();
	volatile int size = 0;
	  for (x = 0; x < 224; x++) {
        if (hdf[x].name[0] == 0x00) break; 
	  }
	  
  
	if(sizee>0){
		size=sizee;
	}else if(sizee==0){
		  while (buf[size] != 0){ size++;}
	}
	for (j = 0; j < 8; j++) hdf[x].name[j] = ' ';
            for (j = 0; j < 3; j++) hdf[x].ext[j] = ' ';

            char *p = filename;
            for (j = 0; j < 8 && *p && *p != '.'; j++) hdf[x].name[j] = *p++;
            if (*p == '.') p++;
            for (j = 0; j < 3 && *p; j++) hdf[x].ext[j] = *p++;
			
           hdf[x].type = 0x20;      // 归档文件
		   int blocks = (size + 511) / 512;
		
		   int tmp=0;
		   int offset = 0;
		    while (hdf[x].reserve[tmp] != 0) tmp++;
		   hdf[x].reserve[tmp] = alloc_block(bitmap);
		ide_dma_write (hdf[x].reserve[tmp],1,buf + 0 * 512);
		offset += 512;

while (offset < size) {
    ide_dma_write(alloc_block(bitmap), 1, buf + offset);
    offset += 512;
}
			hdf[x].reserve[tmp+1] = blocks;
			hdf[x].reserve[9]=0xff;
            hdf[x].size = size;
			ide_dma_write(1,1,(unsigned char *)bitmap);
	ide_dma_write(2,1, (unsigned char *)hdf); 
	//char s[30];
//	sprintf(s,"test%8x",size);
	//createfile("ABC.TXT", s);
	//ide_write_pio( hdf[x].reserve[0],1,(unsigned char *)buf,size);
	
}
void ide_write_pio(unsigned int lba, unsigned char sector_count, short* buffer,int size) {
    // 1. 等待硬盘不再忙碌
    while (io_in8(ATA_STATUS) & 0x80);
	int s,i;
    // 2. 发送参数
    io_out8(ATA_SECTOR_CNT, sector_count);
    io_out8(ATA_LBA_LOW,  (unsigned char)(lba & 0xFF));
    io_out8(ATA_LBA_MID,  (unsigned char)((lba >> 8) & 0xFF));
    io_out8(ATA_LBA_HIGH, (unsigned char)((lba >> 16) & 0xFF));
    
    // LBA 模式，主盘
    io_out8(ATA_DRIVE_SEL, 0xE0 | ((lba >> 24) & 0x0F));

    // 3. 发送写命令 0x30
    io_out8(ATA_COMMAND, 0x30);

    // 4. 循环写入数据
    for ( s = 0; s < sector_count; s++) {
        // 重要：在写入每个扇区之前，必须等待 DRQ 为 1
        // 这表示硬盘内部的缓冲区已经准备好接收你的数据了
        while (!(io_in8(ATA_STATUS) & 0x08));
		if(size>0){
			 for ( i = 0; i < size; i++) {
				io_out16(ATA_DATA, buffer[s * 256 + i]);
			}
			for ( i = size; i < 256; i++) {
				io_out16(ATA_DATA, 0);
			}
		}else{
        // 向数据端口 0x1F0 写入 256 次 16位数据
			for ( i = 0; i < 256; i++) {
				io_out16(ATA_DATA, buffer[s * 256 + i]);
			}
		}
    }

    // 5. 强制硬盘将缓存写入介质（Flush Cache）
    // 虽然不是必须，但为了安全建议发送 0xE7 命令
    io_out8(ATA_COMMAND, 0xE7);
    while (io_in8(ATA_STATUS) & 0x80); 
}
void ide_read_pio(unsigned int lba, unsigned char sector_count, short* buffer) {
    // 1. 等待硬盘准备好
	int i,s;
   while (io_in8(ATA_STATUS) & 0x80);

    // 2. 发送 LBA 地址和要读的扇区数
    io_out8(ATA_SECTOR_CNT, sector_count);    // 读几个扇区
    io_out8(ATA_LBA_LOW,  (unsigned char)(lba & 0xFF));
    io_out8(ATA_LBA_MID,  (unsigned char)((lba >> 8) & 0xFF));
    io_out8(ATA_LBA_HIGH, (unsigned char)((lba >> 16) & 0xFF));
    
    // 3. 设置 LBA28 模式并选择主盘 (Master)
    // 0xE0 的含义: 位7和5必须为1, 位6为1(启用LBA), 位4为0(Master)
    io_out8(ATA_DRIVE_SEL, 0xE0 | ((lba >> 24) & 0x0F));

    // 4. 发送读命令 0x20
    io_out8(ATA_COMMAND, 0x20);

    // 5. 循环读取每个扇区的数据
    for (s = 0; s < sector_count; s++) {
        // 每次读取一个扇区前，都要等待数据准备好 (DRQ位)
        while (!(io_in8(ATA_STATUS) & 0x08)); 

        // 从数据端口 0x1F0 读取 256 次 16位数据 (即 512 字节)
        for (i = 0; i < 256; i++) {
            buffer[s * 256 + i] = io_in16(ATA_DATA);
        }
    }
}