#include "ftl.h"
#include "fcl.h"

#include <stddef.h>
#include <string.h>

typedef enum PhysicalPageState {
    PAGE_FREE = 0,
    PAGE_VALID = 1,
    PAGE_INVALID = 2
} PhysicalPageState;

static MappingEntry mapping_table[MAX_LBA];
static unsigned char page_state[BLOCK_COUNT][PAGES_PER_BLOCK];
static int page_owner_lba[BLOCK_COUNT][PAGES_PER_BLOCK];
static int next_block;
static int next_page;
static FtlStats stats;

static int ftl_valid_lba(int lba)
{
    return (lba >= 0 && lba < MAX_LBA);
}

static int ftl_advance_cursor(void)
{
    next_page++;
    if (next_page >= PAGES_PER_BLOCK) {
        next_block++;
        if (next_block >= BLOCK_COUNT) {
            next_block = 0;
        }
        next_page = 0;
    }
    return FTL_OK;
}

static int ftl_allocate_free_page(int excluded_block, int *block, int *page)
{
    int scanned_pages;

    if (!block || !page) {
        return FTL_ERR_INVALID;
    }

    scanned_pages = 0;
    while (scanned_pages < (BLOCK_COUNT * PAGES_PER_BLOCK)) {
        int is_bad;

        if (next_block == excluded_block) {
            scanned_pages += (PAGES_PER_BLOCK - next_page);
            next_page = 0;
            next_block++;
            if (next_block >= BLOCK_COUNT) {
                next_block = 0;
            }
            continue;
        }

        is_bad = fcl_is_bad_block(next_block);
        if (is_bad < 0) {
            return FTL_ERR_NAND;
        }
        if (is_bad == 1) {
            scanned_pages += (PAGES_PER_BLOCK - next_page);
            next_page = 0;
            next_block++;
            if (next_block >= BLOCK_COUNT) {
                next_block = 0;
            }
            continue;
        }

        if (page_state[next_block][next_page] == PAGE_FREE) {
            *block = next_block;
            *page = next_page;
            ftl_advance_cursor();
            return FTL_OK;
        }

        ftl_advance_cursor();
        scanned_pages++;
    }

    return FTL_ERR_NO_SPACE;
}

static int ftl_block_score(int block, int *invalid_pages, int *valid_pages)
{
    int p;
    int invalid_count;
    int valid_count;
    int bad;

    if (!invalid_pages || !valid_pages) {
        return FTL_ERR_INVALID;
    }

    bad = fcl_is_bad_block(block);
    if (bad < 0) {
        return FTL_ERR_NAND;
    }
    if (bad == 1) {
        *invalid_pages = -1;
        *valid_pages = -1;
        return FTL_OK;
    }

    invalid_count = 0;
    valid_count = 0;
    for (p = 0; p < PAGES_PER_BLOCK; p++) {
        if (page_state[block][p] == PAGE_INVALID) {
            invalid_count++;
        } else if (page_state[block][p] == PAGE_VALID) {
            valid_count++;
        }
    }

    *invalid_pages = invalid_count;
    *valid_pages = valid_count;
    return FTL_OK;
}

static int ftl_select_gc_victim(int *victim_block)
{
    int b;
    int best_block;
    int best_invalid;
    int best_valid;

    if (!victim_block) {
        return FTL_ERR_INVALID;
    }

    best_block = -1;
    best_invalid = -1;
    best_valid = 0;

    for (b = 0; b < BLOCK_COUNT; b++) {
        int invalid_pages;
        int valid_pages;
        int rc;

        rc = ftl_block_score(b, &invalid_pages, &valid_pages);
        if (rc != FTL_OK) {
            return rc;
        }
        if (invalid_pages <= 0) {
            continue;
        }
        if (invalid_pages > best_invalid) {
            best_invalid = invalid_pages;
            best_valid = valid_pages;
            best_block = b;
            continue;
        }
        if (invalid_pages == best_invalid && valid_pages < best_valid) {
            best_valid = valid_pages;
            best_block = b;
        }
    }

    if (best_block < 0) {
        return FTL_ERR_NO_SPACE;
    }

    *victim_block = best_block;
    return FTL_OK;
}

