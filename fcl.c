#include "fcl.h"

int fcl_submit(FlashCommand cmd, int block, int page, void *buffer)
{
    if (cmd == FLASH_CMD_NAND_READ) {
        return nand_read(block, page, (unsigned char *)buffer);
    }
    if (cmd == FLASH_CMD_NAND_PROGRAM) {
        return nand_program(block, page, (const unsigned char *)buffer);
    }
    if (cmd == FLASH_CMD_NAND_ERASE) {
        return nand_erase(block);
    }
    return NAND_ERR_INVALID;
}

int fcl_read(int block, int page, unsigned char *buffer)
{
    return fcl_submit(FLASH_CMD_NAND_READ, block, page, buffer);
}

// 這個 buffer 可以當成 void * 傳給 fcl_submit()
int fcl_program(int block, int page, const unsigned char *buffer)
{
    return fcl_submit(FLASH_CMD_NAND_PROGRAM, block, page, (void *)buffer);
}

int fcl_erase(int block)
{
    return fcl_submit(FLASH_CMD_NAND_ERASE, block, 0, (void *)0);
}

int fcl_is_bad_block(int block)
{
    return nand_is_bad_block(block);
}
