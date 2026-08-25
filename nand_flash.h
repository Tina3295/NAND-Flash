#ifndef NAND_FLASH_H
#define NAND_FLASH_H

#define PAGE_SIZE       4096  //每個 Page 4096 bytes
#define PAGES_PER_BLOCK 64  //每個 Block 64 個 Page
#define BLOCK_COUNT     128  //128 個 Block

#define NAND_OK                0
#define NAND_ERR_INVALID      -1  //傳入的參數不合法
#define NAND_ERR_BAD_BLOCK    -2  //這個 Block 是壞的
#define NAND_ERR_PROGRAM_FAIL -3  //這個 Page 已經寫過了，不能再次 Program

extern unsigned char nand[BLOCK_COUNT][PAGES_PER_BLOCK][PAGE_SIZE];

void nand_init(void);
int nand_read(int block, int page, unsigned char *buf);
int nand_program(int block, int page, const unsigned char *buf);
int nand_erase(int block);
int nand_mark_bad_block(int block);
int nand_is_bad_block(int block);
int nand_get_program_count(int block, unsigned int *count);
int nand_get_erase_count(int block, unsigned int *count);

#endif
