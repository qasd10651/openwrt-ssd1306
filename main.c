#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include "I2C.h"
#include "SSD1306_OLED.h"

#define FIFO_PATH "/tmp/oled_fifo"

// 暴力展開所有 API 的 Opcode
enum {
    // 系統與控制 (0x01 ~ 0x0F)
    CMD_INIT = 0x01,
    CMD_CLEAR = 0x02,
    CMD_DISPLAY = 0x03,
    CMD_INVERT_DISPLAY = 0x04,
    CMD_SET_ROTATION = 0x05,
    CMD_DISPLAY_ROTATE = 0x06,
    CMD_DISPLAY_NORMAL = 0x07,
    CMD_INIT_COL_PG = 0x08,

    // 文字相關 (0x10 ~ 0x1F)
    CMD_SET_CURSOR = 0x10,
    CMD_SET_TEXT_SIZE = 0x11,
    CMD_SET_TEXT_COLOR = 0x12,
    CMD_SET_TEXT_WRAP = 0x13,
    CMD_PRINT_STR = 0x14,

    // 基本與進階圖形 (0x20 ~ 0x2F)
    CMD_DRAW_PIXEL = 0x20,
    CMD_DRAW_LINE = 0x21,
    CMD_DRAW_RECT = 0x22,
    CMD_FILL_RECT = 0x23,
    CMD_DRAW_CIRCLE = 0x24,
    CMD_FILL_CIRCLE = 0x25,
    CMD_DRAW_TRIANGLE = 0x26,
    CMD_FILL_TRIANGLE = 0x27,
    CMD_DRAW_ROUND_RECT = 0x28,
    CMD_FILL_ROUND_RECT = 0x29,
    CMD_DRAW_BITMAP = 0x2A,

    // 硬體捲動 (0x30 ~ 0x3F)
    CMD_SCROLL_RIGHT = 0x30,
    CMD_SCROLL_LEFT = 0x31,
    CMD_SCROLL_DIAG_RIGHT = 0x32,
    CMD_SCROLL_DIAG_LEFT = 0x33,
    CMD_SCROLL_STOP = 0x34
};

// 輔助函數：精確讀取資料
void read_exact(int fd, void *buf, size_t count) {
    size_t total = 0;
    while (total < count) {
        ssize_t n = read(fd, (char *)buf + total, count - total);
        if (n <= 0) break;
        total += n;
    }
}

// 輔助函數：讀取 2 Bytes 的 short (適用於座標 x, y, w, h)
short read_short(int fd) {
    short val;
    read_exact(fd, &val, sizeof(short));
    return val;
}

// 輔助函數：讀取 1 Byte 的 unsigned char
unsigned char read_byte(int fd) {
    unsigned char val;
    read_exact(fd, &val, 1);
    return val;
}

