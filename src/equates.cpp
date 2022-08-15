#include <fileioc.h>

#include "equates.h"

void Equates::load() {
    slot_nr = ti_Open("DISASM", "r");

    if (slot_nr) {
        has_equates = true;

        ti_Read(&nr_equates, 3, 1, slot_nr);

        equates = (struct equate_t *) ti_GetDataPtr(slot_nr);

        ti_Seek(0, SEEK_SET, slot_nr);
    }
}

char *Equates::find(unsigned int addr) {
    if (!has_equates) return nullptr;

    unsigned int left = 0;
    unsigned int right = nr_equates - 1;

    while (left <= right) {
        unsigned int mid = (left + right) / 2;

        if (equates[mid].address == addr) {
            return (char *) ((uint8_t *) ti_GetDataPtr(slot_nr) + equates[mid].string_offset);
        } else if (equates[mid].address < addr) {
            left = mid + 1;
        } else if (!mid) {
            return nullptr;
        } else {
            right = mid - 1;
        }
    }

    return nullptr;
}
