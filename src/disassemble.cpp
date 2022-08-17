#include "disassemble.h"
#include "equates.h"

#include <cstring>
#include <graphx.h>
#include <keypadc.h>
#include <cstdio>

static unsigned int buffer_offset = 0;
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

                break;
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
                break;
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
                byte_to_hex(&buffer[buffer_offset], val);
                buffer_offset += 2;

                if (ctx->zdis_user_size & SUFFIXED_IMM) {
                    buffer[buffer_offset++] = 'h';
                }
            }

            break;
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
                        int_to_hex(&buffer[buffer_offset], val);
                        buffer_offset += 6;
                    } else {
                        short_to_hex(&buffer[buffer_offset], val);
                        buffer_offset += 4;
                    }

                    if (ctx->zdis_user_size & SUFFIXED_IMM) {
                        buffer[buffer_offset++] = 'h';
                    }
                }
            }

            break;
        case ZDIS_PUT_CHAR:
            buffer[buffer_offset++] = (char) val;

            break;
        case ZDIS_PUT_MNE_SEP:
            buffer[buffer_offset++] = COMMAND_SET_COLOR;
            buffer[buffer_offset++] = 0;

            if (ctx->zdis_user_size & MNE_SPACE) {
                buffer[buffer_offset++] = ' ';
            } else {
                buffer[buffer_offset++] = COMMAND_ARGUMENT_TAB;
            }

            break;
        case ZDIS_PUT_ARG_SEP:
            buffer[buffer_offset++] = COMMAND_SET_COLOR;
            buffer[buffer_offset++] = 0;
            buffer[buffer_offset++] = ',';

            if (ctx->zdis_user_size & ARG_SPACE) {
                buffer[buffer_offset++] = ' ';
            }

            break;
        case ZDIS_PUT_END:
            break;
        default:
            return false;
    }

    return true;
}

uint8_t Disassembly::set_label(char *buf, char *string) {
    size_t len = strlen(string) + 3;

    buf[0] = COMMAND_LABEL_TAB;
    buf[1] = COMMAND_SET_COLOR;
    buf[2] = 0;
    strcpy(&buf[3], string);
    buf[len++] = ':';

    return len;
}

