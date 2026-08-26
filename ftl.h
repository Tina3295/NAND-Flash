#ifndef FTL_H
#define FTL_H

#include "nand_flash.h"

#define MAX_LBA (BLOCK_COUNT * PAGES_PER_BLOCK)

#define FTL_OK            0
#define FTL_ERR_INVALID  -100
#define FTL_ERR_UNMAPPED -101
#define FTL_ERR_NO_SPACE -102
#define FTL_ERR_NAND     -103

typedef struct FtlStats {
    unsigned int gc_count;
    unsigned int page_migration_count;
    unsigned int valid_pages_copied;
    unsigned int erase_count;
    unsigned int host_write_count;
    unsigned int nand_program_count;
} FtlStats;

typedef struct MappingEntry {
    int lba;
    int block;
    int page;
    int valid;
} MappingEntry;

void ftl_init(void);
int ftl_write(int lba, const unsigned char *buf);
int ftl_read(int lba, unsigned char *buf);
int ftl_get_mapping(int lba, int *block, int *page);
int ftl_trim(int lba);
int ftl_garbage_collect(void);
int ftl_get_stats(FtlStats *stats);
double ftl_get_write_amplification(void);

#endif
