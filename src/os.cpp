#include "disassemble.h"
#include "os.h"
#include "zdis/zdis.h"

#include <cstdio>

static int read(struct zdis_ctx *ctx, uint32_t addr) {
    (void) ctx;

    if (addr <= 0xFFFFFF) {
        return *(uint8_t *) addr;
    }

    return EOF;
}

void disassemble_os() {
    struct zdis_ctx ctx{};
    ctx.zdis_offset = 0;
    ctx.zdis_read = read;

    auto *dis = new Disassembly(&ctx);
    dis->run();
}
