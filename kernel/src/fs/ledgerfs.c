#include "ledgerfs.h"
#include "../memory.h"
#include "../sync/spinlock.h"
#include "../timer/timer.h"

static char log_store[MAX_LOG_ENTRIES][LOG_ENTRY_SIZE];
static size_t log_count = 0;
static spinlock_t log_lock;

void ledgerfs_init(void) {
    spinlock_init(&log_lock);
    log_count = 0;
    memset(log_store, 0, sizeof(log_store));
}

bool ledgerfs_write_log(const char *entry) {
    if (!entry) return false;

    spinlock_lock(&log_lock);

    size_t slot = (log_count < MAX_LOG_ENTRIES) ? log_count : (log_count % MAX_LOG_ENTRIES);
    log_count++;

    size_t len = 0;
    while (entry[len] && len < LOG_ENTRY_SIZE - 1) len++;

    memcpy(log_store[slot], entry, len);
    log_store[slot][len] = '\0';

    spinlock_unlock(&log_lock);
    return true;
}

size_t ledgerfs_read_logs(char *buf, size_t max_buf_size) {
    if (!buf || max_buf_size == 0) return 0;

    spinlock_lock(&log_lock);
    size_t offset = 0;
    size_t displayed = (log_count > MAX_LOG_ENTRIES) ? MAX_LOG_ENTRIES : log_count;

    for (size_t i = 0; i < displayed; i++) {
        size_t len = 0;
        while (log_store[i][len]) len++;

        if (offset + len + 1 >= max_buf_size) break;

        memcpy(buf + offset, log_store[i], len);
        offset += len;
        buf[offset++] = '\n';
    }

    buf[offset] = '\0';
    spinlock_unlock(&log_lock);
    return log_count;
}

size_t ledgerfs_get_count(void) {
    return log_count;
}
