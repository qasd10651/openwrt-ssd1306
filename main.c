#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "I2C.h"
#include "SSD1306_OLED.h"

#define FIFO_PATH "/tmp/oled_fifo"

int main() {
    // 初始化
    init_i2c_dev("/dev/i2c-0", 0x3C);
    display_Init_seq();
    clearDisplay();
    Display();

    mkfifo(FIFO_PATH, 0666);
    printf("OLED 全功能守護行程已啟動...\n");

    char buffer[512]; // 加大緩衝區以容納長字串

    while (1) {
        FILE *fp = fopen(FIFO_PATH, "r");
        if (!fp) continue;

        int need_display = 0;

        while (fgets(buffer, sizeof(buffer), fp)) {
            char cmd[32];
            // 先讀取第一個單字作為指令
            if (sscanf(buffer, "%31s", cmd) != 1) continue;

            int x, y, x1, y1, x2, y2, w, h, r, color, size, start, stop;
            char str_arg[256];

            // ================= [系統與控制指令] =================
            if (strcmp(cmd, "CLEAR") == 0) {
                clearDisplay();[cite: 3]
                need_display = 1;
            } 
            else if (strcmp(cmd, "DISPLAY") == 0) {
                Display();[cite: 3]
                need_display = 0; 
            }
            else if (strcmp(cmd, "INVERT") == 0) {
                sscanf(buffer, "%*s %d", &x);
                invertDisplay((unsigned char)x); // 1 反轉, 0 正常
            }
            else if (strcmp(cmd, "ROTATION") == 0) {
                sscanf(buffer, "%*s %d", &x);
                setRotation((unsigned char)x); // 0~3[cite: 3]
            }

            // ================= [文字相關指令] =================
            else if (strcmp(cmd, "CURSOR") == 0) {
                sscanf(buffer, "%*s %d %d", &x, &y);
                setCursor(x, y);[cite: 3]
            }
            else if (strcmp(cmd, "TEXTSIZE") == 0) {
                sscanf(buffer, "%*s %d", &size);
                setTextSize((unsigned char)size);[cite: 3]
            }
            else if (strcmp(cmd, "PRINT") == 0) {
                // %[^\n] 代表讀取直到遇到換行符號為止，保留空格
                sscanf(buffer, "%*s %[^\n]", str_arg);
                print_str((unsigned char*)str_arg);[cite: 3]
                need_display = 1;
            }

            // ================= [基本圖形指令] =================
            else if (strcmp(cmd, "PIXEL") == 0) {
                sscanf(buffer, "%*s %d %d %d", &x, &y, &color);
                drawPixel(x, y, color);[cite: 3]
                need_display = 1;
            }
            else if (strcmp(cmd, "LINE") == 0) {
                sscanf(buffer, "%*s %d %d %d %d %d", &x, &y, &x1, &y1, &color);
                drawLine(x, y, x1, y1, color);[cite: 3]
                need_display = 1;
            }
            else if (strcmp(cmd, "RECT") == 0) {
                sscanf(buffer, "%*s %d %d %d %d %d", &x, &y, &w, &h, &color);
                drawRect(x, y, w, h, color);[cite: 3]
                need_display = 1;
            }
            else if (strcmp(cmd, "FILLRECT") == 0) {
                sscanf(buffer, "%*s %d %d %d %d %d", &x, &y, &w, &h, &color);
                fillRect(x, y, w, h, color);[cite: 3]
                need_display = 1;
            }

            // ================= [進階圖形指令] =================
            else if (strcmp(cmd, "CIRCLE") == 0) {
                sscanf(buffer, "%*s %d %d %d %d", &x, &y, &r, &color);
                drawCircle(x, y, r, color);[cite: 3]
                need_display = 1;
            }
            else if (strcmp(cmd, "FILLCIRCLE") == 0) {
                sscanf(buffer, "%*s %d %d %d %d", &x, &y, &r, &color);
                fillCircle(x, y, r, color);[cite: 3]
                need_display = 1;
            }
            else if (strcmp(cmd, "TRIANGLE") == 0) {
                sscanf(buffer, "%*s %d %d %d %d %d %d %d", &x, &y, &x1, &y1, &x2, &y2, &color);
                drawTriangle(x, y, x1, y1, x2, y2, color);[cite: 3]
                need_display = 1;
            }
            else if (strcmp(cmd, "ROUNDRECT") == 0) {
                sscanf(buffer, "%*s %d %d %d %d %d %d", &x, &y, &w, &h, &r, &color);
                drawRoundRect(x, y, w, h, r, color);[cite: 3]
                need_display = 1;
            }

            // ================= [硬體捲動指令] =================
            else if (strcmp(cmd, "SCROLL_R") == 0) {
                sscanf(buffer, "%*s %d %d", &start, &stop);
                startscrollright((unsigned char)start, (unsigned char)stop);[cite: 3]
            }
            else if (strcmp(cmd, "SCROLL_STOP") == 0) {
                stopscroll();[cite: 3]
            }
        }
        
        fclose(fp);

        if (need_display) {
            Display(); 
        }
    }
    return 0;
}