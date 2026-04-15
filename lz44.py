import lz4.block
import sys
import os

def compress_file(input_path, output_path):
    try:
        # 1. 读取未压缩的原始数据
        with open(input_path, 'rb') as f:
            data = f.read()
        
        # 2. 使用 LZ4 Block 模式压缩
        # store_size=True 会在压缩数据的开头存入 4 字节的原始大小
        # 这对你的 OS 很有用，因为解压时你需要知道要申请多大的内存
        compressed_data = lz4.block.compress(data, store_size=True)
        
        # 3. 写入文件
        with open(output_path, 'wb') as f:
            f.write(compressed_data)
            
        orig_size = len(data)
        comp_size = len(compressed_data)
        ratio = (comp_size / orig_size) * 100
        
        print(f"✅ 压缩完成!")
        print(f"原始大小: {orig_size} 字节")
        print(f"压缩后大小: {comp_size} 字节")
        print(f"压缩比: {ratio:.2f}%")
        
    except Exception as e:
        print(f"❌ 出错了: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("用法: python lz4_compress.py <输入文件> <输出文件>")
    else:
        compress_file(sys.argv[1], sys.argv[2])