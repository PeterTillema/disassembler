#include "disassemble.h"
#include "equates.h"

#include <cstring>
#include <graphx.h>
#include <keypadc.h>

static uint8_t buffer_offset = 0;
static char *buffer;
static Equates static_equates;

Disassembly::Disassembly(struct zdis_ctx *ctx) {
    this->ctx = ctx;
}

bool Disassembly::put(struct zdis_ctx *ctx, enum zdis_put kind, int32_t val, bool il) {
    char buf[9];
    char *string;

    switch (kind) {
        case ZDIS_PUT_REL: // JR/DJNZ targets
            val += (int32_t) ctx->zdis_end_addr;

            if ((string = static_equates.find(val))) {
                buffer[buffer_offset++] = COMMAND_SET_COLOR;
                buffer[buffer_offset++] = 0;
                strcpy(&buffer[buffer_offset], string);
                buffer_offset += strlen(string);

                return true;
            }
            if (ctx->zdis_user_size & COMPUTE_REL) {
                return put(ctx, ZDIS_PUT_WORD, val, il);
            }

            buffer[buffer_offset++] = COMMAND_SET_COLOR;
            buffer[buffer_offset++] = (char) COLOR_NUM;
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
            buffer[buffer_offset++] = (char) COLOR_NUM;

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
            buffer[buffer_offset++] = (char) COLOR_NUM;

            if (ctx->zdis_user_size & COMPUTE_ABS) {
                int32_t extend = il ? 8 : 16;
                buffer[buffer_offset++] = '$';

                return put(ctx, ZDIS_PUT_OFF, (int32_t) (val - ctx->zdis_start_addr) << extend >> extend, il);
            }
            // fallthrough
        case ZDIS_PUT_WORD: // word immediates (il ? 24 : 16) bits wide
        case ZDIS_PUT_ADDR: // load/store immediate addresses
            if ((string = static_equates.find(val))) {
                buffer[buffer_offset++] = COMMAND_SET_COLOR;
                buffer[buffer_offset++] = 0;
                strcpy(&buffer[buffer_offset], string);
                buffer_offset += strlen(string);
            } else {
                buffer[buffer_offset++] = COMMAND_SET_COLOR;
                buffer[buffer_offset++] = (char) COLOR_NUM;

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

bool Disassembly::disassemble_line() {
    auto dis_line = &disassembly_lines[line];
    dis_line->address = ctx->zdis_start_addr = ctx->zdis_end_addr;

    // First check if a label is present
    char *string;
    if ((string = equates.find((uint24_t) ctx->zdis_end_addr + ctx->zdis_offset))) {
        dis_line->buffer[0] = COMMAND_LABEL_TAB;
        dis_line->buffer[1] = COMMAND_SET_COLOR;
        dis_line->buffer[2] = 0;
        strcpy(&dis_line->buffer[3], string);

        dis_line->instruction_size = 0;
        dis_line->buffer_size = strlen(string) + 3;
        dis_line->buffer[dis_line->buffer_size++] = ':';

        dis_line = &disassembly_lines[line + 1];
        dis_line->address = ctx->zdis_start_addr;

        line++;
    }

    // Set the address
    dis_line->buffer[0] = COMMAND_SET_COLOR;
    dis_line->buffer[1] = COLOR_ADDRESS;
    int_to_hex(&dis_line->buffer[2], (uint24_t) ctx->zdis_end_addr + ctx->zdis_offset);
    dis_line->buffer[8] = COMMAND_BYTES_TAB;
    dis_line->buffer[9] = COMMAND_SET_COLOR;
    dis_line->buffer[10] = 0;

    // Set the bytes
    int8_t size = zdis_inst_size(ctx);
    buffer_offset = 11;
    for (uint8_t i = 0; i < size; i++) {
        byte_to_hex(&dis_line->buffer[buffer_offset], ctx->zdis_read(ctx, ctx->zdis_start_addr + i));
        buffer_offset += 2;
    }

    dis_line->instruction_size = ctx->zdis_end_addr - ctx->zdis_start_addr;
    ctx->zdis_end_addr = ctx->zdis_start_addr;

    // Set the instruction
    dis_line->buffer[buffer_offset++] = COMMAND_INSTRUCTION_TAB;
    dis_line->buffer[buffer_offset++] = COMMAND_SET_COLOR;
    dis_line->buffer[buffer_offset++] = COLOR_INSTRUCTION;

    buffer = dis_line->buffer;
    bool out = zdis_put_inst(ctx);

    dis_line->buffer_size = buffer_offset;

    line++;

    return out;
}

void Disassembly::run() {
    // First of all, get the equates
    equates = Equates();
    equates.load();
    static_equates = equates;

    // Setup the ctx parameters
    ctx->zdis_put = &Disassembly::put;
    ctx->zdis_start_addr = 0;
    ctx->zdis_end_addr = 0;
    ctx->zdis_lowercase = true;
    ctx->zdis_implicit = true;
    ctx->zdis_adl = true;
    ctx->zdis_user_size = ARG_SPACE;

    // Custom character
    uint8_t dots[8] = {0, 0, 0, 0, 0, 0, 0xDB, 0xDB};
    gfx_SetCharData(1, dots);

    // Next, disassembly the first lines
    while (line < 24) {
        disassemble_line();
    }

    // Keep displaying the disassembly
    while (!kb_IsDown(kb_KeyClear)) {
        gfx_FillScreen(255);
        gfx_SetTextXY(1, 1);

        // Display all the lines
        for (line = 0; line < 24; line++) {
            auto dis_line = &disassembly_lines[line];
            for (uint8_t i = 0; i < dis_line->buffer_size; i++) {
                char c = dis_line->buffer[i];

                if (c == COMMAND_SET_COLOR) {
                    gfx_SetTextFGColor(dis_line->buffer[++i]);
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
                } else {
                    if (gfx_GetTextX() > GFX_LCD_WIDTH - 16) {
                        gfx_PrintChar('\x01');
                        break;
                    } else {
                        gfx_PrintChar(c);
                    }
                }
            }

            gfx_SetTextXY(1, gfx_GetTextY() + 10);
        }

        // Other graphics
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

        if (kb_IsDown(kb_KeyUp) && disassembly_lines[0].address) {
            // Shift the lines up
            ctx->zdis_start_addr = ctx->zdis_end_addr = disassembly_lines[0].address;
            memmove(&disassembly_lines[1], &disassembly_lines[0], sizeof (disassembly_lines[0]) * 24);

            if (ctx->zdis_start_addr >= 15) {
                ctx->zdis_start_addr -= 15;
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

            line = 0;
            disassemble_line();
        } else if (kb_IsDown(kb_KeyDown)) {
            // Shift the lines down
            memmove(&disassembly_lines[0], &disassembly_lines[1], sizeof (disassembly_lines[0]) * 24);

            // If the last line is not a label, fetch a new instruction
            if (disassembly_lines[22].instruction_size) {
                ctx->zdis_end_addr = disassembly_lines[22].address + disassembly_lines[22].instruction_size;

                line = 23;
                disassemble_line();
            }
        }
    }
}
