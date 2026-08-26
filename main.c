#include "dv.h"
#include "ftl.h"
#include "storage_sdk.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    uint8_t write_data[PAGE_SIZE];
    uint8_t read_data[PAGE_SIZE];
    FtlStats stats;

    if (storage_init() != STORAGE_OK) {
        puts("storage_init failed");
        return 1;
    }

    memset(write_data, 0xAB, sizeof(write_data));
    if (storage_write(100, write_data, sizeof(write_data)) != STORAGE_OK) {
        puts("storage_write failed");
        return 1;
    }

    memset(read_data, 0x00, sizeof(read_data));
    if (storage_read(100, read_data, sizeof(read_data)) != STORAGE_OK) {
        puts("storage_read failed");
        return 1;
    }

    if (memcmp(write_data, read_data, sizeof(write_data)) != 0) {
        puts("read after write mismatch");
        return 1;
    }

    if (ftl_get_stats(&stats) != FTL_OK) {
        puts("ftl_get_stats failed");
        return 1;
    }

    printf("SDK demo: host_write_count = %u\n", stats.host_write_count);
    printf("SDK demo: nand_program_count = %u\n", stats.nand_program_count);
    printf("SDK demo: write_amplification = %.3f\n", ftl_get_write_amplification());

    return run_dv_tests();
}
