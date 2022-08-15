#include "program.h"
#include "disassemble.h"

#include <cstdint>
#include <cstring>
#include <fileioc.h>
#include <graphx.h>
#include <keypadc.h>
#include <ti/getcsc.h>

static char programs[500][9];

static int read(struct zdis_ctx *ctx, uint32_t addr) {
    ti_var_t file = *(ti_var_t *) ctx->zdis_user_ptr;

    if (ti_Seek(addr + 2, SEEK_SET, file) == EOF) {
        return EOF;
    }

    return ti_GetC(file);
}

static int string_compare(const void *a, const void *b) {
    return strcmp((const char *) a, (const char *) b);
}

unsigned int select_program(unsigned int nr_prog) {
    unsigned int start_program = 0;
    uint8_t relative_selected_program = 0;
    uint8_t nr_programs_display = (nr_prog > PROGRAMS_PER_SCREEN ? PROGRAMS_PER_SCREEN : nr_prog);
    kb_key_t key;

    while ((key = os_GetCSC()) != sk_Enter && key != sk_Clear) {
        // Display buttons
        gfx_FillScreen(222);
        gfx_SetColor(0);
        gfx_Rectangle_NoClip(89, 14, 142, 202);
        gfx_SetColor(141);
        gfx_FillRectangle_NoClip(90, 15 + 20 * relative_selected_program, 140, 20);
        gfx_PrintStringXY("Disassembler - by Peter \"PT_\" Tillema", 35, 1);

        // Display programs
        for (uint8_t i = 0; i < nr_programs_display; i++) {
            gfx_PrintStringXY(programs[start_program + i], 90 + (140 - gfx_GetStringWidth(programs[start_program + i])) / 2, i * 20 + 21);
        }

        gfx_SwapDraw();

        // Check key
        if (key == sk_Up) {
            if (relative_selected_program) relative_selected_program--;
            else if (start_program) start_program--;
        } else if (key == sk_Down) {
            if (relative_selected_program != nr_programs_display - 1) relative_selected_program++;
            else if (start_program != nr_prog - nr_programs_display) start_program++;
        }
    }

    if (key == sk_Clear) {
        return -1;
    }

    return start_program + relative_selected_program;
}

void disassemble_program() {
    char *temp_name;
    uint8_t type;
    ti_var_t slot;
    unsigned int cur_prog = 0;
    void *search_pos = NULL;

    // Find all programs
    while ((temp_name = ti_DetectAny(&search_pos, NULL, &type)) != NULL) {
        if (type == TI_PRGM_TYPE || type == TI_PPRGM_TYPE) {
            slot = ti_OpenVar(temp_name, "r", type);

            if (!slot) continue;

            auto *data = (uint8_t *) ti_GetDataPtr(slot);
            ti_Close(slot);
            if (data[0] != 0xEF || data[1] != 0x7B) continue;

            // Hidden programs
            if (*temp_name < 64) *temp_name += 64;

            strcpy(programs[cur_prog++], temp_name);
        }
    }

    // Sort the programs alphabetically
    if (cur_prog > 1) {
        qsort(programs, cur_prog, sizeof(programs[0]), string_compare);
    }

    unsigned int selected_prog = select_program(cur_prog);
    if (selected_prog == -1) return;

    slot = ti_OpenVar(programs[selected_prog], "r", TI_PRGM_TYPE);
    if (!slot) slot = ti_OpenVar(programs[selected_prog], "r", TI_PPRGM_TYPE);
    if (!slot) {
        gfx_End();

        return;
    }

    // Setup ctx
    struct zdis_ctx ctx{};
    ctx.zdis_user_ptr = &slot;
    ctx.zdis_offset = 0xD1A881;
    ctx.zdis_read = read;

    auto *dis = new Disassembly(&ctx);
    dis->run();

    ti_Close(slot);
}
