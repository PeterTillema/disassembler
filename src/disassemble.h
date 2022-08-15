#ifndef DISASSEMBLY_H
#define DISASSEMBLY_H

#include <cstdint>

#include "equates.h"
#include "zdis/zdis.h"

#define COMMAND_SET_COLOR (0x00)
#define COMMAND_LABEL_TAB (0x01)
#define COMMAND_BYTES_TAB (0x02)
#define COMMAND_INSTRUCTION_TAB (0x03)
#define COMMAND_ARGUMENT_TAB (0x04)

#define COLOR_INSTRUCTION (25)
#define COLOR_ADDRESS (117)
#define COLOR_NUM (195)

enum Options {
    SUFFIXED_IMM = 1 << 0,
    DECIMAL_IMM = 1 << 1,
    MNE_SPACE = 1 << 2,
    ARG_SPACE = 1 << 3,
    COMPUTE_REL = 1 << 4,
    COMPUTE_ABS = 1 << 5,
};

struct disassembly_line {
    uint32_t address;
    uint8_t instruction_size;
    char buffer[60];
    uint8_t buffer_size;
};

class Disassembly {
private:
    struct zdis_ctx *ctx;
    disassembly_line disassembly_lines[25] = {};
    Equates equates;
    uint8_t line = 0;

    bool disassemble_line();

    static bool put(struct zdis_ctx *ctx, enum zdis_put kind, int32_t val, bool il);

public:
    explicit Disassembly(struct zdis_ctx *ctx);

    void run();
};

void int_to_digits(char *buf, unsigned int val);

void byte_to_hex(char *buf, uint8_t byte);

void short_to_hex(char *buf, unsigned int val);

void int_to_hex(char *buf, unsigned int val);


#endif
