#include <stdio.h>
#include <string.h>

void translate_line(char *line) {
    int val;
	char tmp[100];
char *p = tmp; // p 永远指向当前字符串的末尾（即 \0 的位置）

// 模拟 case a: tmp = "123"
p = tmp; // 回到开头，相当于重置字符串
//strcpy(p, "123");

    // 1. 处理 SET R0, Val (将 R0 设为某个数值)
    if (strncmp(line, "SET R0, ", 8) == 0) {
    val = atoi(line + 8);

    strcpy(p, "[-]");
    p += 3;

    for (int i = 0; i < val; i++) {
        *p++ = '+';
    }
}

// 2. SET R1
else if (strncmp(line, "SET R1, ", 8) == 0) {
    val = atoi(line + 8);

    strcpy(p, ">[-]");
    p += 4;

    for (int i = 0; i < val; i++) {
        *p++ = '+';
    }

    *p++ = '<';   // 比 strcpy 更简单
}

// 3. OUT R0
else if (strncmp(line, "OUT R0", 6) == 0) {
    *p++ = '.';   // ✅ 正确写法（你原来这里错了）
}

// 4. OUT R1
else if (strncmp(line, "OUT R1", 6) == 0) {
    strcpy(p, ">.<");
    p += 3;
}
	printf(tmp);
}

int main() {

    char *line;
	line="SET R0, 3";
	
        translate_line(line);
		line="OUT R0";
		translate_line(line);
    return 0;
}