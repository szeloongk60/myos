/* 窗口相关函数 */

#include "bootpack.h"

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act)
{
	boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         xsize - 1, 0        );
	boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         xsize - 2, 1        );
	boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         0,         ysize - 1);
	boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         1,         ysize - 2);
	boxfill8(buf, xsize, COL8_848484, xsize - 2, 1,         xsize - 2, ysize - 2);
	boxfill8(buf, xsize, COL8_000000, xsize - 1, 0,         xsize - 1, ysize - 1);
	boxfill8(buf, xsize, COL8_C6C6C6, 2,         2,         xsize - 3, ysize - 3);
	boxfill8(buf, xsize, COL8_848484, 1,         ysize - 2, xsize - 2, ysize - 2);
	boxfill8(buf, xsize, COL8_000000, 0,         ysize - 1, xsize - 1, ysize - 1);
	make_wtitle8(buf, xsize, title, act);
	return;
}

void make_wtitle8(unsigned char *buf, int xsize, char *title, char act)
{
	static char closebtn[14][16] = {
		"OOOOOOOOOOOOOOO@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQQQ@@QQQQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"O$$$$$$$$$$$$$$@",
		"@@@@@@@@@@@@@@@@"
	};
	int x, y;
	char c, tc, tbc;
	if (act != 0) {
		tc = COL8_FFFFFF;
		tbc = COL8_000084;
	} else {
		tc = COL8_C6C6C6;
		tbc = COL8_848484;
	}
	boxfill8(buf, xsize, tbc, 3, 3, xsize - 4, 20);
	putfonts8_asc(buf, xsize, 24, 4, tc, title);
	for (y = 0; y < 14; y++) {
		for (x = 0; x < 16; x++) {
			c = closebtn[y][x];
			if (c == '@') {
				c = COL8_000000;
			} else if (c == '$') {
				c = COL8_848484;
			} else if (c == 'Q') {
				c = COL8_C6C6C6;
			} else {
				c = COL8_FFFFFF;
			}
			buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
		}
	}
	return;
}

void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l)
{
	boxfill8(sht->buf, sht->bxsize, b, x, y, x + l * 8 - 1, y + 15);
	putfonts8_asc(sht->buf, sht->bxsize, x, y, c, s);
	sheet_refresh(sht, x, y, x + l * 8, y + 16);
	return;
}

void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c)
{
	int x1 = x0 + sx, y1 = y0 + sy;
	boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
	boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
	boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
	boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
	boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
	boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, 0xffffff,           x0 - 1, y0 - 1, x1 + 0, y1 + 0);
	return;
}
enum ASSET_TYPE {
    ASSET_CORE_UI = 0,    // 默认UI
    ASSET_PROGRESS_BAR,   // 进度条
    ASSET_ICONS,          // 图标集
    ASSET_MAX             // 总数
};
struct ASSETS_LIB {
    unsigned int *buf;    // 像素数据的首地址
    int width;            // 大图的总宽度 (src_w)
    int height;           // 大图的总高度
    int size;             // 占用的总内存大小（用于 memfree）
};

// 全局变量，方便所有函数调用
struct ASSETS_LIB img_assets[10];


void load_mx_img(unsigned char *lz4) {
    char *magic = (char *)lz4;
    unsigned int ori_size = *(unsigned int *)(lz4 + 4);
    unsigned int com_size = *(unsigned int *)(lz4 + 8);
    unsigned char *raw_buf;
    unsigned int *new_mem;
    unsigned short w, h;
    unsigned char *src;
    int i, j, img_idx;
    unsigned char r, g, b, a = 0xFF;
int index=0;
    if (magic[0] != 'L' || magic[1] != 'Z' || index >= 10) return;

    // 1. 申请临时解压空间并解压
    raw_buf = (unsigned char *)memget(ori_size);
    LZ4_decompress_safe(lz4 + 12, raw_buf, com_size, ori_size);

    // 2. 获取宽高
    w = *(unsigned short *)(raw_buf + 0);
    h = *(unsigned short *)(raw_buf + 2);
    src = raw_buf + 4; // 像素起点

    // 3. 【修正】申请 32位 像素内存：宽 * 高 * 4字节
    new_mem = (unsigned int *)memget(w * h * 4);

    // 4. 格式转换：RGB -> RGBA
    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            img_idx = (j * w + i) * 3;
            r = src[img_idx + 0];
            g = src[img_idx + 1];
            b = src[img_idx + 2];
            // 存入新申请的缓冲区
            new_mem[j * w + i] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }

    // 5. 存入你的“多宝阁”结构体
    img_assets[index].buf    = new_mem;
    img_assets[index].width  = w;
    img_assets[index].height = h;
    img_assets[index].size   = w * h * 4;
}
void draw_sprite(struct SHEET *sht, int dx, int dy, 
                 int sx, int sy, int w, int h, int asset_index) 
{
    int i, j;
    unsigned int color;
    // 1. 获取对应的素材包
    struct ASSETS_LIB *lib = &img_assets[asset_index];
    int ww,hh;
	ww=w-sx;
	hh=h-sy;
	sht->bxsize=ww;
	sht->bysize=hh;
    // 安全检查：如果这个索引没载入内容，直接返回，防止死机
    if (lib->buf == 0) return;

    for (j = 0; j < hh; j++) {
        // 垂直越界检查：如果画出 sheet 底部了就停下
        if (dy + j >= sht->bysize) break;
        if (dy + j < 0) continue;

        for (i = 0; i < ww; i++) {
            // 水平越界检查
            if (dx + i >= sht->bxsize) break;
            if (dx + i < 0) continue;

            // 2. 从素材包里取颜色 (利用 lib 里的 width)
            color = lib->buf[(sy + j) * lib->width + (sx + i)];
            
            // 3. 透明处理 (假设 0x00FF00FF 是透明)
            // 注意：有些素材图用纯黑 0 或粉色作为透明
            if (color != 0xFF00FF) {
                sht->buf[(dy + j) * sht->bxsize + (dx + i)] = color;
            }
        }
    }
	
}
