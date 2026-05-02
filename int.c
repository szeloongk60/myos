/*初始化关系 */

#include "bootpack.h"
#include <stdio.h>
int fdc_ready_flag = 0;
void init_pic(void)
/* PIC初始化 */
{
	io_out8(PIC0_IMR,  0xff  ); /* 禁止所有中断 */
	io_out8(PIC1_IMR,  0xff  ); /* 禁止所有中断 */

	io_out8(PIC0_ICW1, 0x11  ); /* 边缘触发模式（edge trigger mode） */
	io_out8(PIC0_ICW2, 0x20  ); /* IRQ0-7由INT20-27接收 */
	io_out8(PIC0_ICW3, 1 << 2); /* PIC1由IRQ2相连 */
	io_out8(PIC0_ICW4, 0x01  ); /* 无缓冲区模式 */

	io_out8(PIC1_ICW1, 0x11  ); /* 边缘触发模式（edge trigger mode） */
	io_out8(PIC1_ICW2, 0x28  ); /* IRQ8-15由INT28-2f接收 */
	io_out8(PIC1_ICW3, 2     ); /* PIC1由IRQ2连接 */
	io_out8(PIC1_ICW4, 0x01  ); /* 无缓冲区模式 */

	io_out8(PIC0_IMR,  0xfb  ); /* 11111011 PIC1以外全部禁止 */
	io_out8(PIC1_IMR,  0xff  ); /* 11111111 禁止所有中断 */

	return;
}

void inthandler27(int *esp)
/* PIC0中断的不完整策略 */
/* 这个中断在Athlon64X2上通过芯片组提供的便利，只需执行一次 */
/* 这个中断只是接收，不执行任何操作 */
/* 为什么不处理？
	→  因为这个中断可能是电气噪声引发的、只是处理一些重要的情况。*/
{
	io_out8(PIC0_OCW2, 0x67); /* 通知PIC的IRQ-07（参考7-1） */
	return;
}

	extern struct SHEET*sht_back;
	unsigned int IDE_DMA_BASE = 0; 
	int dma_done_flag=0;
void inthandler2e(int *esp)
{
unsigned char status;
putfonts8_ascAll(200,16,"int 2e");
putfonts8_sht(sht_back,120,16,"int 2e",0);
    /* 1. 通知 PIC0 和 PIC1：中断受理已经开始（发送 EOI） */
    /* 因为 IRQ 14 在从片上，必须先向从片发送，再向主片发送 */
  

    /* 或者使用通用的 EOI 发送方式（取决于你的系统实现） */
    // io_out8(0xa0, 0x20); 
    // io_out8(0x20, 0x20);

    /* 2. 读取 Bus Master IDE 的状态寄存器并清理中断标志位 */
    /* 这是 DMA 模式下最关键的一步，不清理这个位，下次 DMA 无法启动 */
    // dma_io_base 是你之前从 PCI BAR4 拿到的地址
   status = io_in8(IDE_DMA_BASE + 2);
    io_out8(IDE_DMA_BASE + 2, status | 0x04); // 往 Bit 2 写 1 即可清零该位

    /* 3. 读取硬盘状态寄存器 (0x1F7) */
    /* 这一步是为了让硬盘控制器知道 CPU 已经响应，从而撤销 IRQ 信号 */
    status = io_in8(0x1F7); 
	  io_out8(PIC1_OCW2, 0x66); // 0x66 是针对 IRQ 14 的特定的 EOI (0x60 + 6)
    io_out8(PIC0_OCW2, 0x62); // 0x62 是针对 IRQ 2 (从片级联位) 的 EOI (0x60 + 2)
 
    /* 4. 执行你的后续逻辑 */
    // 比如设置一个全局变量标识传输已完成
    dma_done_flag = 1;

    return;

}
void inthandler2f(int *esp)
{
unsigned char status;
putfonts8_ascAll(200,16,"int 2f");
putfonts8_sht(sht_back,120,16,"int 2f",1);
    /* 1. 通知 PIC0 和 PIC1：中断受理已经开始（发送 EOI） */
    /* 因为 IRQ 14 在从片上，必须先向从片发送，再向主片发送 */
    io_out8(PIC1_OCW2, 0x66); // 0x66 是针对 IRQ 14 的特定的 EOI (0x60 + 6)
    io_out8(PIC0_OCW2, 0x62); // 0x62 是针对 IRQ 2 (从片级联位) 的 EOI (0x60 + 2)

    /* 或者使用通用的 EOI 发送方式（取决于你的系统实现） */
    // io_out8(0xa0, 0x20); 
    // io_out8(0x20, 0x20);

    /* 2. 读取 Bus Master IDE 的状态寄存器并清理中断标志位 */
    /* 这是 DMA 模式下最关键的一步，不清理这个位，下次 DMA 无法启动 */
    // dma_io_base 是你之前从 PCI BAR4 拿到的地址
  //  status = io_in8(dma_io_base + 2);
  //  io_out8(dma_io_base + 2, status | 0x04); // 往 Bit 2 写 1 即可清零该位

    /* 3. 读取硬盘状态寄存器 (0x1F7) */
    /* 这一步是为了让硬盘控制器知道 CPU 已经响应，从而撤销 IRQ 信号 */
    status = io_in8(0x1F7); 
 
    /* 4. 执行你的后续逻辑 */
    // 比如设置一个全局变量标识传输已完成
    //dma_done_flag = 1;

    return;

}

