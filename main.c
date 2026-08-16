#include "nand_flash.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    unsigned char page_data[PAGE_SIZE];
    unsigned char read_back[PAGE_SIZE];

    nand_init();

    memset(page_data, 0xA5, sizeof(page_data));

    // 把 page_data 寫到 Block 0、Page 0
    if (nand_program(0, 0, page_data) != 0) {
        puts("nand_program failed");
        return 1;
    }

    // 從 Block 0 / Page 0 把資料讀到 read_back
    if (nand_read(0, 0, read_back) != 0) {
        puts("nand_read failed");
        return 1;
    }

    // 比對寫入的資料與讀回的資料是否一致
    if (memcmp(page_data, read_back, PAGE_SIZE) != 0) {
        puts("data mismatch after program/read");
        return 1;
    }

    if (nand_erase(0) != 0) {
        puts("nand_erase failed");
        return 1;
    }

    memset(read_back, 0x00, sizeof(read_back));
    if (nand_read(0, 0, read_back) != 0) {
        puts("nand_read after erase failed");
        return 1;
    }

    for (int i = 0; i < 16; ++i) {
        if (read_back[i] != 0xFF) {
            puts("erase did not restore erased state");
            return 1;
        }
    }

    puts("NAND flash simulation OK");
    return 0;
}
