#ifndef LEDGEROS_LEDGERFS_H
#define LEDGEROS_LEDGERFS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_LOG_ENTRIES 128
#define LOG_ENTRY_SIZE 128

void ledgerfs_init(void);
bool ledgerfs_write_log(const char *entry);
size_t ledgerfs_read_logs(char *buf, size_t max_buf_size);
size_t ledgerfs_get_count(void);

#endif
