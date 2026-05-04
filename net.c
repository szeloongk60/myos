#include "bootpack.h"

extern struct PICBAR *pcbar;
// [1. 类型定义]
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef unsigned int   size_t;
// [2. 宏定义]
#define mmio_read32(addr) (*(volatile uint32_t *)(addr))
#define mmio_write32(addr, val) (*(volatile uint32_t *)(addr) = val)
#define RX_DESC_COUNT 32  // 必须是 8 的倍数
#define RX_BUFFER_SIZE 2048
#define REG_ICR      0x00C0  // Interrupt Cause Read (读了它，硬件才会清中断)
#define REG_ICS      0x00C8  // Interrupt Cause Set (手动触发中断调试用)
#define REG_IMS      0x00D0  // Interrupt Mask Set (开启中断掩码)
#define REG_IMC      0x00D8  // Interrupt Mask Clear (关闭中断掩码)
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
struct dhcp_packet {
    uint8_t  op;          // 1 为请求, 2 为回复
    uint8_t  htype;       // 1 (以太网)
    uint8_t  hlen;        // 6 (MAC长度)
    uint8_t  hops;        // 0
    uint32_t xid;         // 随机事务 ID，用于匹配回复
    uint16_t secs;        // 启动后的秒数
    uint16_t flags;       // 0x8000 (支持广播)
    uint32_t ciaddr;      // 0.0.0.0 (客户端 IP)
    uint32_t yiaddr;      // 0.0.0.0 (这是你要拿到的 IP)
    uint32_t siaddr;      // 服务器 IP
    uint32_t giaddr;      // 网关 IP
    uint8_t  chaddr[16];  // 你的 MAC 地址 (填入前 6 字节)
    uint8_t  sname[64];   // 置零
    uint8_t  file[128];   // 置零
    uint32_t magic_cookie; // 必须为 0x63538263
    uint8_t  options[312]; // 选项字段（包含消息类型、请求的 IP 等）
};
struct eth_header {
    uint8_t  dest[6];
    uint8_t  src[6];
    uint16_t type; // 0x0800 为 IP
} __attribute__((packed));

struct ip_header {
    uint8_t  ver_ihl;   // 0x45
    uint8_t  tos;       // 0x00
    uint16_t len;       // 总长度
    uint16_t id;        // 标识符
    uint16_t flags_off; // 0x0000
    uint8_t  ttl;       // 0x40
    uint8_t  proto;     // 0x11 (UDP)
    uint16_t checksum;  // 校验和 (关键！)
    uint32_t src_ip;    // 0.0.0.0
    uint32_t dest_ip;   // 255.255.255.255
} __attribute__((packed));

struct udp_header {
    uint16_t src_port;  // 68
    uint16_t dest_port; // 67
    uint16_t len;       // UDP 长度
    uint16_t checksum;  // 0 (UDP 校验和可选，填 0 比较省事)
} __attribute__((packed));
#define TX_DESC_COUNT 8
struct e1000_tx_desc *tx_ring;
char *tx_buffers[TX_DESC_COUNT];
struct e1000_rx_desc *rx_ring;
char *rx_buffers[RX_DESC_COUNT]; 
uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t qemu_default_mac[6] ;
static inline uint16_t swap16(uint16_t val) {
    return (uint16_t)((val << 8) | (val >> 8));
}
void get_e1000_mac( uint8_t *mac) {
    // 读取低 32 位 (前 4 字节)
    uint32_t low = mmio_read32(pcbar->NET_DMA_BASE  + 0x5400);
    // 读取高 32 位 (后 2 字节)
    uint32_t high = mmio_read32(pcbar->NET_DMA_BASE  + 0x5404);

    // 映射到 MAC 数组
    mac[0] = (uint8_t)(low & 0xFF);         // 0x52
    mac[1] = (uint8_t)((low >> 8) & 0xFF);  // 0x54
    mac[2] = (uint8_t)((low >> 16) & 0xFF); // 0x00
    mac[3] = (uint8_t)((low >> 24) & 0xFF); // 0x12
    mac[4] = (uint8_t)(high & 0xFF);        // 0x34
    mac[5] = (uint8_t)((high >> 8) & 0xFF); // 0x56
}
uint16_t calculate_ip_checksum(void *vdata, size_t length) {
    uint32_t sum = 0;
    uint16_t *data = (uint16_t *)vdata;

    // 每次处理 2 字节（16 位）
    while (length > 1) {
        sum += *data++;
        length -= 2;
    }

    // 如果长度是奇数，处理最后一个字节
    if (length > 0) {
        sum += *(uint8_t *)data;
    }

    // 将 32 位的 sum 折叠回 16 位
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // 最后取反
    return (uint16_t)(~sum);
}

