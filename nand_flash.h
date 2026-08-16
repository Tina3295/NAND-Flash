#ifndef NAND_FLASH_H
#define NAND_FLASH_H

#define PAGE_SIZE       4096  //每個 Page 4096 bytes
#define PAGES_PER_BLOCK 64  //每個 Block 64 個 Page
#define BLOCK_COUNT     128  //128 個 Block

extern unsigned char nand[BLOCK_COUNT][PAGES_PER_BLOCK][PAGE_SIZE];

void nand_init(void);
int nand_read(int block, int page, unsigned char *buf);
int nand_program(int block, int page, const unsigned char *buf);
int nand_erase(int block);

#endif
