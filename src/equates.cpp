#include <fileioc.h>
#include <cstring>

#include "equates.h"

static int equate_compare(const void *a, const void *b) {
    auto *a_eq = (struct equate_t *) a;
    auto *b_eq = (struct equate_t *) b;

    if (a_eq->address == b_eq->address) return 0;

    return a_eq->address < b_eq->address ? -1 : 1;
}

void Equates::load() {
    slot_nr = ti_Open("DISASM", "r");

    if (slot_nr) {
        unsigned int nr_jump_table_equates, j = 0;

        ti_Read(&nr_equates, 3, 1, slot_nr);
        ti_Read(&nr_jump_table_equates, 3, 1, slot_nr);
        has_equates = true;

        if (nr_jump_table_equates) {
            // Should be in RAM, as we're writing back equates
            if (ti_IsArchived(slot_nr)) {
                if (!ti_SetArchiveStatus(false, slot_nr)) {
                    exit(1);
                }
            }

            equates = (struct equate_t *) ti_GetDataPtr(slot_nr);

            // Get everything from the jump tables
            for (unsigned int i = 0; i < nr_equates && j < nr_jump_table_equates; i++) {
                auto *addr = reinterpret_cast<uint8_t *>(equates[i].address - 4);

                // Check if it's a jump, and the next/previous as well
                if (addr[4] == 0xC3 && (addr[0] == 0xC3 || addr[8] == 0xC3)) {
                    equates[nr_equates + j].address = *(unsigned int *) (addr + 5);
                    equates[nr_equates + j].string_offset = equates[i].string_offset - 1;
                    j++;
                }
            }

            if (j < nr_jump_table_equates) nr_jump_table_equates = j;

            // And sort all the equates!
            nr_equates += nr_jump_table_equates;
            qsort(equates, nr_equates, sizeof(struct equate_t), equate_compare);
            j = 0;

            // Write back the right variables
            ti_Rewind(slot_nr);
            ti_Write(&nr_equates, 3, 1, slot_nr);
            ti_Write(&j, 3, 1, slot_nr);

            ti_SetArchiveStatus(true, slot_nr);
        }

        equates = (struct equate_t *) ti_GetDataPtr(slot_nr);

        ti_Rewind(slot_nr);
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

unsigned int Equates::find_label(char *label) {
    auto start = (char *) ti_GetDataPtr(slot_nr);

    for (unsigned int i = 0; i < nr_equates; i++) {
        if (!strcmp(equates[i].string_offset + start, label)) {
            return equates[i].address;
        }
    }

    return 0;
}
