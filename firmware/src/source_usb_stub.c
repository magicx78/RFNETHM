// SPDX-License-Identifier: GPL-2.0-or-later
// The P4 Ethernet firmware variant only exposes the RPI-RF-MOD UART source.

#include "source_usb.h"
#include <string.h>

source_t *source_usb_init(void)
{
    return NULL;
}

void source_usb_get_stats(source_usb_stats_t *out)
{
    if (out) memset(out, 0, sizeof(*out));
}
