#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "I2C.h"
#include "SSD1306_OLED.h"

#define FIFO_PATH "/tmp/oled_fifo"

// 定義二進制操作碼 (Opcodes)
enum {
    CMD_INIT      = 0x01,
    CMD_CLEAR     = 0x02,
    CMD_DISPLAY   = 0x03,
    CMD_CURSOR    = 0x04, // 參數: x(1), y(1)
    CMD_TEXTCOLOR = 0x05, // 參數: color(1)
    CMD_TEXTSIZE  = 0x06, // 參數: size(1)
    CMD_PRINT     = 0x07, // 參數: len(1), string(len)
    CMD_RECT      = 0x08, // 參數: x(1), y(1), w(1), h(1), color(1)
    CMD_BITMAP    = 0x09  // 參數: x(1), y(1), w(1), h(1), color(1), data(w*h/8)
};

// 輔助函數：確保完整讀取指定長度的二進制資料
void read_exact(int fd, void *buf, size_t count) {
    size_t total = 0;
    while (total < count) {
        ssize_t n = read(fd, (char *)buf + total, count - total);
        if (n <= 0) break;
        total += n;
    }
}

int main() {
    // 啟動時不初始化 I2C，僅建立 FIFO
    mkfifo(FIFO_PATH, 0666);
    int fd = open(FIFO_PATH, O_RDONLY);
    if (fd < 0) return -1;

    unsigned char cmd;
    unsigned char args[8];
    int i2c_initialized = 0;

    while (1) {
        // 讀取 1 Byte 的操作碼 (若無寫入者會阻塞等待，無消耗)
        if (read(fd, &cmd, 1) <= 0) {
            close(fd);
            fd = open(FIFO_PATH, O_RDONLY); // 重新阻塞監聽
            continue;
        }

        switch(cmd) {
            case CMD_INIT:
                if (!i2c_initialized) {
                    init_i2c_dev("/dev/i2c-0", 0x3C); //[cite: 4]
                    i2c_initialized = 1;
                }
                display_Init_seq(); //[cite: 2]
                setTextColor(WHITE); //[cite: 3]
                break;
                
            case CMD_CLEAR:
                clearDisplay(); //[cite: 2]
                break;
                
            case CMD_DISPLAY:
                Display(); //[cite: 2]
                break;
                
            case CMD_CURSOR:
                read_exact(fd, args, 2);
                setCursor(args[0], args[1]); //[cite: 3]
                break;
                
            case CMD_TEXTCOLOR:
                read_exact(fd, args, 1);
                setTextColor(args[0]); //[cite: 3]
                break;
                
            case CMD_TEXTSIZE:
                read_exact(fd, args, 1);
                setTextSize(args[0]); //[cite: 3]
                break;
                
            case CMD_PRINT:
                {
                    unsigned char len;
                    read_exact(fd, &len, 1); // 先讀取字串長度
                    char *str = malloc(len + 1);
                    read_exact(fd, str, len);
                    str[len] = '\0'; // 確保字串結尾
                    print_str((unsigned char*)str); //[cite: 3]
                    free(str);
                }
                break;
                
            case CMD_RECT:
                read_exact(fd, args, 5);
                drawRect(args[0], args[1], args[2], args[3], args[4]); //[cite: 3]
                break;
        }
    }
    return 0;
}
