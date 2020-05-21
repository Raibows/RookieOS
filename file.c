#include "bootpack.h"


void file_unzip_fat(int* fat, unsigned char* img) {
    /*
     * 对磁盘映像中的fat表先解压缩
     * fat是一个记录表，对于多于512字节的文件，他们的扇区是一个个链表，用fat可以查找下一个
     */
    int i, j = 0;
    for (i = 0; i < 2880; i += 2)
    {
        fat[i + 0] = (img[j + 0] | img[j + 1] << 8) & 0xfff;
        fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xfff;
        j += 3;
    }
    return;
}

void file_loadfile(int cluster_id, int size, char* buf, int* fat, unsigned char* img) {
    int i;
    while (1)
    {
        if (size <= 512)
        {
            for (i = 0; i < size; ++i) buf[i] = img[cluster_id * 512 + i];
            break;
        }
        for (i = 0; i < 512; ++i) buf[i] = img[cluster_id * 512 + i];
        size -= 512;
        buf += 512;
        cluster_id = fat[cluster_id];
    }
    return;
}

int file_locate(struct FileInfo* fileinfo, unsigned char* filename) {
    /*
     * filename = name7 + .1 + ext3共11个字节，要用空格补齐的
     * return -1 not found
     * return others file exits
     */
    assert(strlen(filename) == 11, "file_locate must have 11B");
    char flag = 0;
    int t1, t2;
    for (t1 = 0; t1 < 224; ++t1)
    {
        if (fileinfo[t1].name[0] == 0x00) break; // 已到头
        if (fileinfo[t1].name[0] == 0xe5) continue; // 0xe5代表被删除
        if ((fileinfo[t1].type & 0x18) == 0)
        {
            flag = 1;
            for (t2 = 0; t2 < 11; ++t2)
            {
                if (fileinfo[t1].name[t2] != filename[t2])
                {
                    flag = 0;
                    break;
                }
            }
            if (flag == 1) return t1;
        }
    }
    return -1;
}




