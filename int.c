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
void wait_ide_ready2() {
    unsigned char status;

    while (1) {
        status = io_in8(0x1F7);

        // BSY=0 且 DRQ=1 才可以读写数据
        if ((status & 0x80) == 0 && (status & 0x08)) {
            break;
        }

        // 可选：错误处理
        if (status & 0x01) {
            // ERR = 1
            break;
        }
    }
}
// 存 BAR4
unsigned int eth_mmio_base;
void init_ide_pci() {
	unsigned int bus;
	unsigned int dev;
	unsigned int func;
	int tt=0;
	char temp[64];
	char s[50]={0};
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
			if (base_class == 0x02 && sub_class == 0x00) {
    // 找到了以太网控制器！
    
    // 1. 获取基地址 (通常网卡使用 BAR0 偏移 0x10)
    unsigned int bar0 = pci_read(bus, dev, func, 0x10);
    
    // 判断是 IO 端口还是内存映射 (MMIO)
    if (bar0 & 0x01) {
        // IO Port 模式
        unsigned int eth_io_base = bar0 & 0xFFFFFFFC;
    } else {
        // MMIO 模式
         eth_mmio_base = bar0 & 0xFFFFFFF0;
    }

    // 2. 必须开启 Bus Master 和 内存/IO 响应
    // 偏移 0x04 是 PCI Command Register
    unsigned int cmd = pci_read(bus, dev, func, 0x04);
    cmd |= (1 << 2); // 开启 Bus Master (DMA 必备)
    cmd |= (1 << 0); // 开启 IO Space
    cmd |= (1 << 1); // 开启 Memory Space
    pci_write(bus, dev, func, 0x04, cmd);

    // 3. 获取中断号 (用于处理收包)
    unsigned int interrupt_line = pci_read(bus, dev, func, 0x3C) & 0xFF;

unsigned int low =  (*(volatile unsigned int *)(eth_mmio_base + 0x5400));
unsigned int high = (*(volatile unsigned int *)(eth_mmio_base + 0x5404));

// MAC 地址前 4 字节在 low，后 2 字节在 high 的低 16 位
sprintf(s, "MAC: %02x:%02x:%02x:%02x:%02x:%02x",
        low & 0xFF, (low >> 8) & 0xFF, (low >> 16) & 0xFF, (low >> 24) & 0xFF,
        high & 0xFF, (high >> 8) & 0xFF);
putfonts8_ascAll(200, 220, s);

    tt++;
	if(tt==2){return; }// 找
    // 接下来根据具体的网卡型号（如 Realtek 8139 或 Intel E1000）进行寄存器操作
}
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
					io_out8( 0x1F6, 0xA0);
				
					char* hdbuf=memget(1024*5);
					  //////////////////////////////////////////////////////////////
					//  ide_dma_read(0,1,hdbuf);
					 // sprintf(temp,"测试%s",hdbuf);
				//createfile("ABC.TXT", temp);
				tt++;
				if(tt==2){return; }// 找
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
// [1. 类型定义]
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

// [2. 宏定义]
#define mmio_read32(addr) (*(volatile uint32_t *)(addr))
#define mmio_write32(addr, val) (*(volatile uint32_t *)(addr) = val)
#define RX_DESC_COUNT 32  // 必须是 8 的倍数
#define RX_BUFFER_SIZE 2048
// [3. 结构体定义]
struct e1000_rx_desc {
    uint32_t addr_low;
    uint32_t addr_high;
    uint16_t length;
    uint16_t checksum;
    uint8_t  status;
    uint8_t  errors;
    uint16_t special;
} __attribute__((packed));
struct e1000_tx_desc {
    unsigned int addr_low;   // 缓冲区物理地址低32位
    unsigned int addr_high;  // 高32位 (通常为0)
    unsigned short length;   // 长度
    unsigned char cso;       // Checksum Offset
    unsigned char cmd;       // Command
    unsigned char sta;       // Status
    unsigned char css;       // Checksum Start
    unsigned short special;
} __attribute__((packed));
#define TX_DESC_COUNT 8
struct e1000_tx_desc *tx_ring;
char *tx_buffers[TX_DESC_COUNT];
struct e1000_rx_desc *rx_ring;
char *rx_buffers[RX_DESC_COUNT]; 

void init_e1000_tx() {
    // 1. 分配并对齐发送环内存 (同 RX 逻辑)
    unsigned int raw_addr = (unsigned int)memget(TX_DESC_COUNT * 16 + 128);
    tx_ring = (struct e1000_tx_desc *)((raw_addr + 127) & ~127);

    // 2. 配置寄存器
    mmio_write32(eth_mmio_base + 0x3800, (uint32_t)tx_ring); // TDBAL
    mmio_write32(eth_mmio_base + 0x3804, 0);                 // TDBAH
    mmio_write32(eth_mmio_base + 0x3808, TX_DESC_COUNT * 16); // TDLEN
    mmio_write32(eth_mmio_base + 0x3810, 0);                 // TDH (头)
    mmio_write32(eth_mmio_base + 0x3818, 0);                 // TDT (尾)

    // 3. 开启发送控制寄存器 (TCTL)
    // EN=1 (Bit 1), PSP=1 (Bit 3, 自动填充短包)
    mmio_write32(eth_mmio_base + 0x0400, (1 << 1) | (1 << 3));
}
void init_e1000()
{
   int i;
    unsigned int raw_addr;

    // 1. 分配并【强制 128 字节对齐】
    // 8个描述符只需要 128字节，但为了对齐，我们分配 256字节
    raw_addr = (unsigned int)memget(RX_DESC_COUNT * 16 + 128);
    rx_ring = (struct e1000_rx_desc *)((raw_addr + 127) & ~127);

    // 2. 为每个描述符分配缓冲区
    for (i = 0; i < RX_DESC_COUNT; i++) {
        rx_buffers[i] = (char *)memget(RX_BUFFER_SIZE);
        
        rx_ring[i].addr_low = (uint32_t)rx_buffers[i]; 
        rx_ring[i].addr_high = 0;
        rx_ring[i].status = 0;   // DD (Descriptor Done) 位清零
        rx_ring[i].errors = 0;
        rx_ring[i].length = 0;
    }

    // [额外安全步] 禁用网卡所有中断，防止初始化时崩溃
    mmio_write32(eth_mmio_base + 0x00D0, 0xFFFFFFFF);

    // 3. 告诉网卡描述符基地址 (必须是刚才对齐后的地址)
    mmio_write32(eth_mmio_base + 0x2800, (uint32_t)rx_ring);
    mmio_write32(eth_mmio_base + 0x2804, 0); 

    // 4. 设置环的长度 (8 * 16 = 128 字节)
    mmio_write32(eth_mmio_base + 0x2808, RX_DESC_COUNT * 16);

    // 5. 设置头尾指针
    // RDH = 0 (硬件负责移动)
    mmio_write32(eth_mmio_base + 0x2810, 0); 
    // RDT = 7 (软件负责移动，指向网卡可写的最后一个槽位)
    mmio_write32(eth_mmio_base + 0x2818, RX_DESC_COUNT - 1); 

    // 6. 配置接收控制寄存器 (RCTL)
    // Bit 1: EN, Bit 15: BAM, Bit 26: SECRC
    // 如果你发现收不到包，可以尝试加上 Bit 4 (MPE) 和 Bit 3 (UPE) 开启混杂模式测试
    unsigned int rctl = (1 << 1) | (1 << 15) | (1 << 26); 
    mmio_write32(eth_mmio_base + 0x0100, rctl);
unsigned int ctrl = mmio_read32(eth_mmio_base + 0x0000);

// Bit 6 是 SLU (Set Link Up) 位
// 设置这一位会强制网卡建立链路，通常绿灯就会亮起或闪烁
ctrl |= (1 << 6); 

// 同时确保复位位 (Bit 26) 是 0，防止网卡一直处于复位状态
ctrl &= ~(1 << 26);

mmio_write32(eth_mmio_base + 0x0000, ctrl);
    // 调试显示
    char debug_msg[64];
    //sprintf(debug_msg, "E1000 OK! Ring: %08X", (unsigned int)rx_ring);
   // putfonts8_ascAll(180, 280, debug_msg);
	unsigned int status = mmio_read32(eth_mmio_base + 0x0008);
if (status & (1 << 1)) {
    putfonts8_ascAll(200, 340, "LINK IS UP! (Light should be on)");
} else {
    putfonts8_ascAll(200, 340, "LINK IS DOWN...");
}
}
int current_rx = 0;
int ps[50];
// 在主循环里调用这个
 unsigned char arp_packet[60]; // 以太网最小长度是60字节（不含CRC）
void dump_packet(unsigned char *buf, int len) {
    char s[64];
    
    // 1. 打印源 MAC 地址 (第 6-11 字节)
    // 这就是发包给你的那台电脑（宿主机）的网卡地址
    sprintf(s, "MAC: %02X:%02X:%02X:%02X:%02X:%02X", 
            buf[6], buf[7], buf[8], buf[9], buf[10], buf[11]);
    putfonts8_ascAll(200, 420, s);

    // 2. 打印协议类型
    // 0800 是 IPv4, 0806 是 ARP
    unsigned short type = (buf[12] << 8) | buf[13];
    sprintf(s, "type: %04X", type);
    putfonts8_ascAll(200, 440, s);
	if (type == 0x0806) {
        // 简单判断：如果对方是在找我的 IP
        // 这里可以提取 buf[38-41] 看看是不是你的 IP
        
        unsigned char my_mac[6] = {0x08, 0x00, 0x27, 0x00, 0x00, 0x13};
        unsigned char my_ip[4]  = {192, 168, 56, 66};
        
        // 构造回复：把对方的 MAC 和 IP 填进去
        build_arp_reply(my_mac, &buf[6], my_ip, &buf[28]);
        
        // 发送！
        e1000_send_packet(arp_packet, 60);
	}
	return;
}
int bbx=200;
void check_packet() {
	putfonts8_ascAll(200, 280, "*");
    // 检查 DD 位 (status 的 bit 0)
    if (rx_ring[current_rx].status & 0x01) {
        
        // 打印出收到的包长度
        //sprintf(ps, "New Packet! Len: %d", rx_ring[current_rx].length);
       // putfonts8_ascAll(200, 280, ps);
		unsigned char *buf = (unsigned char *)rx_buffers[current_rx];
        dump_packet(buf,rx_ring[current_rx].length);
	
        // 处理完后，清除状态位，并更新 RDT 告诉网卡这块地盘你可以用了
        rx_ring[current_rx].status = 0;
        mmio_write32(eth_mmio_base + 0x2818, current_rx);
		putfonts8_ascAll(bbx, 280, "*");
        bbx+=8;
        current_rx = (current_rx + 1) % RX_DESC_COUNT;
    }
	return;
}
int cur_tx = 0;


void e1000_send_packet(void *packet, int len) {
    // 记录下当前的索引，用于后面检查状态
    int old_tx = cur_tx;

    // 1. 填充描述符
    tx_ring[old_tx].addr_low = (unsigned int)packet; 
    tx_ring[old_tx].addr_high = 0;
    tx_ring[old_tx].length = len;
    tx_ring[old_tx].sta = 0;
    // EOP (0x01) | IFCS (0x02) | RS (0x08)
    tx_ring[old_tx].cmd = (1 << 0) | (1 << 1) | (1 << 3);

    // 2. 更新尾指针触发发送
    cur_tx = (cur_tx + 1) % 8; // 假设 TX_DESC_COUNT 是 8
    mmio_write32(eth_mmio_base + 0x3818, cur_tx);

    // 3. 等待发送完成 (硬件会将 sta 置 1)
    while (!(tx_ring[old_tx].sta & 0x01));
	return;
}



void build_arp_reply(unsigned char *src_mac, unsigned char *dst_mac, 
                      unsigned char *src_ip,  unsigned char *dst_ip) {
    int i;
    
    // --- 以太网首部 ---
    for(i=0; i<6; i++) arp_packet[i] = dst_mac[i];   // 目标MAC
    for(i=0; i<6; i++) arp_packet[i+6] = src_mac[i]; // 源MAC
    arp_packet[12] = 0x08; arp_packet[13] = 0x06;    // 类型: ARP

    // --- ARP 数据段 ---
    arp_packet[14] = 0x00; arp_packet[15] = 0x01;    // 硬件类型: Ethernet (1)
    arp_packet[16] = 0x08; arp_packet[17] = 0x00;    // 协议类型: IPv4 (0x0800)
    arp_packet[18] = 0x06;                           // 硬件地址长度 (6)
    arp_packet[19] = 0x04;                           // 协议地址长度 (4)
    arp_packet[20] = 0x00; arp_packet[21] = 0x02;    // 操作码: Reply (2)

    // 发送方 MAC (你)
    for(i=0; i<6; i++) arp_packet[22+i] = src_mac[i];
    // 发送方 IP (你)
    for(i=0; i<4; i++) arp_packet[28+i] = src_ip[i];
    
    // 目标 MAC (宿主机)
    for(i=0; i<6; i++) arp_packet[32+i] = dst_mac[i];
    // 目标 IP (宿主机)
    for(i=0; i<4; i++) arp_packet[38+i] = dst_ip[i];

    // 填充 0 直到 60 字节 (Padding)
    for(i=42; i<60; i++) arp_packet[i] = 0x00;
	return;
}