# fat12_reader.py
import struct

IMG_FILE = "haribote.img"

# FAT12 文件系统解析类
class FAT12:
    def __init__(self, img_file):
        self.img_file = img_file
        self.read_boot_sector()
        self.read_fat_table()
        self.read_root_dir()

    # 读取 Boot Sector
    def read_boot_sector(self):
        with open(self.img_file, "rb") as f:
            f.seek(0)
            bs = f.read(512)
            self.bytes_per_sector = struct.unpack("<H", bs[11:13])[0]
            self.sectors_per_cluster = bs[13]
            self.reserved_sectors = struct.unpack("<H", bs[14:16])[0]
            self.num_fats = bs[16]
            self.max_root_dir_entries = struct.unpack("<H", bs[17:19])[0]
            self.sectors_per_fat = struct.unpack("<H", bs[22:24])[0]
            self.root_dir_sectors = ((self.max_root_dir_entries * 32) + (self.bytes_per_sector - 1)) // self.bytes_per_sector
            self.data_region_start = (self.reserved_sectors + self.num_fats * self.sectors_per_fat + self.root_dir_sectors) * self.bytes_per_sector
            self.cluster_size = self.sectors_per_cluster * self.bytes_per_sector
            print("=== Boot Sector 参数 ===")
            print(f"bytes_per_sector      : {self.bytes_per_sector}")
            print(f"sectors_per_cluster   : {self.sectors_per_cluster}")
            print(f"reserved_sectors      : {self.reserved_sectors}")
            print(f"num_fats              : {self.num_fats}")
            print(f"max_root_dir_entries  : {self.max_root_dir_entries}")
            print(f"sectors_per_fat       : {self.sectors_per_fat}")
            print(f"root_dir_sectors      : {self.root_dir_sectors}")
            print(f"data_region_start     : {self.data_region_start} (字节偏移)")
            print(f"cluster_size          : {self.cluster_size} (字节)")
            print("=======================")

    # 读取 FAT 表（FAT12，每项12位）
    def read_fat_table(self):
        with open(self.img_file, "rb") as f:
            f.seek(self.reserved_sectors * self.bytes_per_sector)
            fat_bytes = f.read(self.sectors_per_fat * self.bytes_per_sector)
            self.fat_bytes = fat_bytes

    # 获取 FAT12 簇的下一个簇编号
    def get_fat12_entry(self, n):
        # 每簇 12 位 → 1.5 字节
        byte_index = n + n // 2
        if n % 2 == 0:
            val = self.fat_bytes[byte_index] + ((self.fat_bytes[byte_index + 1] & 0x0F) << 8)
        else:
            val = ((self.fat_bytes[byte_index] >> 4) & 0x0F) + (self.fat_bytes[byte_index + 1] << 4)
        return val

    # 读取根目录
    def read_root_dir(self):
        with open(self.img_file, "rb") as f:
            f.seek((self.reserved_sectors + self.num_fats * self.sectors_per_fat) * self.bytes_per_sector)
            self.root_dir = []
            for _ in range(self.max_root_dir_entries):
                entry = f.read(32)
                if entry[0] == 0x00:
                    break
                if entry[0] == 0xE5:
                    continue
                filename = entry[0:8].decode('ascii').rstrip()
                ext = entry[8:11].decode('ascii').rstrip()
                attr = entry[11]
                start_cluster = struct.unpack("<H", entry[26:28])[0]
                file_size = struct.unpack("<I", entry[28:32])[0]
                if filename:
                    self.root_dir.append({
                        "name": f"{filename}.{ext}" if ext else filename,
                        "attr": attr,
                        "start_cluster": start_cluster,
                        "size": file_size
                    })

    # 列出根目录文件
    def list_files(self):
        for i, f in enumerate(self.root_dir):
            print(f"{i}: {f['name']} (size: {f['size']} bytes, start cluster: {f['start_cluster']})")

    # 读取文件内容
    def read_file(self, file_index):
        file_entry = self.root_dir[file_index]
        clusters = []
        cluster = file_entry["start_cluster"]
        # FAT12 EOF = 0xFF8 ~ 0xFFF
        while 0x002 <= cluster < 0xFF8:
            clusters.append(cluster)
            cluster = self.get_fat12_entry(cluster)

        file_data = bytearray()
        with open(self.img_file, "rb") as f:
            for c in clusters:
                offset = self.data_region_start + (c - 2) * self.cluster_size
                f.seek(offset)
                file_data.extend(f.read(self.cluster_size))
        return file_data[:file_entry["size"]]


# ===== 使用示例 =====
fat = FAT12(IMG_FILE)

print("根目录文件列表:")
fat.list_files()

file_index = int(input("输入要读取的文件编号: "))
data = fat.read_file(file_index)

output_name = fat.root_dir[file_index]["name"]
with open(output_name, "wb") as out:
    out.write(data)

print(f"已读取 {output_name}，大小 {len(data)} bytes")