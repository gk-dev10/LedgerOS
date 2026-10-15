#ifndef LEDGEROS_GUI_H
#define LEDGEROS_GUI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void gui_init(void);
void gui_render_dashboard(void);
void gui_update_price(const char *ticker, uint32_t price);
void gui_update_external_price(const char *ticker, uint32_t price);
void gui_run_live_mode(void);

#endif
