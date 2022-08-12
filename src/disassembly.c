#include "disassembly.h"

#include <fileioc.h>
#include <graphx.h>
#include <keypadc.h>
#include <stdio.h>
#include <string.h>

static bool has_equates = false;
static unsigned nr_equates = 0;
static struct equate_t *equates = NULL;
static ti_var_t inc_file_slot = 0;

static char buffer[BUF_SIZE];
static uint32_t buffer_start_addr = 0;
static uint32_t buffer_end_addr = 0;
static bool line_has_label = false;
static unsigned int buffer_offset = 0;
static unsigned int buffer_size = 0;

static unsigned int binary_search_equate(unsigned int addr) {
    unsigned int left = 0;
    unsigned int right = nr_equates - 1;

    while (left <= right) {
        unsigned int mid = (left + right) / 2;

        if (equates[mid].address == addr)
            return equates[mid].string_offset;

        if (equates[mid].address < addr) {
            left = mid + 1;
        } else {
            // Edge case for left = right = mid = 0
            if (!mid) return 0;

            right = mid - 1;
        }
    }

    return 0;
}

static bool put(struct zdis_ctx *ctx, enum zdis_put kind, int32_t val, bool il) {
    char buf[9];
    unsigned int offset;

    switch (kind) {
        case ZDIS_PUT_REL: // JR/DJNZ targets
            val += (int32_t) ctx->zdis_end_addr;

            if (has_equates && (offset = binary_search_equate(val))) {
                buffer[buffer_offset++] = COMMAND_SET_COLOR;
                buffer[buffer_offset++] = 0;
                strcpy(&buffer[buffer_offset], (char *) (ti_GetDataPtr(inc_file_slot) + offset));
                buffer_offset += strlen(&buffer[buffer_offset]);

                return true;
            }
            if (ctx->zdis_user_size & COMPUTE_REL) {
                return put(ctx, ZDIS_PUT_WORD, val, il);
            }

            buffer[buffer_offset++] = COMMAND_SET_COLOR;
            buffer[buffer_offset++] = COLOR_NUM;
            buffer[buffer_offset++] = '$';

            val -= (int32_t) ctx->zdis_start_addr;
            // fallthrough
        case ZDIS_PUT_OFF: // immediate offsets from index registers
            buffer[buffer_offset++] = COMMAND_SET_COLOR;
            buffer[buffer_offset++] = 0;

            if (val > 0) {
                buffer[buffer_offset++] = '+';
            } else if (val < 0) {
                buffer[buffer_offset++] = '-';
                val = -val;
            } else {
                return true;
            }
            // fallthrough
        case ZDIS_PUT_BYTE: // byte immediates
        case ZDIS_PUT_PORT: // immediate ports
        case ZDIS_PUT_RST: // RST targets
            buffer[buffer_offset++] = COMMAND_SET_COLOR;
            buffer[buffer_offset++] = COLOR_NUM;

            if (ctx->zdis_user_size & DECIMAL_IMM) {
                int_to_digits(buf, val);
                strcpy(&buffer[buffer_offset], buf);
                buffer_offset += strlen(buf);

                if (ctx->zdis_user_size & SUFFIXED_IMM) {
                    buffer[buffer_offset++] = 'd';
                }
            } else {
                buffer[buffer_offset++] = ctx->zdis_user_size & SUFFIXED_IMM ? '0' : '$';
                byte_to_hex(buf, val);
                strcpy(&buffer[buffer_offset], buf);
                buffer_offset += strlen(buf);

                if (ctx->zdis_user_size & SUFFIXED_IMM) {
                    buffer[buffer_offset++] = 'h';
                }
            }

            return true;
        case ZDIS_PUT_ABS: // JP/CALL immediate targets
            buffer[buffer_offset++] = COMMAND_SET_COLOR;
            buffer[buffer_offset++] = COLOR_NUM;

            if (ctx->zdis_user_size & COMPUTE_ABS) {
                int32_t extend = il ? 8 : 16;
                buffer[buffer_offset++] = '$';

                return put(ctx, ZDIS_PUT_OFF, (int32_t) (val - ctx->zdis_start_addr) << extend >> extend, il);
            }
            // fallthrough
        case ZDIS_PUT_WORD: // word immediates (il ? 24 : 16) bits wide
        case ZDIS_PUT_ADDR: // load/store immediate addresses
            if (has_equates && (offset = binary_search_equate(val))) {
                buffer[buffer_offset++] = COMMAND_SET_COLOR;
                buffer[buffer_offset++] = 0;
                strcpy(&buffer[buffer_offset], (char *) (ti_GetDataPtr(inc_file_slot) + offset));
                buffer_offset += strlen(&buffer[buffer_offset]);
            } else {
                buffer[buffer_offset++] = COMMAND_SET_COLOR;
                buffer[buffer_offset++] = COLOR_NUM;

                if (ctx->zdis_user_size & DECIMAL_IMM) {
                    int_to_digits(buf, val);
                    strcpy(&buffer[buffer_offset], buf);
                    buffer_offset += strlen(buf);

                    if (ctx->zdis_user_size & SUFFIXED_IMM) {
                        buffer[buffer_offset++] = 'd';
                    }
                } else {
                    buffer[buffer_offset++] = ctx->zdis_user_size & SUFFIXED_IMM ? '0' : '$';
                    if (il) {
                        int_to_hex(buf, val);
                    } else {
                        short_to_hex(buf, val);
                    }
                    strcpy(&buffer[buffer_offset], buf);
                    buffer_offset += strlen(buf);

                    if (ctx->zdis_user_size & SUFFIXED_IMM) {
                        buffer[buffer_offset++] = 'h';
                    }
                }
            }

            return true;
        case ZDIS_PUT_CHAR:
            buffer[buffer_offset++] = (char) val;

            return true;
        case ZDIS_PUT_MNE_SEP:
            buffer[buffer_offset++] = COMMAND_SET_COLOR;
            buffer[buffer_offset++] = 0;

            if (ctx->zdis_user_size & MNE_SPACE) {
                buffer[buffer_offset++] = ' ';
            } else {
                buffer[buffer_offset++] = COMMAND_ARGUMENT_TAB;
            }

            return true;
        case ZDIS_PUT_ARG_SEP:
            buffer[buffer_offset++] = COMMAND_SET_COLOR;
            buffer[buffer_offset++] = 0;
            buffer[buffer_offset++] = ',';

            if (ctx->zdis_user_size & ARG_SPACE) {
                buffer[buffer_offset++] = ' ';
            }

            return true;
        case ZDIS_PUT_END:
            return true;
        default:
            return false;
    }
}

