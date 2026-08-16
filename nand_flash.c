#include "nand_flash.h"

#include <stddef.h>
#include <string.h>

unsigned char nand[BLOCK_COUNT][PAGES_PER_BLOCK][PAGE_SIZE];

// 檢查 Block 編號是否合法
static int nand_valid_block(int block)
{
    return (block >= 0 && block < BLOCK_COUNT);
}

// 檢查 Page 編號是否合法
static int nand_valid_page(int page)
{
    return (page >= 0 && page < PAGES_PER_BLOCK);
}

void nand_init(void)
{
    memset(nand, 0xFF, sizeof(nand));
}

// 從 NAND 某個 Page 讀資料到 buf
int nand_read(int block, int page, unsigned char *buf)
{
    if (!buf || !nand_valid_block(block) || !nand_valid_page(page)) {
        return -1;
    }

    memcpy(buf, nand[block][page], PAGE_SIZE);
    return 0;
}

// 將 buf 的資料寫入 NAND 某個 Page
int nand_program(int block, int page, const unsigned char *buf)
{
    if (!buf || !nand_valid_block(block) || !nand_valid_page(page)) {
        return -1;
    }

    memcpy(nand[block][page], buf, PAGE_SIZE);
    return 0;
}

// 擦除整個 Block，將所有 Page 設為 0xFF
int nand_erase(int block)
{
    if (!nand_valid_block(block)) {
        return -1;
    }

    memset(nand[block], 0xFF, sizeof(nand[block]));
    return 0;
}