int main() {
    mkfifo(FIFO_PATH, 0666);
    int fd = open(FIFO_PATH, O_RDONLY);
    if (fd < 0) return -1;

    unsigned char cmd;
    int i2c_initialized = 0;

    while (1) {
        if (read(fd, &cmd, 1) <= 0) {
            close(fd);
            fd = open(FIFO_PATH, O_RDONLY);
            continue;
        }

        switch(cmd) {
            // ================= 系統與控制 =================
            case CMD_INIT:
                if (!i2c_initialized) {
                    init_i2c_dev(I2C_DEV0_PATH, 0x3C);
                    i2c_initialized = 1;
                }
                display_Init_seq();
                break;
            case CMD_CLEAR: clearDisplay(); break;
            case CMD_DISPLAY: Display(); break;
            case CMD_INVERT_DISPLAY: invertDisplay(read_byte(fd)); break;
            case CMD_SET_ROTATION: setRotation(read_byte(fd)); break;
            case CMD_DISPLAY_ROTATE: display_rotate(); break;
            case CMD_DISPLAY_NORMAL: display_normal(); break;
            case CMD_INIT_COL_PG: {
                unsigned char c_s = read_byte(fd), c_e = read_byte(fd);
                unsigned char p_s = read_byte(fd), p_e = read_byte(fd);
                Init_Col_PG_addrs(c_s, c_e, p_s, p_e);
                break;
            }

            // ================= 文字設定 =================
            case CMD_SET_CURSOR: {
                short x = read_short(fd), y = read_short(fd);
                setCursor(x, y); break;
            }
            case CMD_SET_TEXT_SIZE: setTextSize(read_byte(fd)); break;
            case CMD_SET_TEXT_COLOR: setTextColor(read_short(fd)); break;
            case CMD_SET_TEXT_WRAP: setTextWrap((bool)read_byte(fd)); break;
            case CMD_PRINT_STR: {
                short len = read_short(fd);
                char *str = malloc(len + 1);
                read_exact(fd, str, len);
                str[len] = '\0';
                print_str((unsigned char*)str);
                free(str);
                break;
            }

            // ================= 繪圖 API =================
            case CMD_DRAW_PIXEL: {
                short x = read_short(fd), y = read_short(fd), c = read_short(fd);
                drawPixel(x, y, c); break;
            }
            case CMD_DRAW_LINE: {
                short x0 = read_short(fd), y0 = read_short(fd);
                short x1 = read_short(fd), y1 = read_short(fd), c = read_short(fd);
                drawLine(x0, y0, x1, y1, c); break;
            }
            case CMD_DRAW_RECT: {
                short x = read_short(fd), y = read_short(fd);
                short w = read_short(fd), h = read_short(fd), c = read_short(fd);
                drawRect(x, y, w, h, c); break;
            }
            case CMD_FILL_RECT: {
                short x = read_short(fd), y = read_short(fd);
                short w = read_short(fd), h = read_short(fd), c = read_short(fd);
                fillRect(x, y, w, h, c); break;
            }
            case CMD_DRAW_CIRCLE: {
                short x = read_short(fd), y = read_short(fd);
                short r = read_short(fd), c = read_short(fd);
                drawCircle(x, y, r, c); break;
            }
            case CMD_FILL_CIRCLE: {
                short x = read_short(fd), y = read_short(fd);
                short r = read_short(fd), c = read_short(fd);
                fillCircle(x, y, r, c); break;
            }
            case CMD_DRAW_TRIANGLE:
            case CMD_FILL_TRIANGLE: {
                short x0 = read_short(fd), y0 = read_short(fd);
                short x1 = read_short(fd), y1 = read_short(fd);
                short x2 = read_short(fd), y2 = read_short(fd), c = read_short(fd);
                if (cmd == CMD_DRAW_TRIANGLE) drawTriangle(x0, y0, x1, y1, x2, y2, c);
                else fillTriangle(x0, y0, x1, y1, x2, y2, c);
                break;
            }
            case CMD_DRAW_ROUND_RECT:
            case CMD_FILL_ROUND_RECT: {
                short x = read_short(fd), y = read_short(fd);
                short w = read_short(fd), h = read_short(fd);
                short r = read_short(fd), c = read_short(fd);
                if (cmd == CMD_DRAW_ROUND_RECT) drawRoundRect(x, y, w, h, r, c);
                else fillRoundRect(x, y, w, h, r, c);
                break;
            }
            case CMD_DRAW_BITMAP: {
                short x = read_short(fd), y = read_short(fd);
                short w = read_short(fd), h = read_short(fd), c = read_short(fd);
                int bytes = ((w + 7) / 8) * h; // 計算 Bitmap 需要的位元組數
                unsigned char *bmp = malloc(bytes);
                read_exact(fd, bmp, bytes);
                drawBitmap(x, y, bmp, w, h, c);
                free(bmp);
                break;
            }

            // ================= 捲動控制 =================
            case CMD_SCROLL_RIGHT:
            case CMD_SCROLL_LEFT:
            case CMD_SCROLL_DIAG_RIGHT:
            case CMD_SCROLL_DIAG_LEFT: {
                unsigned char start = read_byte(fd), stop = read_byte(fd);
                if (cmd == CMD_SCROLL_RIGHT) startscrollright(start, stop);
                if (cmd == CMD_SCROLL_LEFT) startscrollleft(start, stop);
                if (cmd == CMD_SCROLL_DIAG_RIGHT) startscrolldiagright(start, stop);
                if (cmd == CMD_SCROLL_DIAG_LEFT) startscrolldiagleft(start, stop);
                break;
            }
            case CMD_SCROLL_STOP: stopscroll(); break;
        }
    }
    return 0;
}
