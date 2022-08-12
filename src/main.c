#include <graphx.h>
#include <keypadc.h>
#include <ti/getcsc.h>

#include "zdis/zdis.h"
#include "os.h"
#include "program.h"

int main(void) {
    // Setup the screen
    gfx_Begin();
    gfx_SetDrawBuffer();

    uint8_t selected_option = 0;
    kb_key_t key;
    while ((key = os_GetCSC()) != sk_Clear && key != sk_Enter) {
        gfx_FillScreen(222);
        gfx_SetColor(0);
        gfx_Rectangle_NoClip(89, 99, 142, 42);
        gfx_SetColor(141);
        gfx_FillRectangle_NoClip(90, 100 + 20 * selected_option, 140, 20);
        gfx_PrintStringXY("Disassembler - by Peter \"PT_\" Tillema", 35, 1);
        gfx_PrintStringXY("Please select an option", 81, 84);
        gfx_PrintStringXY("Operating System", 101, 106);
        gfx_PrintStringXY("Programs", 128, 126);
        gfx_SwapDraw();

        if (key == sk_Up && selected_option) selected_option = 0;
        if (key == sk_Down && !selected_option) selected_option = 1;
    }

    // Setup the keypad
    kb_SetMode(MODE_3_CONTINUOUS);

    // Let's disassemble!
    if (key == sk_Enter) {
        if (selected_option == 0) disassemble_os();
        else disassemble_program();
    }

    gfx_End();

    return 0;
}