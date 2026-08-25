#include "ftl.h"

#include <stddef.h>

static MappingEntry mapping_table[MAX_LBA];
static int next_block;
static int next_page;

static int ftl_valid_lba(int lba)
{
    return (lba >= 0 && lba < MAX_LBA);
}

static int ftl_allocate_physical_page(int *block, int *page)
{
    int b;
    int p;

    b = next_block;
    p = next_page;

    while (b < BLOCK_COUNT) {
        if (nand_is_bad_block(b) == 1) {
            b++;
            p = 0;
            continue;
        }

        while (p < PAGES_PER_BLOCK) {
            *block = b;
            *page = p;
            next_block = b;
            next_page = p + 1;
            if (next_page >= PAGES_PER_BLOCK) {
                next_block++;
                next_page = 0;
            }
            return FTL_OK;
        }

        b++;
        p = 0;
    }

    return FTL_ERR_NO_SPACE;
}

void ftl_init(void)
{
    int i;

    nand_init();
    next_block = 0;
    next_page = 0;

    for (i = 0; i < MAX_LBA; i++) {
        mapping_table[i].lba = i;
        mapping_table[i].block = -1;
        mapping_table[i].page = -1;
        mapping_table[i].valid = 0;
    }
}

int ftl_write(int lba, const unsigned char *buf)
{
    int block;
    int page;
    int rc;

    if (!buf || !ftl_valid_lba(lba)) {
        return FTL_ERR_INVALID;
    }

    for (;;) {
        rc = ftl_allocate_physical_page(&block, &page);
        if (rc != FTL_OK) {
            return rc;
        }

        rc = nand_program(block, page, buf);
        if (rc == NAND_OK) {
            break;
        }
        if (rc != NAND_ERR_PROGRAM_FAIL) {
            return FTL_ERR_NAND;
        }
    }

    mapping_table[lba].block = block;
    mapping_table[lba].page = page;
    mapping_table[lba].valid = 1;

    return FTL_OK;
}

int ftl_read(int lba, unsigned char *buf)
{
    if (!buf || !ftl_valid_lba(lba)) {
        return FTL_ERR_INVALID;
    }

    if (!mapping_table[lba].valid) {
        return FTL_ERR_UNMAPPED;
    }

    if (nand_read(mapping_table[lba].block, mapping_table[lba].page, buf) != NAND_OK) {
        return FTL_ERR_NAND;
    }

    return FTL_OK;
}

int ftl_get_mapping(int lba, int *block, int *page)
{
    if (!block || !page || !ftl_valid_lba(lba)) {
        return FTL_ERR_INVALID;
    }

    if (!mapping_table[lba].valid) {
        return FTL_ERR_UNMAPPED;
    }

    *block = mapping_table[lba].block;
    *page = mapping_table[lba].page;
    return FTL_OK;
}
