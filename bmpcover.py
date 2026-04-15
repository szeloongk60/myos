import sys
import os
import struct
from PIL import Image
import lz4.block

def pack_resource(input_path, output_path):
    try:
        # 1. 处理图片
        img = Image.open(input_path)
        # 统一转成 RGBA 模式 (每个像素 4 字节：R, G, B, A)
        img = img.convert('RGB')
      #  r, g, b, a = img.split()
        #black_a = Image.new('L', img.size, 0)
       # img_argb = Image.merge("RGBA", (black_a, r, g, b)) 
        
       # pixels = img_argb.tobytes()
        width, height = img.size
        # 这里的 pixels 就是宽 * 高 * 4 字节的原始数据了
        pixels = img.tobytes() 
        
        # 组装原始 Raw 数据: [宽2B][高2B][像素数据...]
        # 不再包含 768 字节的调色板
        raw_data = struct.pack('<HH', width, height) + pixels
        original_size = len(raw_data)

        # 2. LZ4 压缩
        compressed_data = lz4.block.compress(raw_data, store_size=False)
        compressed_size = len(compressed_data)

        # 3. 构建 12 字节头 (为了区分，你可以把 Magic 改成 LZ32)
        magic = b"LZ32" 
        header = struct.pack('<4sII', magic, original_size, compressed_size)

        # 4. 写入文件
        with open(output_path, 'wb') as f:
            f.write(header)
            f.write(compressed_data)

        print(f"--- 真彩色打包成功 ---")
        print(f"原始大小: {original_size} 字节 (不含调色板)")
        print(f"压缩后:   {compressed_size} 字节")

    except Exception as e:
        print(f"失败: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python pack.py <input_bmp> <output_lz4>")
    else:
        pack_resource(sys.argv[1], sys.argv[2])