void send_dhcp_discover() {
    struct dhcp_packet pkt;
    memset(&pkt, 0, sizeof(pkt));
	get_e1000_mac(qemu_default_mac);
    // --- 标准字段配置 ---
    pkt.op = 1; 
    pkt.htype = 1;
    pkt.hlen = 6;
    pkt.xid = 0x33202605; // 既然你 33 岁，在 2026 年 5 月写 OS，可以用这个当 ID
    pkt.flags = 0x0080;   // 注意字节序，通常设为 0x8000 表示支持广播
    memcpy(pkt.chaddr, qemu_default_mac, 6); //
    pkt.magic_cookie = 0x63538263; // 固定的魔数

    // --- DHCP Options (最重要部分) ---
    uint8_t *opt = pkt.options;
    
    // 1. Message Type: Discover
    *opt++ = 53; *opt++ = 1; *opt++ = 1; 
    
    // 2. Client Identifier (Option 61)
    *opt++ = 61; *opt++ = 7; *opt++ = 1;
    memcpy(opt, qemu_default_mac, 6); opt += 6;
    
    // 3. Parameter Request List (请求子网掩码、路由器、DNS)
    *opt++ = 55; *opt++ = 3;
    *opt++ = 1;  // Subnet Mask
    *opt++ = 3;  // Router
    *opt++ = 6;  // DNS
    
    // 4. End Option
    *opt++ = 255;

    // --- 包装成 UDP/IP/Ethernet 并发送 ---
    // 这里你需要调用你写的以太网发送函数
    // 目标 MAC: FF:FF:FF:FF:FF:FF
    // 目标 IP: 255.255.255.255
    // 源端口: 68, 目标端口: 67
    ethernet_send_udp(broadcast_mac, "255.255.255.255", 68, 67, &pkt, sizeof(pkt));
}
void ethernet_send_udp(uint8_t *dest_mac, uint32_t dest_ip, uint16_t src_port, uint16_t dst_port, void *data, int len) {
    uint8_t buffer[1500]; // 准备一个足够大的缓冲区（MTU通常是1500）
    int offset = 0;

    // 1. 填充以太网头
    struct eth_header *eth = (struct eth_header *)buffer;
    memcpy(eth->dest, dest_mac, 6);

memcpy(eth->src, qemu_default_mac, 6);
    eth->type = swap16(0x0800); 
    offset += sizeof(struct eth_header);

    // 2. 填充 IP 头
 struct ip_header *ip = (struct ip_header *)(buffer + offset);
ip->ver_ihl = 0x45;
ip->tos = 0x00; // 记得把这个也清零，否则计算会乱
ip->len = swap16(sizeof(struct ip_header) + sizeof(struct udp_header) + len);
ip->id = swap16(0x1234); // 建议给个标识符，方便 Wireshark 追踪
ip->flags_off = 0x0000;
ip->ttl = 64;
ip->proto = 0x11; // UDP
ip->src_ip = 0x00000000; // DHCP Discover 时源 IP 为 0
ip->dest_ip = 0xFFFFFFFF; // 广播目标 IP

// --- 核心步骤：计算校验和 ---
ip->checksum = 0; // 必须先置零再计算！
ip->checksum = calculate_ip_checksum(ip, sizeof(struct ip_header));

offset += sizeof(struct ip_header);

    // 3. 填充 UDP 头
    struct udp_header *udp = (struct udp_header *)(buffer + offset);
    udp->src_port = swap16(src_port);
    udp->dest_port = swap16(dst_port);
    udp->len = swap16(sizeof(struct udp_header) + len);
    offset += sizeof(struct udp_header);

    // 4. 拷贝 DHCP 数据
    memcpy(buffer + offset, data, len);
    offset += len;

    // 5. 调用你已经写好的底层发送函数
    e1000_send_packet(buffer, offset);
}
void inthandler29(int *esp)