static void draw_buffer(void) {
    uint8_t line = 0;

    for (unsigned int i = 0; i < buffer_size && line < 24; i++) {
        char c = buffer[i];

        if (c == COMMAND_SET_COLOR) {
            gfx_SetTextFGColor(buffer[++i]);
        } else if (c == COMMAND_LABEL_TAB) {
            gfx_SetTextXY(130, gfx_GetTextY());
        } else if (c == COMMAND_BYTES_TAB) {
            gfx_SetMonospaceFont(8);
            gfx_SetTextXY(54, gfx_GetTextY());
        } else if (c == COMMAND_INSTRUCTION_TAB) {
            gfx_SetMonospaceFont(0);
            gfx_SetTextXY(152, gfx_GetTextY());
        } else if (c == COMMAND_ARGUMENT_TAB) {
            gfx_SetTextXY(200, gfx_GetTextY());
        } else if (c == COMMAND_NEWLINE) {
            gfx_SetTextXY(1, gfx_GetTextY() + 10);
            line++;
        } else {
            gfx_PrintChar(c);
        }
    }
}

static bool disassemble_line(struct zdis_ctx *ctx) {
    char buf[10];

    line_has_label = false;

    unsigned int offset;
    if (has_equates && (offset = binary_search_equate((uint24_t) ctx->zdis_end_addr + ctx->zdis_offset))) {
        // A label is found, add it to the buffer
        buffer[buffer_offset++] = COMMAND_LABEL_TAB;
        buffer[buffer_offset++] = COMMAND_SET_COLOR;
        buffer[buffer_offset++] = 0;

        strcpy(&buffer[buffer_offset], (char *) (ti_GetDataPtr(inc_file_slot) + offset));
        buffer_offset += strlen(&buffer[buffer_offset]);
        buffer[buffer_offset++] = COMMAND_NEWLINE;

        line_has_label = true;
    }

    // Add the address to the buffer
    int_to_hex(buf, (uint24_t) ctx->zdis_end_addr + ctx->zdis_offset);
    buffer[buffer_offset++] = COMMAND_SET_COLOR;
    buffer[buffer_offset++] = COLOR_ADDRESS;
    strcpy(&buffer[buffer_offset], buf);
    buffer_offset += 6;

    // Add the bytes to the buffer
    buffer[buffer_offset++] = COMMAND_BYTES_TAB;
    buffer[buffer_offset++] = COMMAND_SET_COLOR;
    buffer[buffer_offset++] = 0;

    int8_t size = zdis_inst_size(ctx);
    for (uint8_t i = 0; i < size; i++) {
        byte_to_hex(buf, ctx->zdis_read(ctx, ctx->zdis_start_addr + i));
        buffer[buffer_offset++] = buf[0];
        buffer[buffer_offset++] = buf[1];
    }

    ctx->zdis_end_addr = ctx->zdis_start_addr;

    // Finally, add the instruction to the buffer
    buffer[buffer_offset++] = COMMAND_INSTRUCTION_TAB;
    buffer[buffer_offset++] = COMMAND_SET_COLOR;
    buffer[buffer_offset++] = COLOR_INSTRUCTION;

    return zdis_put_inst(ctx);
}