bool Disassembly::disassemble_line(bool allow_label) {
    auto dis_line = &disassembly_lines[line];
    dis_line->address = ctx->zdis_start_addr = ctx->zdis_end_addr;

    // First check if a label is present
    char *string;
    if (allow_label && (string = equates.find((uint24_t) ctx->zdis_end_addr + ctx->zdis_offset))) {
        dis_line->buffer_size = set_label(dis_line->buffer, string);
        dis_line->instruction_size = 0;

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

void Disassembly::full_disassembly() {
    line = 0;
    while (line < MAX_NR_LINES && ctx->zdis_read(ctx, ctx->zdis_end_addr) != EOF) {
        disassemble_line(true);
    }
}

void Disassembly::run() {
    bool goto_popup = false;
    char goto_buffer[36] = {0};
    unsigned int goto_buffer_offset = 0;

    // First of all, get the equates
    equates = Equates();
    equates.load();
    static_equates = equates;

    // Set the ctx parameters
    ctx->zdis_put = &put;
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
    full_disassembly();

    // Keep displaying the disassembly
    while (!kb_IsDown(kb_KeyClear) || goto_popup) {
        gfx_FillScreen(255);
        gfx_SetTextXY(1, 1);

        // Display all the lines
        for (line = 0; line < MAX_NR_LINES; line++) {
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
                    gfx_SetMonospaceFont(0);
                } else if (c == COMMAND_INSTRUCTION_TAB) {
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
        gfx_SetColor(0);
        gfx_PrintStringXY("Page up", 7, 231);
        gfx_VertLine_NoClip(5, 230, 10);
        gfx_VertLine_NoClip(58, 230, 10);
        gfx_HorizLine_NoClip(6, 229, 52);
        gfx_PrintStringXY("Page down", 66, 231);
        gfx_VertLine_NoClip(64, 230, 10);
        gfx_VertLine_NoClip(133, 230, 10);
        gfx_HorizLine_NoClip(65, 229, 68);
        gfx_PrintStringXY("Goto", 217, 231);
        gfx_VertLine_NoClip(215, 230, 10);
        gfx_VertLine_NoClip(248, 230, 10);
        gfx_HorizLine_NoClip(216, 229, 32);
        gfx_PrintStringXY("Settings", 256, 231);
        gfx_VertLine_NoClip(254, 230, 10);
        gfx_VertLine_NoClip(315, 230, 10);
        gfx_HorizLine_NoClip(255, 229, 60);

        // Popup
        if (goto_popup) {
            gfx_Rectangle_NoClip(19, 114, 283, 12);
            gfx_SetColor(149);
            gfx_FillRectangle_NoClip(20, 115, 281, 10);
            gfx_SetMonospaceFont(8);
            gfx_PrintStringXY(goto_buffer, 21, 117);
            gfx_SetMonospaceFont(0);
        }

        gfx_SwapDraw();
        kb_Scan();

        if (goto_popup) {
            if (kb_IsDown(kb_KeyClear)) {
                goto_popup = false;
                kb_Scan();
            }

            if (goto_buffer_offset < 35) {
                static char keys[] = {'\0',
                                      '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
                                      '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
                                      '\0', '3', '6', '9', '\0', '\0', '\0', '\0',
                                      '\0', '2', '5', '8', '\0', 'F', 'C', '\0',
                                      '0', '1', '4', '7', '\0', 'E', 'B', '\0',
                                      '\0', '\0', '\0', '\0', '\0', 'D', 'A', '\0',
                                      '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};

                for (uint8_t key = 1, group = 7; group; --group) {
                    for (uint8_t mask = 1; mask; mask <<= 1, ++key) {
                        if (kb_Data[group] & mask) {
                            char c = keys[key];

                            if (c) {
                                goto_buffer[goto_buffer_offset++] = c;
                                goto_buffer[goto_buffer_offset] = 0;
                            } else if (key == 56 && goto_buffer_offset) {
                                goto_buffer[--goto_buffer_offset] = 0;
                            } else if (key == 9) {
                                auto addr = (unsigned int) strtol(goto_buffer, nullptr, 16);
                                ctx->zdis_end_addr = addr;

                                full_disassembly();
                                goto_popup = false;
                            }
                        }
                    }
                }
            }
        } else {
            if (kb_IsDown(kb_KeyUp) && disassembly_lines[0].address) {
                struct disassembly_line *line0 = &disassembly_lines[0];
                struct disassembly_line *line1 = &disassembly_lines[1];

                // Shift the lines up
                ctx->zdis_start_addr = ctx->zdis_end_addr = line0->address;
                memmove(line1, line0, sizeof(disassembly_lines[0]) * MAX_NR_LINES);

                // Check if we might add a label
                char *string;
                if (line1->instruction_size && (string = equates.find(line1->address + ctx->zdis_offset))) {
                    line0->instruction_size = 0;
                    line0->buffer_size = set_label(line0->buffer, string);
                    line0->address = line1->address;
                } else {
                    if (ctx->zdis_start_addr >= 6) {
                        ctx->zdis_start_addr -= 6;
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
                    disassemble_line(false);
                }
            } else if (kb_IsDown(kb_KeyDown)) {
                // Shift the lines down
                memmove(&disassembly_lines[0], &disassembly_lines[1], sizeof(disassembly_lines[0]) * MAX_NR_LINES);

                // If the last line is not a label, fetch a new instruction
                struct disassembly_line *tmp_line = &disassembly_lines[MAX_NR_LINES - 2];
                if (tmp_line->instruction_size) {
                    ctx->zdis_end_addr = tmp_line->address + tmp_line->instruction_size;

                    line = MAX_NR_LINES - 1;
                    disassemble_line(true);
                }
            } else if (kb_IsDown(kb_KeyYequ)) {
                ctx->zdis_end_addr = disassembly_lines[0].address;

                if (ctx->zdis_end_addr >= 60) {
                    ctx->zdis_end_addr -= 60;

                    // Get to the next full instruction
                    zdis_inst_size(ctx);
                    zdis_inst_size(ctx);
                    zdis_inst_size(ctx);
                } else {
                    ctx->zdis_end_addr = 0;
                }

                full_disassembly();
            } else if (kb_IsDown(kb_KeyWindow)) {
                ctx->zdis_end_addr = disassembly_lines[MAX_NR_LINES - 1].address +
                                     disassembly_lines[MAX_NR_LINES - 1].instruction_size;

                full_disassembly();
            } else if (kb_IsDown(kb_KeyTrace)) {
                goto_popup = true;
                *goto_buffer = 0;
                goto_buffer_offset = 0;
            }
        }
    }
}