{

uint32_t icr = mmio_read32(pcbar->NET_DMA_BASE + 0x00C0);

    // 2. 判断中断原因
    if (icr & 0x04) {
        // 链路改变，打印一下状态
        // printf("Link Status Changed: %x\n", mmio_read32(pcbar->NET_DMA_BASE + 0x0008));
    }

    if (icr & 0x80) {
        // 收到包了！调用收包函数
        //e1000_recv_task(); 
		check_packet();
    }
 io_out8(PIC1_OCW2, 0x63); // 0x60 + 3 = 0x63 (针对从片 IRQ 3，即全局 IRQ 11)
io_out8(PIC0_OCW2, 0x62);

	
	return;
}
void init_e1000_tx() {
	if(pcbar->NET_DMA_BASE == 0){return;}
    // 1. 分配并对齐发送环内存 (同 RX 逻辑)
    unsigned int raw_addr = (unsigned int)memget(TX_DESC_COUNT * 16 + 128);
    tx_ring = (struct e1000_tx_desc *)((raw_addr + 127) & ~127);

    // 2. 配置寄存器
    mmio_write32(pcbar->NET_DMA_BASE + 0x3800, (uint32_t)tx_ring); // TDBAL
    mmio_write32(pcbar->NET_DMA_BASE + 0x3804, 0);                 // TDBAH
    mmio_write32(pcbar->NET_DMA_BASE + 0x3808, TX_DESC_COUNT * 16); // TDLEN
    mmio_write32(pcbar->NET_DMA_BASE + 0x3810, 0);                 // TDH (头)
    mmio_write32(pcbar->NET_DMA_BASE + 0x3818, 0);                 // TDT (尾)

    // 3. 开启发送控制寄存器 (TCTL)
    // EN=1 (Bit 1), PSP=1 (Bit 3, 自动填充短包)
    mmio_write32(pcbar->NET_DMA_BASE + 0x0400, (1 << 1) | (1 << 3));
}
void init_e1000()
{
	if(pcbar->NET_DMA_BASE == 0){return;}
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
    mmio_write32( pcbar->NET_DMA_BASE + 0x00D8, 0xFFFFFFFF);

    // 3. 告诉网卡描述符基地址 (必须是刚才对齐后的地址)
    mmio_write32( pcbar->NET_DMA_BASE + 0x2800, (uint32_t)rx_ring);
    mmio_write32( pcbar->NET_DMA_BASE + 0x2804, 0); 

    // 4. 设置环的长度 (8 * 16 = 128 字节)
    mmio_write32( pcbar->NET_DMA_BASE + 0x2808, RX_DESC_COUNT * 16);

    // 5. 设置头尾指针
    // RDH = 0 (硬件负责移动)
    mmio_write32( pcbar->NET_DMA_BASE + 0x2810, 0); 
    // RDT = 7 (软件负责移动，指向网卡可写的最后一个槽位)
    mmio_write32( pcbar->NET_DMA_BASE + 0x2818, RX_DESC_COUNT - 1); 

    // 6. 配置接收控制寄存器 (RCTL)
    // Bit 1: EN, Bit 15: BAM, Bit 26: SECRC
    // 如果你发现收不到包，可以尝试加上 Bit 4 (MPE) 和 Bit 3 (UPE) 开启混杂模式测试
    unsigned int rctl = (1 << 1) | (1 << 15) | (1 << 26) | (1 << 4); 
    mmio_write32(pcbar->NET_DMA_BASE + 0x0100, rctl);
	
unsigned int ctrl = mmio_read32(pcbar->NET_DMA_BASE + 0x0000);
    ctrl |= (1 << 6);  // SLU
    ctrl &= ~(1 << 26); // 清除复位
    mmio_write32(pcbar->NET_DMA_BASE + 0x0000, ctrl);
mmio_write32(pcbar->NET_DMA_BASE + 0x00D0, 0x1F6DC);
    // 4. [合闸顺序]
    // A. 开启中断屏蔽 (写 IMS)
    mmio_write32(pcbar->NET_DMA_BASE + 0x00D0, 0x84); 
    
    // B. 清空一次状态 (读 ICR) - 这一步必须在写 IMS 之后，确保之后的信号能触发中断
    volatile uint32_t clean_icr = mmio_read32(pcbar->NET_DMA_BASE + 0x00C0);
    (void)clean_icr;

    // 5. 链路状态显示
    unsigned int status = mmio_read32(pcbar->NET_DMA_BASE + 0x0008);
    if (status & (1 << 1)) {
        putfonts8_ascAll(200, 340, "LINK IS UP!"); // 看到这个，说明硬件链路通了
    } else {
        putfonts8_ascAll(200, 340, "LINK IS DOWN...");
    }
	mmio_write32(pcbar->NET_DMA_BASE + 0x00C8, 0x84);
	return;
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
	if(pcbar->NET_DMA_BASE == 0){return;}
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
        mmio_write32( pcbar->NET_DMA_BASE + 0x2818, current_rx);
		putfonts8_ascAll(bbx, 280, "*");
        bbx+=8;
        current_rx = (current_rx + 1) % RX_DESC_COUNT;
    }
	return;
}
int cur_tx = 0;


void e1000_send_packet(void *packet, int len) {
	if(pcbar->NET_DMA_BASE == 0){return;}
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
    mmio_write32( pcbar->NET_DMA_BASE + 0x3818, cur_tx);

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