static void disassemble(struct zdis_ctx *ctx) {
    uint8_t cur_line = 0;

    memset(buffer, 0, sizeof(buffer));

    ctx->zdis_start_addr = buffer_start_addr;
    ctx->zdis_end_addr = buffer_start_addr;
    while (cur_line < 24 && ctx->zdis_read(ctx, ctx->zdis_end_addr) != EOF) {
        if (!disassemble_line(ctx)) break;

        if (line_has_label) {
            cur_line++;
        }

        buffer[buffer_offset++] = COMMAND_NEWLINE;
        cur_line++;
    }

    buffer_end_addr = ctx->zdis_end_addr;
    buffer_size = buffer_offset;
}

void disassembly(struct zdis_ctx *ctx) {
    uint32_t end_addr = 0;

    ctx->zdis_put = put;
    ctx->zdis_start_addr = 0;
    ctx->zdis_end_addr = 0;
    ctx->zdis_lowercase = true;
    ctx->zdis_implicit = true;
    ctx->zdis_adl = true;
    ctx->zdis_user_size = ARG_SPACE;

    // Open the appvar equates
    inc_file_slot = ti_Open("DISASM", "r");
    if (inc_file_slot) {
        has_equates = true;

        ti_Read(&nr_equates, 3, 1, inc_file_slot);
        equates = (struct equate_t *) ti_GetDataPtr(inc_file_slot);

        ti_Seek(0, SEEK_SET, inc_file_slot);
    }

    disassemble(ctx);

    while (!kb_IsDown(kb_KeyClear)) {
        gfx_FillScreen(255);
        gfx_SetTextXY(1, 1);

        draw_buffer();

        gfx_SetTextFGColor(0);
        gfx_SetColor(255);
        gfx_FillRectangle_NoClip(214, 229, 106, 11);
        gfx_SetColor(0);
        gfx_PrintStringXY("Goto", 217, 231);
        gfx_VertLine_NoClip(215, 230, 10);
        gfx_VertLine_NoClip(248, 230, 10);
        gfx_HorizLine_NoClip(216, 229, 32);
        gfx_PrintStringXY("Settings", 256, 231);
        gfx_VertLine_NoClip(254, 230, 10);
        gfx_VertLine_NoClip(315, 230, 10);
        gfx_HorizLine_NoClip(255, 229, 60);
        gfx_SwapDraw();

        ctx->zdis_end_addr = end_addr;
        ctx->zdis_start_addr = end_addr;

        if (kb_IsDown(kb_KeyUp)) {
            // Try to start 10 bytes from the current start, in order to get the right last instruction
            /* if (ctx->zdis_start_addr >= 10) {
                ctx->zdis_start_addr -= 10;
            } else {
                ctx->zdis_start_addr = 0;
            }

            uint32_t temp_end_addr = ctx->zdis_end_addr;

            while (ctx->zdis_start_addr < temp_end_addr) {
                ctx->zdis_end_addr = ctx->zdis_start_addr;
                zdis_inst_size(ctx);

                if (ctx->zdis_end_addr == temp_end_addr) {
                    // Found the last instruction, set the right address
                    ctx->zdis_end_addr = ctx->zdis_start_addr;
                    break;
                }

                ctx->zdis_start_addr = ctx->zdis_end_addr;
            }

            disassemble(ctx);*/
        } else if (kb_IsDown(kb_KeyDown)) {
            // First, find the next newline in the buffer
            char *found = memchr(buffer, COMMAND_NEWLINE, buffer_size);

            if (found != NULL) {
                found++;

                // If it's found, move the buffer backwards
                memcpy(buffer, found, BUF_SIZE - (found - buffer));
                buffer_offset = (buffer_size -= (found - buffer));

                // Advance the start address of the buffer
                ctx->zdis_end_addr = buffer_start_addr;
                zdis_inst_size(ctx);
                buffer_start_addr = ctx->zdis_end_addr;

                // Next, fetch a new instruction when the last line didn't contain a label
                if (!line_has_label) {
                    ctx->zdis_end_addr = buffer_end_addr;
                    disassemble_line(ctx);
                    buffer_end_addr = ctx->zdis_end_addr;

                    buffer[buffer_offset++] = COMMAND_NEWLINE;
                    buffer_size = buffer_offset;
                } else {
                    line_has_label = false;
                }
            }
        }
    }
}
