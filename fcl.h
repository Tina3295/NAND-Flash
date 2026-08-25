#ifndef FCL_H
#define FCL_H

#include "nand_flash.h"

typedef enum FlashCommand {
    FLASH_CMD_NAND_READ = 0,
    FLASH_CMD_NAND_PROGRAM = 1,
    FLASH_CMD_NAND_ERASE = 2
} FlashCommand;

int fcl_submit(FlashCommand cmd, int block, int page, void *buffer);
int fcl_read(int block, int page, unsigned char *buffer);
int fcl_program(int block, int page, const unsigned char *buffer);
int fcl_erase(int block);
int fcl_is_bad_block(int block);

#endif
