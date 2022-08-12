#include <stdio.h>
#include "disassembly.h"
#include "os.h"
#include "zdis/zdis.h"

static int read(struct zdis_ctx *ctx, uint32_t addr) {
    (void)ctx;

    if (addr <= 0xFFFFFF) {
        return *(uint8_t *) addr;
    }

    return EOF;
}

void disassemble_os(void) {
    struct zdis_ctx ctx;
    ctx.zdis_offset = 0;
    ctx.zdis_read = read;

    disassembly(&ctx);
}