unsigned int pci_read(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned int address;
    // 组合 PCI 地址：最高位必须是 1
    address = (unsigned int)((unsigned int)0x80000000 | 
              ((unsigned int)bus << 16) | 
              ((unsigned int)slot << 11) | 
              ((unsigned int)func << 8) | 
              (offset & 0xfc));
    
     io_out32(0xCF8, address);
    return io_in32(0xCFC);
}

void pci_write(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset, unsigned int data) {
    unsigned int address = (unsigned int)((unsigned int)0x80000000 | 
                           ((unsigned int)bus << 16) | 
                           ((unsigned int)slot << 11) | 
                           ((unsigned int)func << 8) | 
                           (offset & 0xfc));
    io_out32(0xCF8, address);
    io_out32(0xCFC, data);
}
struct PRD_Entry {
    unsigned int addr;     // physical address
    unsigned short size;   // byte count (0 = 64KB)
    unsigned short flags;  // bit15 = EOT
} __attribute__((packed));

// 申请一个全局 PRD 表（假设只用一个条目）
struct PRD_Entry my_prd __attribute__((aligned(4)));

void setup_prd(void* buffer, unsigned short bytes) {
    my_prd.addr = (unsigned int)buffer; // 注意：如果是分页模式，这里必须是物理地址
    my_prd.size = bytes;
    my_prd.flags = 0x8000; // 只有这一个片段，所以它是结束标志
}
void wait_ide_ready() {
    // 硬盘状态端口是 0x1F7
    // Bit 7 (BSY): 1 = 忙碌，0 = 准备好了
    // Bit 6 (DRDY): 1 = 驱动器就绪 (Device Ready)
    
    while (1) {
        // 读取状态寄存器
        unsigned char status = io_in8(0x1F7);
        
        // 我们需要：BSY位为0 且 DRDY位为1
        // 0x80 是 10000000 (BSY)
        // 0x40 是 01000000 (DRDY)
        if ((status & 0x80) == 0x00 && (status & 0x40)) {
            break; 
        }
    }
}
// 存 BAR4

