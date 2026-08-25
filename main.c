#include "nand_flash.h"
#include "ftl.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    unsigned char page_data[PAGE_SIZE];
    unsigned char read_back[PAGE_SIZE];
    FtlStats ftl_stats;
    unsigned int count;
    int i;
    int lba;
    int old_block;
    int old_page;
    int new_block;
    int new_page;

    nand_init();

    memset(page_data, 0xA5, sizeof(page_data));

    // 把 page_data 寫到 Block 0、Page 0
    if (nand_program(0, 0, page_data) != 0) {
        puts("nand_program failed");
        return 1;
    }

    // NAND 限制：同一個 page 未擦除前不能再 program 一次
    if (nand_program(0, 0, page_data) != NAND_ERR_PROGRAM_FAIL) {
        puts("expected program fail on rewriting programmed page");
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

    // 擦除後，讀回的資料應該都是 0xFF
    memset(read_back, 0x00, sizeof(read_back));
    if (nand_read(0, 0, read_back) != 0) {
        puts("nand_read after erase failed");
        return 1;
    }

    for (i = 0; i < 16; i++) {
        if (read_back[i] != 0xFF) {
            puts("erase did not restore erased state");
            return 1;
        }
    }

    if (nand_mark_bad_block(5) != NAND_OK) {
        puts("mark bad block failed");
        return 1;
    }

    if (nand_program(5, 0, page_data) != NAND_ERR_BAD_BLOCK) {
        puts("bad block program should fail");
        return 1;
    }

    for (i = 0; i < 152; i++) {
        if (nand_erase(0) != NAND_OK) {
            puts("erase block 0 failed");
            return 1;
        }
    }
    for (i = 0; i < 21; i++) {
        if (nand_erase(1) != NAND_OK) {
            puts("erase block 1 failed");
            return 1;
        }
    }
    for (i = 0; i < 149; i++) {
        if (nand_erase(2) != NAND_OK) {
            puts("erase block 2 failed");
            return 1;
        }
    }

    if (nand_get_erase_count(0, &count) != NAND_OK) {
        puts("get erase_count block 0 failed");
        return 1;
    }
    printf("Block 0 erase_count = %u\n", count);

    if (nand_get_erase_count(1, &count) != NAND_OK) {
        puts("get erase_count block 1 failed");
        return 1;
    }
    printf("Block 1 erase_count = %u\n", count);

    if (nand_get_erase_count(2, &count) != NAND_OK) {
        puts("get erase_count block 2 failed");
        return 1;
    }
    printf("Block 2 erase_count = %u\n", count);

    if (nand_get_program_count(0, &count) != NAND_OK) {
        puts("get program_count block 0 failed");
        return 1;
    }
    printf("Block 0 program_count = %u\n", count);

    ftl_init();

    if (nand_mark_bad_block(0) != NAND_OK) {
        puts("mark bad block 0 failed");
        return 1;
    }

    memset(page_data, 0x11, sizeof(page_data));
    if (ftl_write(100, page_data) != FTL_OK) {
        puts("ftl_write lba 100 v1 failed");
        return 1;
    }

    if (ftl_get_mapping(100, &old_block, &old_page) != FTL_OK) {
        puts("ftl_get_mapping lba 100 v1 failed");
        return 1;
    }

    if (old_block == 0) {
        puts("ftl should skip bad block 0");
        return 1;
    }

    if (ftl_read(100, read_back) != FTL_OK) {
        puts("ftl_read lba 100 v1 failed");
        return 1;
    }

    if (memcmp(page_data, read_back, PAGE_SIZE) != 0) {
        puts("ftl data mismatch v1");
        return 1;
    }

    memset(page_data, 0x22, sizeof(page_data));
    if (ftl_write(100, page_data) != FTL_OK) {
        puts("ftl_write lba 100 v2 failed");
        return 1;
    }

    if (ftl_get_mapping(100, &new_block, &new_page) != FTL_OK) {
        puts("ftl_get_mapping lba 100 v2 failed");
        return 1;
    }

    if (old_block == new_block && old_page == new_page) {
        puts("ftl overwrite should allocate new physical page");
        return 1;
    }

    if (ftl_read(100, read_back) != FTL_OK) {
        puts("ftl_read lba 100 v2 failed");
        return 1;
    }

    if (memcmp(page_data, read_back, PAGE_SIZE) != 0) {
        puts("ftl data mismatch v2");
        return 1;
    }

    if (ftl_read(101, read_back) != FTL_ERR_UNMAPPED) {
        puts("expected unmapped read error for lba 101");
        return 1;
    }

    printf("LBA 100 old PPA = (%d, %d)\n", old_block, old_page);
    printf("LBA 100 new PPA = (%d, %d)\n", new_block, new_page);

    ftl_init();

    for (lba = 0; lba < 8; lba++) {
        memset(page_data, lba + 1, sizeof(page_data));
        if (ftl_write(lba, page_data) != FTL_OK) {
            puts("ftl_write initial data failed");
            return 1;
        }
    }

    memset(page_data, 0xA1, sizeof(page_data));
    if (ftl_write(1, page_data) != FTL_OK) {
        puts("ftl rewrite lba 1 failed");
        return 1;
    }
    memset(page_data, 0xA3, sizeof(page_data));
    if (ftl_write(3, page_data) != FTL_OK) {
        puts("ftl rewrite lba 3 failed");
        return 1;
    }
    memset(page_data, 0xA5, sizeof(page_data));
    if (ftl_write(5, page_data) != FTL_OK) {
        puts("ftl rewrite lba 5 failed");
        return 1;
    }

    if (ftl_garbage_collect() != FTL_OK) {
        puts("manual garbage collection failed");
        return 1;
    }

    if (ftl_read(1, read_back) != FTL_OK || read_back[0] != 0xA1) {
        puts("gc verify lba 1 failed");
        return 1;
    }
    if (ftl_read(3, read_back) != FTL_OK || read_back[0] != 0xA3) {
        puts("gc verify lba 3 failed");
        return 1;
    }
    if (ftl_read(5, read_back) != FTL_OK || read_back[0] != 0xA5) {
        puts("gc verify lba 5 failed");
        return 1;
    }
    if (ftl_read(0, read_back) != FTL_OK || read_back[0] != 0x01) {
        puts("gc verify lba 0 failed");
        return 1;
    }

    if (ftl_get_stats(&ftl_stats) != FTL_OK) {
        puts("ftl_get_stats failed");
        return 1;
    }
    if (ftl_stats.gc_count == 0 ||
        ftl_stats.page_migration_count == 0 ||
        ftl_stats.valid_pages_copied == 0 ||
        ftl_stats.erase_count == 0) {
        puts("gc stats should be non-zero after garbage collection");
        return 1;
    }

    printf("GC count = %u\n", ftl_stats.gc_count);
    printf("Page migration count = %u\n", ftl_stats.page_migration_count);
    printf("Valid pages copied = %u\n", ftl_stats.valid_pages_copied);
    printf("Erase count (GC) = %u\n", ftl_stats.erase_count);
    printf("Write amplification = %.3f\n", ftl_get_write_amplification());
    puts("NAND flash + FTL + GC simulation OK");
    return 0;
}
