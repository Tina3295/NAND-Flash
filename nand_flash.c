#include "nand_flash.h"

#include <stddef.h>
#include <string.h>

unsigned char nand[BLOCK_COUNT][PAGES_PER_BLOCK][PAGE_SIZE];
static unsigned int program_count[BLOCK_COUNT];
static unsigned int erase_count[BLOCK_COUNT];
static unsigned char bad_block[BLOCK_COUNT];

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

static int nand_page_is_programmed(int block, int page)
{
    int i;
    for (i = 0; i < PAGE_SIZE;i++) {
        if (nand[block][page][i] != 0xFF) {
            return 1;
        }
    }
    return 0;
}

void nand_init(void)
{
    memset(nand, 0xFF, sizeof(nand));
    memset(program_count, 0, sizeof(program_count));
    memset(erase_count, 0, sizeof(erase_count));
    memset(bad_block, 0, sizeof(bad_block));
}

// 從 NAND 某個 Page 讀資料到 buf
int nand_read(int block, int page, unsigned char *buf)
{
    if (!buf || !nand_valid_block(block) || !nand_valid_page(page)) {
        return NAND_ERR_INVALID;
    }

    if (bad_block[block]) {
        return NAND_ERR_BAD_BLOCK;
    }

    // 讀取資料
    memcpy(buf, nand[block][page], PAGE_SIZE);
    return NAND_OK;
}

// 將 buf 的資料寫入 NAND 某個 Page
int nand_program(int block, int page, const unsigned char *buf)
{
    if (!buf || !nand_valid_block(block) || !nand_valid_page(page)) {
        return NAND_ERR_INVALID;
    }

    if (bad_block[block]) {
        return NAND_ERR_BAD_BLOCK;
    }

    if (nand_page_is_programmed(block, page)) {
        return NAND_ERR_PROGRAM_FAIL;
    }

    memcpy(nand[block][page], buf, PAGE_SIZE);
    ++program_count[block];
    return NAND_OK;
}

// 擦除整個 Block，將所有 Page 設為 0xFF
int nand_erase(int block)
{
    if (!nand_valid_block(block)) {
        return NAND_ERR_INVALID;
    }

    if (bad_block[block]) {
        return NAND_ERR_BAD_BLOCK;
    }

    memset(nand[block], 0xFF, sizeof(nand[block]));
    ++erase_count[block];
    return NAND_OK;
}

int nand_mark_bad_block(int block)
{
    if (!nand_valid_block(block)) {
        return NAND_ERR_INVALID;
    }

    bad_block[block] = 1;
    return NAND_OK;
}

int nand_is_bad_block(int block)
{
    if (!nand_valid_block(block)) {
        return NAND_ERR_INVALID;
    }

    return bad_block[block] ? 1 : 0;
}

int nand_get_program_count(int block, unsigned int *count)
{
    if (!count || !nand_valid_block(block)) {
        return NAND_ERR_INVALID;
    }

    *count = program_count[block];
    return NAND_OK;
}

int nand_get_erase_count(int block, unsigned int *count)
{
    if (!count || !nand_valid_block(block)) {
        return NAND_ERR_INVALID;
    }

    *count = erase_count[block];
    return NAND_OK;
}
