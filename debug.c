#include "bootpack.h"
#include "string.h"

struct Sheet* sht;

void debug_init(struct Sheet* sht_back) {
    sht = sht_back;
    return;
}

void debug_print(char* info) {
    char prefix[50] = "DEBUG:  ";
    strcat(prefix, info);
    putfonts8_asc_sht(sht, 0, 200, COL8_FFFFFF, COL8_000000, prefix);
    return;
}
    
    
    
    
    
    