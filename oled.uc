import { open } from 'fs';
import { pack } from 'struct';

const CMD = {
    INIT: 0x01, CLEAR: 0x02, DISPLAY: 0x03, INVERT_DISPLAY: 0x04,
    SET_ROTATION: 0x05, DISPLAY_ROTATE: 0x06, DISPLAY_NORMAL: 0x07, INIT_COL_PG: 0x08,
    SET_CURSOR: 0x10, SET_TEXT_SIZE: 0x11, SET_TEXT_COLOR: 0x12, SET_TEXT_WRAP: 0x13, PRINT_STR: 0x14,
    DRAW_PIXEL: 0x20, DRAW_LINE: 0x21, DRAW_RECT: 0x22, FILL_RECT: 0x23,
    DRAW_CIRCLE: 0x24, FILL_CIRCLE: 0x25, DRAW_TRIANGLE: 0x26, FILL_TRIANGLE: 0x27,
    DRAW_ROUND_RECT: 0x28, FILL_ROUND_RECT: 0x29, DRAW_BITMAP: 0x2A,
    SCROLL_RIGHT: 0x30, SCROLL_LEFT: 0x31, SCROLL_DIAG_RIGHT: 0x32, SCROLL_DIAG_LEFT: 0x33, SCROLL_STOP: 0x34
};

function create_oled() {
    let fd = null;

    try {
        fd = open("/tmp/oled_fifo", 'w+');
    } catch (e) {
        fd = null;
    }

    function send(data) {
        if (!fd) {
            try {
                fd = open("/tmp/oled_fifo", 'w+');
            } catch (e) {
                fd = null;
                return false;
            }
        }

        try {
            let header = pack('BB', 0xAA, 0x55);
            fd.write(header + data);
            fd.flush();
            return true;
        } catch (e) {
            try { fd.close(); } catch(err) {}
            fd = null;
            return false;
        }
    }

    return {
        // ================= 系統與控制 =================
        init: function() { send(pack('B', CMD.INIT)); },
        clear: function() { send(pack('B', CMD.CLEAR)); },
        display: function() { send(pack('B', CMD.DISPLAY)); },
        invertDisplay: function(i) { send(pack('BB', CMD.INVERT_DISPLAY, i)); },
        setRotation: function(r) { send(pack('BB', CMD.SET_ROTATION, r)); },
        displayRotate: function() { send(pack('B', CMD.DISPLAY_ROTATE)); },
        displayNormal: function() { send(pack('B', CMD.DISPLAY_NORMAL)); },
        initColPg: function(c_s, c_e, p_s, p_e) { 
            send(pack('BBBBB', CMD.INIT_COL_PG, c_s, c_e, p_s, p_e)); 
        },
        
        // ================= 文字設定 =================
        // 注意：這裡將 1Byte 的 Opcode 與 2Bytes 的 short 分開 pack 來避免 Padding 錯位
        setCursor: function(x, y) { send(pack('B', CMD.SET_CURSOR) + pack('hh', x, y)); },
        setTextSize: function(size) { send(pack('BB', CMD.SET_TEXT_SIZE, size)); },
        setTextColor: function(color) { send(pack('B', CMD.SET_TEXT_COLOR) + pack('h', color)); },
        setTextWrap: function(wrap) { send(pack('BB', CMD.SET_TEXT_WRAP, wrap ? 1 : 0)); },
        printStr: function(str) {
            let len = length(str);
            send(pack('B', CMD.PRINT_STR) + pack('h', len) + str);
        },
        
        // ================= 繪圖 API =================
        drawPixel: function(x, y, c) { send(pack('B', CMD.DRAW_PIXEL) + pack('hhh', x, y, c)); },
        drawLine: function(x0, y0, x1, y1, c) { send(pack('B', CMD.DRAW_LINE) + pack('hhhhh', x0, y0, x1, y1, c)); },
        drawRect: function(x, y, w, h, c) { send(pack('B', CMD.DRAW_RECT) + pack('hhhhh', x, y, w, h, c)); },
        fillRect: function(x, y, w, h, c) { send(pack('B', CMD.FILL_RECT) + pack('hhhhh', x, y, w, h, c)); },
        drawCircle: function(x, y, r, c) { send(pack('B', CMD.DRAW_CIRCLE) + pack('hhhh', x, y, r, c)); },
        fillCircle: function(x, y, r, c) { send(pack('B', CMD.FILL_CIRCLE) + pack('hhhh', x, y, r, c)); },
        drawTriangle: function(x0, y0, x1, y1, x2, y2, c) { send(pack('B', CMD.DRAW_TRIANGLE) + pack('hhhhhhh', x0, y0, x1, y1, x2, y2, c)); },
        fillTriangle: function(x0, y0, x1, y1, x2, y2, c) { send(pack('B', CMD.FILL_TRIANGLE) + pack('hhhhhhh', x0, y0, x1, y1, x2, y2, c)); },
        drawRoundRect: function(x, y, w, h, r, c) { send(pack('B', CMD.DRAW_ROUND_RECT) + pack('hhhhhh', x, y, w, h, r, c)); },
        fillRoundRect: function(x, y, w, h, r, c) { send(pack('B', CMD.FILL_ROUND_RECT) + pack('hhhhhh', x, y, w, h, r, c)); },
        
        // ================= 捲動控制 =================
        scrollRight: function(start, stop) { send(pack('BBB', CMD.SCROLL_RIGHT, start, stop)); },
        scrollLeft: function(start, stop) { send(pack('BBB', CMD.SCROLL_LEFT, start, stop)); },
        scrollDiagRight: function(start, stop) { send(pack('BBB', CMD.SCROLL_DIAG_RIGHT, start, stop)); },
        scrollDiagLeft: function(start, stop) { send(pack('BBB', CMD.SCROLL_DIAG_LEFT, start, stop)); },
        scrollStop: function() { send(pack('B', CMD.SCROLL_STOP)); },
        
        close: function() { 
            if (fd) {
                try { fd.close(); } catch(e) {}
                fd = null;
            }
        }
    };
}

export { create_oled };