void init_ide_pci() {
	unsigned int bus;
	unsigned int dev;
	unsigned int func;
	
	char temp[64];
	//char s[520]={0};
    for ( bus = 0; bus < 256; bus++) {
        for ( dev = 0; dev < 32; dev++) {
            for (func = 0; func < 8; func++) { // 遍历 8 个功能
            unsigned int ashome = pci_read(bus, dev, func, 0x00);
			if ((ashome& 0xFFFF) == 0xFFFF) continue;
			unsigned int info = pci_read(bus, dev, func, 0x08);
            unsigned char base_class = (info >> 24) & 0xFF;
            unsigned char sub_class = (info >> 16) & 0xFF;
			//sprintf(s, "dev=%02x base=%02x sub=%02x", dev, base_class, sub_class);
			
			//sprintf(temp, "ashome=%02x info=%02x dev=%02x base=%02x sub=%02x\n", ashome,info,dev, base_class, sub_class);
			//strcat(s, temp); // 把新的一行接到总变量 s 后面
            if (base_class == 0x01 && sub_class == 0x01) {
                // 找到了 IDE 控制器！
                
                // 1. 获取 BAR4 (偏移 0x20)，它是 DMA 的端口基地址
                unsigned int bar4 = pci_read(bus, dev, func, 0x20);
               IDE_DMA_BASE = bar4 & 0xFFFFFFFC; // 屏蔽低位标志
			  // io_out32(dma_io_base + 4, (unsigned int)&my_prd);
			

                // 2. 关键：开启 Bus Master (命令寄存器偏移 0x04)
                unsigned int cmd = pci_read(bus, dev, func, 0x04);
                if (!(cmd & 0x04)) {
                    cmd |= 0x04; // 设置 Bit 2 为 1
                    pci_write(bus, dev, func, 0x04, cmd);
					io_out8(0x3F6, 0x00);
					char* hdbuf=memget(1024*5);
					  //////////////////////////////////////////////////////////////
					//  ide_dma_read(0,1,hdbuf);
					 // sprintf(temp,"测试%s",hdbuf);
				//createfile("ABC.TXT", temp);

                return; // 找
			  ////////////////////////////////////////////////////////////////////
                }
				//到一个就行了
            }
			}
        }
    }
	
}
void ide_dma_read(unsigned int lba, unsigned char count, char *buf) {
    if (IDE_DMA_BASE == 0 ) return; // 没扫到硬件
	dma_done_flag=0;
    // 1. 设置 PRD 表（注意：buf 必须是物理地址）
    setup_prd(buf, count * 512);
    io_out32(IDE_DMA_BASE + 4, (unsigned int)&my_prd);

    // 2. 准备 DMA 控制器 (读方向 + 清理状态)
    io_out8(IDE_DMA_BASE + 0, 0x08); 
    io_out8(IDE_DMA_BASE + 2, 0x06);

    // 3. 设置 ATA 参数 (Task File)
    wait_ide_ready();
    io_out8(0x1F6, 0xE0 | ((lba >> 24) & 0x0F)); // LBA 高位
    io_out8(0x1F2, count);
    io_out8(0x1F3, lba & 0xFF);
    io_out8(0x1F4, (lba >> 8) & 0xFF);
    io_out8(0x1F5, (lba >> 16) & 0xFF);

    // 4. 发命令并合闸
    io_out8(0x1F7, 0xC8); // DMA READ
    io_out8(IDE_DMA_BASE + 0, 0x08 | 0x01); // START!

    // 5. 等待 (如果你用了中断，这里可以改成等待信号量，现在先轮询)
	int a=0;
    while (!dma_done_flag) {
		if(a>10000){break;}
        a++;
    }
}
void ide_dma_write(unsigned int lba, unsigned char count, char *buf) {
    if (IDE_DMA_BASE == 0) return;

    // 1. 设置 PRD 表（把 buf 里的内容搬到硬盘）
    setup_prd(buf, count * 512);
    io_out32(IDE_DMA_BASE + 4, (unsigned int)&my_prd);

    // 2. 准备 DMA 控制器
    // 注意：这里是 0x00，不是 0x08！ 
    // Bit 3 = 0 表示从内存写入硬盘 (Memory to Device)
    io_out8(IDE_DMA_BASE + 0, 0x00); 
    io_out8(IDE_DMA_BASE + 2, 0x06); // 清理状态位

    // 3. 设置 ATA 寻址参数
    wait_ide_ready();
    io_out8(0x1F6, 0xE0 | ((lba >> 24) & 0x0F)); 
    io_out8(0x1F2, count);
    io_out8(0x1F3, lba & 0xFF);
    io_out8(0x1F4, (lba >> 8) & 0xFF);
    io_out8(0x1F5, (lba >> 16) & 0xFF);

    // 4. 发送写命令并启动
    io_out8(0x1F7, 0xCA); // 关键修改：0xCA 是 DMA WRITE 命令
    
    // 合闸：Bit 0 设为 1 启动。此时 Bit 3 依然是 0（写方向）
    io_out8(IDE_DMA_BASE + 0, 0x01); 

    // 5. 轮询等待完成
    while (1) {
        unsigned char status = io_in8(IDE_DMA_BASE + 2);
        if (!(status & 0x01)) break; 
        if (status & 0x02) { /* 错误处理 */ break; }
    }
    
    /* 写入完成后，最好发送一个缓存刷新命令（可选但推荐） */
     io_out8(0x1F7, 0xE7); // Flush Cache
}