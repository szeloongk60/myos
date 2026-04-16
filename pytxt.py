import struct

def pack_moxiang_data(input_txt, output_dat):
    with open(input_txt, 'r', encoding='gbk') as f:
        lines = [line.strip() for line in f if line.strip()]

    pure_texts = []
    for line in lines:
        # 去掉开头的数字索引（例如 "1 请输入" -> "请输入"）
        parts = line.split(maxsplit=1)
        pure_texts.append(parts[1] if len(parts) > 1 else parts[0])

    count = len(pure_texts)
    # 编码为 GBK 并加上 \0
    encoded_texts = [t.encode('gbk') + b'\x00' for t in pure_texts]
    
    # 计算偏移量
    # 头部大小 = 4字节(总数) + count * 4字节(每个偏移量)
    header_size = 4 + (count * 4)
    offsets = []
    current_offset = header_size
    for t in encoded_texts:
        offsets.append(current_offset)
        current_offset += len(t)

    # 开始写入二进制文件
    with open(output_dat, 'wb') as f:
        # 1. 写入总条数 (Little Endian)
        f.write(struct.pack('<I', count))
        # 2. 写入偏移量表
        for off in offsets:
            f.write(struct.pack('<I', off))
        # 3. 写入字符串数据
        for t in encoded_texts:
            f.write(t)

    print(f"打包完成！文件: {output_dat}, 共 {count} 条数据。")

pack_moxiang_data('imgchat.txt', 'imgchat.dat')