void ftl_init(void)
{
    int i;
    int b;
    int p;

    nand_init();
    next_block = 0;
    next_page = 0;
    memset(&stats, 0, sizeof(stats));

    for (i = 0; i < MAX_LBA; i++) {
        mapping_table[i].lba = i;
        mapping_table[i].block = -1;
        mapping_table[i].page = -1;
        mapping_table[i].valid = 0;
    }

    for (b = 0; b < BLOCK_COUNT; b++) {
        for (p = 0; p < PAGES_PER_BLOCK; p++) {
            page_state[b][p] = PAGE_FREE;
            page_owner_lba[b][p] = -1;
        }
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

    stats.host_write_count++;

    for (;;) {
        rc = ftl_allocate_free_page(-1, &block, &page);
        if (rc == FTL_OK) {
            break;
        }
        if (rc != FTL_ERR_NO_SPACE) {
            return rc;
        }
        rc = ftl_garbage_collect();
        if (rc != FTL_OK) {
            return rc;
        }
    }

    rc = fcl_program(block, page, buf);
    if (rc != NAND_OK) {
        return FTL_ERR_NAND;
    }

    stats.nand_program_count++;

    if (mapping_table[lba].valid) {
        page_state[mapping_table[lba].block][mapping_table[lba].page] = PAGE_INVALID;
        page_owner_lba[mapping_table[lba].block][mapping_table[lba].page] = -1;
    }

    mapping_table[lba].block = block;
    mapping_table[lba].page = page;
    mapping_table[lba].valid = 1;

    page_state[block][page] = PAGE_VALID;
    page_owner_lba[block][page] = lba;

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

    if (fcl_read(mapping_table[lba].block, mapping_table[lba].page, buf) != NAND_OK) {
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

int ftl_trim(int lba)
{
    if (!ftl_valid_lba(lba)) {
        return FTL_ERR_INVALID;
    }
    if (!mapping_table[lba].valid) {
        return FTL_ERR_UNMAPPED;
    }

    page_state[mapping_table[lba].block][mapping_table[lba].page] = PAGE_INVALID;
    page_owner_lba[mapping_table[lba].block][mapping_table[lba].page] = -1;
    mapping_table[lba].block = -1;
    mapping_table[lba].page = -1;
    mapping_table[lba].valid = 0;
    return FTL_OK;
}

int ftl_garbage_collect(void)
{
    int victim_block;
    int p;
    int rc;

    rc = ftl_select_gc_victim(&victim_block);
    if (rc != FTL_OK) {
        return rc;
    }

    for (p = 0; p < PAGES_PER_BLOCK; p++) {
        if (page_state[victim_block][p] == PAGE_VALID) {
            unsigned char temp[PAGE_SIZE];
            int lba;
            int new_block;
            int new_page;

            lba = page_owner_lba[victim_block][p];
            if (!ftl_valid_lba(lba) || !mapping_table[lba].valid ||
                mapping_table[lba].block != victim_block || mapping_table[lba].page != p) {
                return FTL_ERR_NAND;
            }

            rc = fcl_read(victim_block, p, temp);
            if (rc != NAND_OK) {
                return FTL_ERR_NAND;
            }

            for (;;) {
                rc = ftl_allocate_free_page(victim_block, &new_block, &new_page);
                if (rc == FTL_OK) {
                    break;
                }
                if (rc != FTL_ERR_NO_SPACE) {
                    return rc;
                }
                return FTL_ERR_NO_SPACE;
            }

            rc = fcl_program(new_block, new_page, temp);
            if (rc != NAND_OK) {
                return FTL_ERR_NAND;
            }

            stats.nand_program_count++;
            stats.page_migration_count++;
            stats.valid_pages_copied++;

            mapping_table[lba].block = new_block;
            mapping_table[lba].page = new_page;
            mapping_table[lba].valid = 1;

            page_state[new_block][new_page] = PAGE_VALID;
            page_owner_lba[new_block][new_page] = lba;
            page_state[victim_block][p] = PAGE_INVALID;
            page_owner_lba[victim_block][p] = -1;
        }
    }

    rc = fcl_erase(victim_block);
    if (rc != NAND_OK) {
        return FTL_ERR_NAND;
    }

    for (p = 0; p < PAGES_PER_BLOCK; p++) {
        page_state[victim_block][p] = PAGE_FREE;
        page_owner_lba[victim_block][p] = -1;
    }

    stats.gc_count++;
    stats.erase_count++;
    return FTL_OK;
}

int ftl_get_stats(FtlStats *out_stats)
{
    if (!out_stats) {
        return FTL_ERR_INVALID;
    }

    *out_stats = stats;
    return FTL_OK;
}

double ftl_get_write_amplification(void)
{
    if (stats.host_write_count == 0) {
        return 0.0;
    }
    return (double)stats.nand_program_count / (double)stats.host_write_count;
}
