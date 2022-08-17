#ifndef EQUATES_H
#define EQUATES_H

#include <cstdint>

struct equate_t {
    unsigned int address;
    unsigned int string_offset;
};


class Equates {
private:
    bool has_equates = false;
    unsigned int nr_equates = 0;
    struct equate_t *equates = nullptr;
    uint8_t slot_nr = 0;

public:
    void load();

    char *find(unsigned int addr);

    unsigned int find_label(char *label);
};


#endif
