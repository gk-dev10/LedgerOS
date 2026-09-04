#ifndef LEDGEROS_BOOT_H
#define LEDGEROS_BOOT_H

#include <stdint.h>
#include <stdbool.h>
#include <limine.h>

/*
 * Boot information provided by Limine.
 *
 * LedgerOS currently uses the framebuffer boot request.
 * Additional boot requests can be added here later when
 * implementing physical memory management.
 */

extern volatile struct limine_framebuffer_request framebuffer_request;

bool boot_check(void);

#endif