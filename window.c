#include "bootpack.h"
#include "string.h"

extern int XSIZE;
extern int YSIZE;


void putfonts8_asc_sht(struct Sheet* sht, int x, int y, int c, int bc, char* s) {
    /*
     * c 指字体颜色
     * bc 背景颜色, bc = -1时不填充背景
     */
    int l = strlen(s);
    if (bc != -1) boxfill8(sht->buf, sht->bxsize, bc, x, y, x + l * 8 - 1, y + 15);
    putfonts8_asc(sht->buf, sht->bxsize, x, y, c, s);
    sheet_refresh(sht, x, y, x + l * 8, y + 16);
    return;
}

void make_close_button(unsigned char* buf, int xsize) {
    static char closebtn[14][16] = {
            "OOOOOOOOOOOOOOO@",
            "OQQQQQQQQQQQQQ$@",
            "OQQQQQQQQQQQQQ$@",
            "OQQQ@@QQQQ@@QQ$@",
            "OQQQQ@@QQ@@QQQ$@",
            "OQQQQQ@@@@QQQQ$@",
            "OQQQQQQ@@QQQQQ$@",
            "OQQQQQ@@@@QQQQ$@",
            "OQQQQ@@QQ@@QQQ$@",
            "OQQQ@@QQQQ@@QQ$@",
            "OQQQQQQQQQQQQQ$@",
            "OQQQQQQQQQQQQQ$@",
            "O$$$$$$$$$$$$$$@",
            "@@@@@@@@@@@@@@@@"
    };
    char c;
    int x, y;
    for (y = 0; y < 14; y++)
    {
        for (x = 0; x < 16; x++)
        {
            c = closebtn[y][x];
            switch (c)
            {
                case '@':
                    c = COL8_000000;
                    break;
                case '$':
                    c = COL8_848484;
                    break;
                case 'Q':
                    c = COL8_C6C6C6;
                    break;
                default:
                    c = COL8_FFFFFF;
                    break;
            }
            buf[(2 + y) * xsize + (xsize - 18 + x)] = c;
        }
    }
    return;
}

void make_title(struct Sheet* sht, char* title, char is_act, char is_close_btn) {
    int tc, tbc;
    if (is_act != 0)
    {
        tc = COL8_FFFFFF;
        tbc = COL8_000084;
    }
    else
    {
        tc = COL8_C6C6C6;
        tbc = COL8_848484;
    }
    boxfill8(sht->buf, sht->bxsize, tbc, 0, 0, sht->bxsize - 1, 15);
    putfonts8_asc_sht(sht, sht->bxsize / 2 - strlen(title) * 4, 0, tc, tbc, title);
    if (is_close_btn != 0) make_close_button(sht->buf, sht->bxsize);
    sheet_refresh(sht, 0, 0, sht->bxsize, 16);
    return;
}

void make_window(struct MemMan* man, struct Sheet* sht, int xsize, int ysize, int color, int col_inv, char* title, char is_act) {
    xsize = check_pos(xsize, 80, XSIZE);
    ysize = check_pos(ysize, 32, YSIZE);
    unsigned char* buf = (unsigned char*) memman_alloc_4kB(man, xsize * ysize);
    sheet_setbuf(sht, buf, xsize, ysize, col_inv);
    boxfill8(sht->buf, sht->bxsize, color, 0, 16, xsize - 1, ysize - 1);
    make_title(sht, title, is_act, 1);
    sht->cursor_y_high = (ysize - 16) / LINE_GAP * LINE_GAP;
    sht->cursor_x_high = xsize / 8 * 8 - 8;
    sht->cursor_y_low = 16;
    return;
}










