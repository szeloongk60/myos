/* 文件相关函数 */

#include "bootpack.h"

void file_readfat(int *fat, unsigned char *img)
/*将磁盘映像中的FAT解压缩 */
{
	int i, j = 0;
	for (i = 0; i < 2880; i += 2) {
		fat[i + 0] = (img[j + 0] | img[j + 1] << 8) & 0xfff;
		fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xfff;
		j += 3;
	}
	return;
}
void file_savefat(int *fat, unsigned char *img)
/* 将内存中解压后的 FAT 重新压缩回磁盘映像格式 */
{
    int i, j = 0;
    for (i = 0; i < 2880; i += 2) {
        // --- 处理 fat[i] (偶数项) ---
        // 字节 j 存 fat[i] 的低 8 位
        img[j + 0] = (unsigned char)(fat[i] & 0xff);
        // 字节 j+1 的低 4 位 存 fat[i] 的高 4 位
        img[j + 1] = (unsigned char)((fat[i] >> 8) & 0x0f);

        // --- 处理 fat[i+1] (奇数项) ---
        // 字节 j+1 的高 4 位 存 fat[i+1] 的低 4 位
        img[j + 1] |= (unsigned char)((fat[i + 1] << 4) & 0xf0);
        // 字节 j+2 存 fat[i+1] 的高 8 位
        img[j + 2] = (unsigned char)((fat[i + 1] >> 4) & 0xff);

        j += 3;
    }
    return;
}

void file_loadfile(int clustno, int size, char *buf, int *fat, char *img)
{
	int i;
	for (;;) {
		if (size <= 512) {
			for (i = 0; i < size; i++) {
				buf[i] = img[clustno * 512 + i];
			}
			break;
		}
		for (i = 0; i < 512; i++) {
			buf[i] = img[clustno * 512 + i];
		}
		size -= 512;
		buf += 512;
		clustno = fat[clustno];
	}
	return;
}
unsigned short make_date(int year, int month, int day) {
    return (unsigned short)(((year - 1980) << 9) | (month << 5) | day);
}

// 生成 FAT 时间
unsigned short make_time(int hour, int min, int sec) {
    return (unsigned short)((hour << 11) | (min << 5) | (sec / 2));
}

void createfile(char *filename, char *buf)
{
    int i, j;
    int cluster = -1;
	fdc_read_lba((unsigned char *) (ADR_DISKIMG + 0x002600),19);
struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
unsigned char *diskimg = (unsigned char *) ADR_DISKIMG; 
struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
fdc_read_lba((unsigned char *) (ADR_DISKIMG + 0x000200),1);
	int *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	int *savefat = (int *) memman_alloc_4k(memman, 4 * 2880);
file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200)); 

    // int i, cluster = -1;

    // ===== 找空簇 =====
    for (i = 2; i < 2880; i++) {
       if (fat[i] == 0) { cluster = i; break; }
    }
    if (cluster == -1) return;
    fat[cluster] = 0xFFF;  // EOF

    // ===== 写数据 =====
    int size = 0;
    while (buf[size] != 0) size++;
    for (i = 0; i < size; i++) {
        diskimg[cluster * 512 + i] = buf[i];
    }

    // ===== 找空目录项 =====
    //struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
    for (i = 0; i < 224; i++) {
        if (finfo[i].name[0] == 0x00 || finfo[i].name[0] == 0xE5) {
            int j;

            // 清空
            for (j = 0; j < 8; j++) finfo[i].name[j] = ' ';
            for (j = 0; j < 3; j++) finfo[i].ext[j] = ' ';

            // 文件名和扩展名填充
            char *p = filename;
            for (j = 0; j < 8 && *p && *p != '.'; j++) finfo[i].name[j] = *p++;
            if (*p == '.') p++;
            for (j = 0; j < 3 && *p; j++) finfo[i].ext[j] = *p++;

            finfo[i].type = 0x20;      // 普通文件
            finfo[i].clustno = cluster;
            finfo[i].size = size;
			finfo[i].date=make_date(2026, 4, 7);
			finfo[i].time=make_time(12, 30, 0);

            break;
        }
    }
	fdc_write(buf,cluster);
	file_savefat(fat,savefat);
	 floppy_write_lba(1,savefat);
	  floppy_write_lba(10,savefat);
	 floppy_write_lba(19,finfo);
	 
}
void format_to_fat_name(const char *src, char *dest) {
    int i, j;
    for (i = 0; i < 11; i++) dest[i] = ' '; // 先填满空格
    
    // 处理文件名部分
    for (i = 0; i < 8 && src[i] != '.' && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    
    // 寻找扩展名点号
    if (src[i] == '.') {
        i++; // 跳过点
        for (j = 0; j < 3 && src[i] != '\0'; j++, i++) {
            dest[8 + j] = src[i];
        }
    }
}

// 核心查找函数
int findfile(const char *filename) {
	fdc_read_lba((unsigned char *) (ADR_DISKIMG + 0x002600),19);
    struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
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
        if (finfo[x].name[0] == 0x00) break; // 目录结束
        if (finfo[x].name[0] == 0xe5) continue; // 已删除文件
        
        // 忽略卷标和非普通文件
        if ((finfo[x].type & 0x18) != 0) continue; 

        // 比较 11 字节
        int match = 1;
        for (y = 0; y < 11; y++) {
            if (finfo[x].name[y] != s[y]) {
                match = 0;
                break;
            }
        }
        
        if (match) return x; //finfo[x].clustno; // 找到了！返回簇号
    }

    return 0; // 没找到
}
unsigned char *lz4read512k(char *filename) {
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    // 1. 先随便申一块够大的地儿放压缩包（或者你可以先查文件大小再申）
    unsigned char *temp_buf = memman_alloc_4k(memman, 512 * 1024); // 假设压缩包不超512K
    int comp_size = readfile(filename, temp_buf);
    
    if (comp_size <= 0) {
        memman_free_4k(memman, temp_buf, 512 * 1024);
        return 0; // 读取失败
    }

    // 2. 剥壳：取前4字节拿到原始大小
    unsigned int raw_size = *(unsigned int *)temp_buf;
    
    // 3. 申领最终的“豪宅”内存
    unsigned char *dest_buf = memman_alloc_4k(memman, raw_size);
    
    // 4. 解压核心
    LZ4_decompress_safe(temp_buf + 4, dest_buf, comp_size - 4, raw_size);
    
    // 5. 毁尸灭迹：释放临时压缩包内存
    memman_free_4k(memman, temp_buf, 512 * 1024);
    
    return dest_buf; // 凯旋归来
}