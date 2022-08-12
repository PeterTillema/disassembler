#ifndef DISASSEMBLY_H
#define DISASSEMBLY_H

#include "zdis/zdis.h"

#define COMMAND_SET_COLOR (0x00)
#define COMMAND_LABEL_TAB (0x01)
#define COMMAND_BYTES_TAB (0x02)
#define COMMAND_INSTRUCTION_TAB (0x03)
#define COMMAND_ARGUMENT_TAB (0x04)
#define COMMAND_NEWLINE (0x0A)

#define COLOR_INSTRUCTION (25)
#define COLOR_ADDRESS (117)
#define COLOR_NUM (195)

#define BUF_SIZE (1000)

enum Options {
    SUFFIXED_IMM = 1 << 0,
    DECIMAL_IMM = 1 << 1,
    MNE_SPACE = 1 << 2,
    ARG_SPACE = 1 << 3,
    COMPUTE_REL = 1 << 4,
    COMPUTE_ABS = 1 << 5,
};

struct equate_t {
    unsigned int address;
    unsigned int string_offset;
};

void disassembly(struct zdis_ctx *ctx);

void int_to_digits(char *buf, unsigned int val);

void byte_to_hex(char *buf, uint8_t byte);

void short_to_hex(char *buf, unsigned int val);

void int_to_hex(char *buf, unsigned int val);

#endif
